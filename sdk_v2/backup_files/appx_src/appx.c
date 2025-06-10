/******************************************************************************
*
* Copyright (C) 2015 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file xipipsu_self_test_example.c
*
* This file consists of a self test example which uses the XIpiPsu driver to
* send an IPI message to self and get a response
* Each IPI channel can trigger an interrupt to itself and can exchange messages
* through the message buffer. This feature is used here to exercise the driver
* APIs.
* Example control flow:
* - Init the IPI and GIC drivers
* - Setup Interrupt System with IPI handler which inverts the received message
*   and sends back as response
* - Write a Message and Trigger IPI to Self.
* - Keep polling for response till timeout
* - Interrupt handler receives IPI and sends back response
* - Read the received response and do a sanity check
* - Print PASS or FAIL based on sanity check of response message
******************************************************************************/
#include "stdlib.h"
#include "xil_types.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>

/***************************** Include Files *********************************/

#include "xparameters.h"
#include "xil_exception.h"
#include "xil_cache.h"
#include "xscugic.h"
#include "xipipsu.h"
#include "xipipsu_hw.h"
#include "gcm.h"    // define the various AES-GCM library functions

/************************* Test Configuration ********************************/
/* IPI device ID to use for this test */
#define TEST_CHANNEL_ID	XPAR_XIPIPSU_0_DEVICE_ID
/* Test message length in words. Max is 8 words (32 bytes) */
#define TEST_MSG_LEN	4
#define TEST_KEY_LEN	8
#define TEST_IV_LEN		3
#define TEST_AAD_LEN	4
#define TEST_TAG_LEN	4

/* Interrupt Controller device ID */
#define INTC_DEVICE_ID	XPAR_SCUGIC_0_DEVICE_ID
/* Time out parameter while polling for response */
#define TIMEOUT_COUNT 800000

/*****************************************************************************/

/* Global Instances of GIC and IPI devices */
XScuGic GicInst;
XIpiPsu IpiInst;

/* Buffers to store Test Data */
u32 MsgBuffer_rpu[TEST_MSG_LEN];
u32 CtBuffer_rpu[TEST_MSG_LEN];
u32 KeyBuffer_rpu[TEST_KEY_LEN];
u32 IvBuffer_rpu[TEST_IV_LEN];
u32 TagBuffer_rpu[TEST_TAG_LEN];
u32 AadBuffer_rpu[TEST_AAD_LEN];
/**
 * Interrupt Handler :
 * -Polls for each of the valid sources
 * -Checks if there is a message
 * -Reads the message
 * -Inverts the bits
 * -Sends back the inverted message as response
 *
 */
void IpiIntrHandler(void *XIpiPsuPtr)
{

	u32 IpiSrcMask; /**< Holds the IPI status register value */
	u32 Index;

	u32 TmpBufPtr[TEST_MSG_LEN] = { 0 }; /**< Holds the received Message */

	u32 SrcIndex;
	XIpiPsu *InstancePtr = (XIpiPsu *) XIpiPsuPtr;

	xil_printf("\n\n-----Enter Interrupt Handler\r\n");

	Xil_AssertVoid(InstancePtr!=NULL);

	IpiSrcMask = XIpiPsu_GetInterruptStatus(InstancePtr);

	/* Poll for each source */

	for (SrcIndex = 0U; SrcIndex < InstancePtr->Config.TargetCount; SrcIndex++)
	{

		if (IpiSrcMask & InstancePtr->Config.TargetList[SrcIndex].Mask) {

			/*  Read Incoming Message Buffer Corresponding to Source CPU */
			XIpiPsu_ReadMessage(InstancePtr,
					InstancePtr->Config.TargetList[SrcIndex].Mask, TmpBufPtr,
					TEST_MSG_LEN, XIPIPSU_BUF_TYPE_MSG);

			xil_printf("     Message Received From FPGA_TA:\r\n");


			for (Index = 0; Index < TEST_MSG_LEN; Index++) {
				xil_printf("     APPx W%d Expected  -> 0x%08x :Received 0x%08x\r\n", Index, MsgBuffer_rpu[Index],
						TmpBufPtr[Index]);
				if (MsgBuffer_rpu[Index] != (TmpBufPtr[Index])) {
					xil_printf("     Message mismatch %d\r\n", Index);
				}
			}

			/* Clear the Interrupt Status - This clears the OBS bit on the SRC CPU registers */
			XIpiPsu_ClearInterruptStatus(InstancePtr,
					InstancePtr->Config.TargetList[SrcIndex].Mask);


		}
	}

	xil_printf("-----Exit Interrupt Handler\r\n\n\n");


}


static XStatus SetupInterruptSystem(XScuGic *IntcInstancePtr,
		XIpiPsu *IpiInstancePtr, u32 IpiIntrId) {
	u32 Status = 0;
	XScuGic_Config *IntcConfig; /* Config for interrupt controller */

	/* Initialize the interrupt controller driver */
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(&GicInst, IntcConfig,
			IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Connect the interrupt controller interrupt handler to the
	 * hardware interrupt handling logic in the processor.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			(Xil_ExceptionHandler) XScuGic_InterruptHandler, IntcInstancePtr);

	/*
	 * Connect a device driver handler that will be called when an
	 * interrupt for the device occurs, the device driver handler
	 * performs the specific interrupt processing for the device
	 */
	 xil_printf("Setting Up Interrupt ID: %d\r\n\n\n",IpiIntrId);
	Status = XScuGic_Connect(IntcInstancePtr, IpiIntrId,
			(Xil_InterruptHandler) IpiIntrHandler, (void *) IpiInstancePtr);

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/* Enable the interrupt for the device */
	XScuGic_Enable(IntcInstancePtr, IpiIntrId);

	/* Enable interrupts */
	Xil_ExceptionEnable();

	return XST_SUCCESS;
}


/**
 * @brief	Tests the IPI by sending a message and checking the response
 */

static XStatus DoIpiTest(XIpiPsu *InstancePtr)
{

	XIpiPsu_Config *DestCfgPtr;
	u32 Status = 0;
	DestCfgPtr = XIpiPsu_LookupConfig(TEST_CHANNEL_ID);

	/**
	 * Send a Message to TEST_TARGET and WAIT for ACK
	 */

	XIpiPsu_WriteMessage(InstancePtr,
			DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,
			KeyBuffer_rpu,
			TEST_KEY_LEN,
			XIPIPSU_BUF_TYPE_MSG);

	Status = XIpiPsu_TriggerIpi(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask);

		if (Status == !XST_SUCCESS) {
			xil_printf("---APPx's AES Key is not sent to FPGA_TA!--- \r\n");
		} else {
			xil_printf("---APPx's AES Key is sent to FPGA_TA---> \r\n");
		}

	Status = XIpiPsu_PollForAck(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,TIMEOUT_COUNT);


		if (Status == !XST_SUCCESS) {
			xil_printf("---ACK TIMEOUT: APPx's Message to FPGA_TA Not Acknowledged in Time Alloted (TIMEOUT_COUNT)!--- \r\n");
		} else {
			xil_printf("\n\n\n\n\n\n\n\n\n\n---APPx's AES Key is received --- \r\n\n\n");
		}

	usleep(100000);

	XIpiPsu_WriteMessage(InstancePtr,
			DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,
			IvBuffer_rpu,
			TEST_IV_LEN,
			XIPIPSU_BUF_TYPE_MSG);

	Status = XIpiPsu_TriggerIpi(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask);

		if (Status == !XST_SUCCESS) {
			xil_printf("---APPx's IV is not sent to FPGA_TA!--- \r\n");
		} else {
			xil_printf("---APPx's IV is sent to FPGA_TA---> \r\n");
		}

	Status = XIpiPsu_PollForAck(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,TIMEOUT_COUNT);

		if (Status == !XST_SUCCESS) {
			xil_printf("---ACK TIMEOUT: APPx's Message to FPGA_TA Not Acknowledged in Time Alloted (TIMEOUT_COUNT)!--- \r\n");
		} else {
			xil_printf("\n\n\n\n\n\n\n\n\n\n---APPx's IV is received--- \r\n\n\n");
		}

	usleep(100000);

	XIpiPsu_WriteMessage(InstancePtr,
			DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,
			AadBuffer_rpu,
			TEST_AAD_LEN,
			XIPIPSU_BUF_TYPE_MSG);

	Status = XIpiPsu_TriggerIpi(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask);

		if (Status == !XST_SUCCESS) {
			xil_printf("---APPx's AAD is not sent to FPGA_TA!--- \r\n");
		} else {
			xil_printf("---APPx's AAD is sent to FPGA_TA---> \r\n");
		}

	Status = XIpiPsu_PollForAck(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,TIMEOUT_COUNT);

		if (Status == !XST_SUCCESS) {
			xil_printf("---ACK TIMEOUT: APPx's Message to FPGA_TA Not Acknowledged in Time Alloted (TIMEOUT_COUNT)!--- \r\n");
		} else {
			xil_printf("\n\n\n\n\n\n\n\n\n\n---APPx's AAD is received--- \r\n\n\n");
		}

	usleep(100000);

	XIpiPsu_WriteMessage(InstancePtr,
			DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,
			CtBuffer_rpu,
			TEST_MSG_LEN,
			XIPIPSU_BUF_TYPE_MSG);

	Status = XIpiPsu_TriggerIpi(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask);

		if (Status == !XST_SUCCESS) {
			xil_printf("---APPx's Ciphertext is not sent to FPGA_TA!--- \r\n");
		} else {
			xil_printf("---APPx's Ciphertext is sent to FPGA_TA---> \r\n");
		}

	Status = XIpiPsu_PollForAck(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,TIMEOUT_COUNT);

		if (Status == !XST_SUCCESS) {
			xil_printf("---ACK TIMEOUT: APPx's Message to FPGA_TA Not Acknowledged in Time Alloted (TIMEOUT_COUNT)!--- \r\n");
		} else {
			xil_printf("\n\n\n\n\n\n\n\n\n\n---APPx's Ciphertext is received--- \r\n\n\n");
		}

	usleep(100000);

	XIpiPsu_WriteMessage(InstancePtr,
			DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,
			TagBuffer_rpu,
			TEST_TAG_LEN,
			XIPIPSU_BUF_TYPE_MSG);

	Status = XIpiPsu_TriggerIpi(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask);

		if (Status == !XST_SUCCESS) {
			xil_printf("---APPx's Tag is not sent to FPGA_TA!--- \r\n");
		} else {
			xil_printf("---APPx's Tag is sent to FPGA_TA---> \r\n");
		}

	Status = XIpiPsu_PollForAck(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,TIMEOUT_COUNT);

		if (Status == !XST_SUCCESS) {
			xil_printf("---ACK TIMEOUT: APPx's Message to FPGA_TA Not Acknowledged in Time Alloted (TIMEOUT_COUNT)!--- \r\n");
		} else {
			xil_printf("\n\n\n\n\n\n\n\n\n\n---APPx's Tag is received--- \r\n\n\n");
		}

	/*XIpiPsu_WriteMessage(InstancePtr,
				DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXA53_0_CH0_INDEX].Mask,
				*MsgPtr,
				MsgLength,
				XIPIPSU_BUF_TYPE_MSG);

	Status = XIpiPsu_TriggerIpi(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXA53_0_CH0_INDEX].Mask);

	if (Status == !XST_SUCCESS) {
		xil_printf("---APPx's Message is not sent to FPGA_TA!--- \r\n");
	} else {
		xil_printf("---APPx's Message is sent to FPGA_TA---> \r\n");
	}

	Status = XIpiPsu_PollForAck(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXA53_0_CH0_INDEX].Mask,TIMEOUT_COUNT);

	if (Status == !XST_SUCCESS) {
		xil_printf("---ACK TIMEOUT: RPU Message to APU Not Acknowledged in Time Alloted (TIMEOUT_COUNT)!--- \r\n");
	} else {
		xil_printf("\n\n\n\n\n\n\n\n\n\n---APPx's Message is received--- \r\n\n\n");
	}*/
	return XST_SUCCESS;
}

int main() {

	//SEM
	  usleep(700000);
	//SEM


	XIpiPsu_Config *CfgPtr;
	u32 Index;
	int ret;
	gcm_context ctx;
	int Status = XST_FAILURE;
	size_t key_length_b = 32;
	size_t msg_length_b = 16;
	size_t iv_length_b = 12;
	size_t aad_length_b = 16;
	size_t tag_length_b = 16;
	uchar *ct; //= (uchar *)malloc(msg_length_b);
	uchar *pt;//= (uchar *)malloc(msg_length_b);
	uchar *key;//= (uchar *)malloc(key_length_b);
	uchar *iv;// = (uchar *)malloc(iv_length_b);
	uchar *aad;//= (uchar *)malloc(aad_length_b);
	uchar *tag;//= (uchar *)malloc(tag_length_b);
	const char *Key_ = "7f7168a406e7c1ef0fd47ac922c5ec5f659765fb6aaa048f7056f6c6b5d8513d";
	const char *IV_  = "b8b5e407adc0e293e3e7e991";
	const char *AAD_ = "ff7628f6427fbcef1f3b82b37404e116";
	const char *PT_  = "b706194bb0b10c474e1b2d7b2278224c";
	//CT =  8fada0b8 e777a829 ca9680d3 bf4f3574
	//Tag = daca3542 77f6335f c8bec908 86da70


	xil_printf("\n\n\n\n\n\nAPPx [Build: %s %s]\r\n", __DATE__, __TIME__);

	Xil_DCacheDisable();

	pt= (uchar *)malloc(msg_length_b);
	key= (uchar *)malloc(key_length_b);
	iv = (uchar *)malloc(iv_length_b);
	aad= (uchar *)malloc(aad_length_b);
	ct= (uchar *)malloc(msg_length_b);
	tag= (uchar *)malloc(tag_length_b);

	for (size_t i = 0; i < key_length_b; i++) {
		sscanf(Key_ + 2*i, "%2hhx", &key[key_length_b-1-i]);
	}
	for (size_t i = 0; i < iv_length_b; i++) {
		sscanf(IV_ + 2*i, "%2hhx", &iv[iv_length_b-1-i]);
	}
	for (size_t i = 0; i < msg_length_b; i++) {
		sscanf(PT_ + 2*i, "%2hhx", &pt[msg_length_b-1-i]);
	}
	for (size_t i = 0; i < aad_length_b; i++) {
		sscanf(AAD_ + 2*i, "%2hhx", &aad[aad_length_b-1-i]);
	}
	gcm_initialize();

	gcm_setkey( &ctx, key, (const uint)key_length_b );

	//gcm_crypt_and_tag( &ctx, ENCRYPT, (uchar*)&IvBuffer_rpu[0], iv_length_b, (uchar*)&AadBuffer_rpu[0], aad_length_b,
		//		(uchar*)&MsgBuffer_rpu[0], (uchar*)&CtBuffer_rpu[0], msg_length_b, (uchar*)&TagBuffer_rpu[0], tag_length_b);
	gcm_crypt_and_tag( &ctx, ENCRYPT, iv, iv_length_b, aad, aad_length_b, pt, ct, msg_length_b, tag, tag_length_b);

	xil_printf("APPx Message Content:\r\n");

	memcpy(MsgBuffer_rpu, pt, msg_length_b);
	//free(pt);
	for (Index = 0; Index < TEST_MSG_LEN; Index++) {
		xil_printf("PlainText%d: 0x%08x\r\n", Index, MsgBuffer_rpu[Index]);
	}
	xil_printf("\n");
	memcpy(KeyBuffer_rpu, key, key_length_b);
	//free(key);
	for (Index = 0; Index < TEST_KEY_LEN; Index++) {
		xil_printf("Key%d: 0x%08x\r\n", Index, KeyBuffer_rpu[Index]);
	}
	xil_printf("\n");
	memcpy(IvBuffer_rpu, iv, iv_length_b);
	//free(iv);
	for (Index = 0; Index < TEST_IV_LEN; Index++) {
		xil_printf("IV%d: 0x%08x\r\n", Index, IvBuffer_rpu[Index]);
	}
	xil_printf("\n");
	memcpy(AadBuffer_rpu, aad, aad_length_b);
	//free(aad);
	for (Index = 0; Index < TEST_AAD_LEN; Index++) {
		xil_printf("AAD%d: 0x%08x\r\n", Index, AadBuffer_rpu[Index]);
	}
	xil_printf("\n");
	//gcm_setkey( &ctx, (uchar*)&KeyBuffer_rpu[0], (const uint)key_length_b );

	//gcm_crypt_and_tag( &ctx, ENCRYPT, (uchar*)&IvBuffer_rpu[0], iv_length_b, (uchar*)&AadBuffer_rpu[0], aad_length_b,
		//			(uchar*)&MsgBuffer_rpu[0], (uchar*)&CtBuffer_rpu[0], msg_length_b, (uchar*)&TagBuffer_rpu[0], tag_length_b);

	memcpy(CtBuffer_rpu, ct, msg_length_b);
	//free(ct);
	for (Index = 0; Index < TEST_MSG_LEN; Index++) {
		xil_printf("CipherText%d: 0x%08x\r\n", Index, CtBuffer_rpu[Index]);
	}
	xil_printf("\n");
	memcpy(TagBuffer_rpu, tag, tag_length_b);
	//free(tag);
	for (Index = 0; Index < TEST_TAG_LEN; Index++) {
		xil_printf("TAG%d: 0x%08x\r\n", Index, TagBuffer_rpu[Index]);
	}

	gcm_zero_ctx( &ctx );
	/* Look Up the config data */
	CfgPtr = XIpiPsu_LookupConfig(TEST_CHANNEL_ID);

	/* Init with the Cfg Data */
	XIpiPsu_CfgInitialize(&IpiInst, CfgPtr, CfgPtr->BaseAddress);

	/* Setup the GIC */
	SetupInterruptSystem(&GicInst, &IpiInst, 67);

	/* Enable reception of IPIs from all CPUs */
	XIpiPsu_InterruptEnable(&IpiInst, XIPIPSU_ALL_MASK);

	/* Clear Any existing Interrupts */
	XIpiPsu_ClearInterruptStatus(&IpiInst, XIPIPSU_ALL_MASK);

	//SEM
	usleep(700000);
	//SEM
	Status = DoIpiTest(&IpiInst);





	do {
		/**
		 * Do Nothing
		 * We need to loop on to receive IPIs and respond to them
		 */
		__asm("wfi");
	} while (1);

	/* Control never reaches here */
	return Status;

}
