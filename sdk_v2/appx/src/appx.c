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

#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#define RSA_KEY_SIZE 512 // 512-bit RSA
/************************* Test Configuration ********************************/
/* IPI device ID to use for this test */
#define TEST_CHANNEL_ID	XPAR_XIPIPSU_0_DEVICE_ID
/* Test message length in words. Max is 8 words (32 bytes) */
/*Because of this limitation we will send data chunk by chunk */
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
u32 MsgBuffer_apu[TEST_MSG_LEN];
u32 MsgBuffer_rpu[TEST_MSG_LEN] = { 0 };
u32 KeyBuffer_apu[TEST_KEY_LEN];
u32 KeyBuffer_apu_1[TEST_KEY_LEN];// new keybuffer
u32 IvBuffer_apu[TEST_IV_LEN];
u32 TagBuffer_apu[TEST_TAG_LEN];
u32 AadBuffer_apu[TEST_AAD_LEN];
u32 msg_count = 0;
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
	//u32 Index;
	u32 Status;
	//u32 TmpBufPtr[TEST_MSG_LEN] = { 0 }; /**< Holds the received Message */

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
			Status= XIpiPsu_ReadMessage(InstancePtr,
					InstancePtr->Config.TargetList[SrcIndex].Mask, MsgBuffer_rpu, //TmpBufPtr,
					TEST_MSG_LEN, XIPIPSU_BUF_TYPE_MSG);

			if (Status == XST_FAILURE) {
				xil_printf("     Message is not Received from TA_FPGA\r\n");
			} else {
				msg_count = msg_count + 1;
				xil_printf("     Message is Received From TA_FPGA\r\n");
				//for (Index = 0; Index < TEST_MSG_LEN; Index++)
					//xil_printf("Message%d: 0x%08x\r\n", Index, MsgBuffer_rpu[Index]);
			}

			/*for (Index = 0; Index < TEST_MSG_LEN; Index++) {
				xil_printf("     APPx W%d Expected  -> 0x%08x :Received 0x%08x\r\n", Index, MsgBuffer_apu[Index],
						TmpBufPtr[Index]);
				if (MsgBuffer_apu[Index] != (TmpBufPtr[Index])) {
					xil_printf("     Message mismatch %d\r\n", Index);
				}
			}*/

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
			MsgBuffer_apu,
			TEST_MSG_LEN,
			XIPIPSU_BUF_TYPE_MSG);

	Status = XIpiPsu_TriggerIpi(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask);

		if (Status == !XST_SUCCESS) {
			xil_printf("---APPx's Message is not sent to TA_FPGA!--- \r\n");
		} else {
			xil_printf("<---APPx's Message is sent to TA_FPGA--- \r\n");
		}

	Status = XIpiPsu_PollForAck(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,TIMEOUT_COUNT);

		if (Status == !XST_SUCCESS) {
			xil_printf("---ACK TIMEOUT: APPx's Message to TA_FPGA Not Acknowledged in Time Alloted (TIMEOUT_COUNT)!--- \r\n");
		} else {
			xil_printf("\n\n\n\n\n\n\n\n\n\n---APPx's Ciphertext is received--- \r\n\n\n");
		}

	usleep(100000);

	XIpiPsu_WriteMessage(InstancePtr,
			DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,
			KeyBuffer_apu,
			TEST_KEY_LEN,
			XIPIPSU_BUF_TYPE_MSG);

	Status = XIpiPsu_TriggerIpi(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask);

		if (Status == !XST_SUCCESS) {
			xil_printf("---APPx's AES Key's chunk_1 is not sent to TA_FPGA!--- \r\n");
		} else {
			xil_printf("<---APPx's AES Key's chunk_1 is sent to TA_FPGA--- \r\n");
		}

	Status = XIpiPsu_PollForAck(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,TIMEOUT_COUNT);


		if (Status == !XST_SUCCESS) {
			xil_printf("---ACK TIMEOUT: APPx's Message to TA_FPGA Not Acknowledged in Time Alloted (TIMEOUT_COUNT)!--- \r\n");
		} else {
			xil_printf("\n\n\n\n\n\n\n\n\n\n---APPx's AES Key's chunk_1 is received --- \r\n\n\n");
		}

	usleep(100000);
//	// New KeyBuffer_apu_1 chunk by chunk data transfer
	XIpiPsu_WriteMessage(InstancePtr,
			DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,
			KeyBuffer_apu_1,
			TEST_KEY_LEN,
			XIPIPSU_BUF_TYPE_MSG);

	Status = XIpiPsu_TriggerIpi(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask);

		if (Status == !XST_SUCCESS) {
			xil_printf("---APPx's AES Key's chunk_2 is not sent to TA_FPGA!--- \r\n");
		} else {
			xil_printf("<---APPx's AES Key's chunk_2 is sent to TA_FPGA--- \r\n");
		}

	Status = XIpiPsu_PollForAck(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,TIMEOUT_COUNT);


		if (Status == !XST_SUCCESS) {
			xil_printf("---ACK TIMEOUT: APPx's Message to TA_FPGA Not Acknowledged in Time Alloted (TIMEOUT_COUNT)!--- \r\n");
		} else {
			xil_printf("\n\n\n\n\n\n\n\n\n\n---APPx's AES Key's chunk_2 is received --- \r\n\n\n");
		}

	usleep(100000);

	XIpiPsu_WriteMessage(InstancePtr,
			DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,
			IvBuffer_apu,
			TEST_IV_LEN,
			XIPIPSU_BUF_TYPE_MSG);

	Status = XIpiPsu_TriggerIpi(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask);

		if (Status == !XST_SUCCESS) {
			xil_printf("---APPx's IV is not sent to TA_FPGA!--- \r\n");
		} else {
			xil_printf("<---APPx's IV is sent to TA_FPGA--- \r\n");
		}

	Status = XIpiPsu_PollForAck(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,TIMEOUT_COUNT);

		if (Status == !XST_SUCCESS) {
			xil_printf("---ACK TIMEOUT: APPx's Message to TA_FPGA Not Acknowledged in Time Alloted (TIMEOUT_COUNT)!--- \r\n");
		} else {
			xil_printf("\n\n\n\n\n\n\n\n\n\n---APPx's IV is received--- \r\n\n\n");
		}

	usleep(100000);

	XIpiPsu_WriteMessage(InstancePtr,
			DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,
			AadBuffer_apu,
			TEST_AAD_LEN,
			XIPIPSU_BUF_TYPE_MSG);

	Status = XIpiPsu_TriggerIpi(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask);

		if (Status == !XST_SUCCESS) {
			xil_printf("---APPx's AAD is not sent to TA_FPGA!--- \r\n");
		} else {
			xil_printf("<---APPx's AAD is sent to TA_FPGA--- \r\n");
		}

	Status = XIpiPsu_PollForAck(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,TIMEOUT_COUNT);

		if (Status == !XST_SUCCESS) {
			xil_printf("---ACK TIMEOUT: APPx's Message to TA_FPGA Not Acknowledged in Time Alloted (TIMEOUT_COUNT)!--- \r\n");
		} else {
			xil_printf("\n\n\n\n\n\n\n\n\n\n---APPx's AAD is received--- \r\n\n\n");
		}

	usleep(100000);

	/*XIpiPsu_WriteMessage(InstancePtr,
			DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,
			CtBuffer_apu,
			TEST_MSG_LEN,
			XIPIPSU_BUF_TYPE_MSG);

	Status = XIpiPsu_TriggerIpi(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask);

		if (Status == !XST_SUCCESS) {
			xil_printf("---APPx's Ciphertext is not sent to TA_FPGA!--- \r\n");
		} else {
			xil_printf("---APPx's Ciphertext is sent to TA_FPGA---> \r\n");
		}

	Status = XIpiPsu_PollForAck(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,TIMEOUT_COUNT);

		if (Status == !XST_SUCCESS) {
			xil_printf("---ACK TIMEOUT: APPx's Message to TA_FPGA Not Acknowledged in Time Alloted (TIMEOUT_COUNT)!--- \r\n");
		} else {
			xil_printf("\n\n\n\n\n\n\n\n\n\n---APPx's Ciphertext is received--- \r\n\n\n");
		}

	usleep(100000);*/

	/*XIpiPsu_WriteMessage(InstancePtr,
			DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,
			TagBuffer_apu,
			TEST_TAG_LEN,
			XIPIPSU_BUF_TYPE_MSG);

	Status = XIpiPsu_TriggerIpi(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask);

		if (Status == !XST_SUCCESS) {
			xil_printf("---APPx's Tag is not sent to TA_FPGA!--- \r\n");
		} else {
			xil_printf("<---APPx's Tag is sent to TA_FPGA--- \r\n");
		}

	Status = XIpiPsu_PollForAck(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXR5_0_CH0_INDEX].Mask,TIMEOUT_COUNT);

		if (Status == !XST_SUCCESS) {
			xil_printf("---ACK TIMEOUT: APPx's Message to TA_FPGA Not Acknowledged in Time Alloted (TIMEOUT_COUNT)!--- \r\n");
		} else {
			xil_printf("\n\n\n\n\n\n\n\n\n\n---APPx's Tag is received--- \r\n\n\n");
		}*/

	/*XIpiPsu_WriteMessage(InstancePtr,
				DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXA53_0_CH0_INDEX].Mask,
				*MsgPtr,
				MsgLength,
				XIPIPSU_BUF_TYPE_MSG);

	Status = XIpiPsu_TriggerIpi(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXA53_0_CH0_INDEX].Mask);

	if (Status == !XST_SUCCESS) {
		xil_printf("---APPx's Message is not sent to TA_FPGA!--- \r\n");
	} else {
		xil_printf("---APPx's Message is sent to TA_FPGA---> \r\n");
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
	gcm_context ctx;
	int Status = XST_FAILURE;
	size_t key_length_b = 32;
  //size_t msg_length_b = 16;
	size_t iv_length_b = 12;
	size_t aad_length_b = 16;
	size_t tag_length_b = 16;
	uchar *key= (uchar *)malloc(key_length_b);
	uchar *iv= (uchar *)malloc(iv_length_b);
	uchar *aad= (uchar *)malloc(aad_length_b);
	// Define AES Key, IV, and AAD as hexadecimal strings
	const char *Key_ = "7f7168a406e7c1ef0fd47ac922c5ec5f659765fb6aaa048f7056f6c6b5d8513d";
	const char *IV_  = "b8b5e407adc0e293e3e7e991";
	const char *AAD_ = "ff7628f6427fbcef1f3b82b37404e116";

	xil_printf("\n\n\n\n\n\nAPPx [Build: %s %s]\r\n", __DATE__, __TIME__);
	// Define the size of the shift left bitstream
	size_t shift_left_bs_size = 0xF4510;

	// Define memory addresses for plaintext input and encryption output
	const uchar *shift_left_bs_addr =  (uchar *)0x20000000;  // Source plaintext data
	uchar *enc_shift_left_bs_addr = (uchar *)0x30000000;     // Encrypted output buffer
	uchar *tag_shift_left_bs_addr = (uchar *)0x300F4520;     // Authentication tag buffer (AES-GCM)

	Xil_DCacheDisable();


	// Convert Key_ from hex string to byte array (Big-endian to Little-endian)
	for (size_t i = 0; i < key_length_b; i++) {
	    sscanf(Key_ + 2*i, "%2hhx", &key[key_length_b-1-i]); // Convert each 2-character hex to a byte
	    xil_printf("%02X", key[key_length_b - 1 - i]); // Print each converted byte
	}
    xil_printf("\r\n");
	// Convert IV_ from hex string to byte array (Big-endian to Little-endian)
	for (size_t i = 0; i < iv_length_b; i++) {
	    sscanf(IV_ + 2*i, "%2hhx", &iv[iv_length_b-1-i]); // Convert each 2-character hex to a byte
	}
	/*for (size_t i = 0; i < msg_length_b; i++) {
		sscanf(PT_ + 2*i, "%2hhx", &pt[msg_length_b-1-i]);
	}*/
	// Convert AAD_ from hex string to byte array (Big-endian to Little-endian)
	for (size_t i = 0; i < aad_length_b; i++) {
	    sscanf(AAD_ + 2*i, "%2hhx", &aad[aad_length_b-1-i]); // Convert each 2-character hex to a byte
	}
            /* Before gcm initializing we need activate RSA encryption */
	xil_printf("Debug: RSA n, e, d assgiment:\r\n");
	static const byte n[] = {
	    0xC2, 0x0D, 0x04, 0x41, 0x9F, 0x82, 0x0D,
	    0xA2, 0xBB, 0x87, 0xD0, 0x9F, 0x14, 0xBD, 0x51,
	    0xE6, 0x1F, 0x33, 0x72, 0xBB, 0xEB, 0x3C, 0x9D,
	    0x2D, 0x54, 0x67, 0x78, 0x91, 0x29, 0xE8, 0xEA,
	    0xA0, 0x9C, 0xEB, 0xCB, 0x93, 0x02, 0x00, 0xD8,
	    0x37, 0x89, 0x5E, 0x68, 0x7B, 0x08, 0xEB, 0xE3,
	    0xFC, 0xEA, 0xFF, 0x42, 0xD8, 0x84, 0x94, 0x2D,
	    0x5C, 0xB3, 0xB3, 0x13, 0x7C, 0xE4, 0x84, 0x61,
	    0x8F
	};

	static const byte e[] = {0x01, 0x00, 0x01};  // Public exponent (65537)

	static const byte d[] = {
	    0x41, 0xFA, 0x81, 0x06, 0xFB, 0x00, 0x12, 0x38,
	    0xD1, 0x80, 0x65, 0x3B, 0xA1, 0xA6, 0x99, 0x51,
	    0x20, 0x2C, 0xB3, 0x07, 0x17, 0xFD, 0x9D, 0x07,
	    0x8B, 0x9A, 0x3E, 0xD1, 0x44, 0xA9, 0xB0, 0x14,
	    0x62, 0x95, 0x9C, 0x61, 0xF8, 0x35, 0xFD, 0x7F,
	    0x94, 0xE5, 0xFC, 0xF9, 0x17, 0xE6, 0x86, 0xA8,
	    0x78, 0xDC, 0x09, 0xEC, 0xEB, 0x2C, 0x21, 0xE0,
	    0xEA, 0x0E, 0xA9, 0xE2, 0xA9, 0x2A, 0x15, 0x39
	};


    int status;
    RsaKey rsakey;
//    WC_RNG rng;
    byte encrypted[64];  // RSA-512 produces 64-byte ciphertext
    byte chunk1[32];
    byte chunk2[32];

    /* Initialize RSA key */
	xil_printf("Debug: RSA Key Init begins\r\n");
    status = wc_InitRsaKey(&rsakey, NULL);
    if (status != 0) {
        xil_printf("RSA key initialization failed: %d\n", status);
        return status;
    }
	xil_printf("Debug: RSA Key Init ends\r\n");

	xil_printf("Debug: RSA Encyrption begins\r\n");

    /* Set RSA key manually */
    status = mp_read_unsigned_bin(&rsakey.n, n, sizeof(n));
    if (status != 0) {
        xil_printf("Failed to set modulus (n): %d\n", status);
        return status;
    }

	xil_printf("Debug: Set RSA key manually first step\r\n");


    status = mp_read_unsigned_bin(&rsakey.e, e, sizeof(e));
    if (status != 0) {
        xil_printf("Failed to set public exponent (e): %d\n", status);
        return status;
    }
	xil_printf("Debug: Set RSA key manually second step\r\n");

    status = mp_read_unsigned_bin(&rsakey.d, d, sizeof(d));
    if (status != 0) {
        xil_printf("Failed to set private exponent (d): %d\n", status);
        return status;
    }
	xil_printf("Debug: Set RSA key manually third step\r\n");

//	xil_printf("Debug: RNG Init begins\r\n");
//    /* Initialize RNG (needed for encryption) */
//    wc_InitRng(&rng);
//	xil_printf("Debug: RNG Init ends\r\n");
    /* Encrypt the 32-byte AES key using RSA-512 */
	unsigned char padded_key[64] = {0};
	// Copy the 32-byte AES key and manually zero-pad to 64 bytes
	memset(padded_key, 0, 64);  // Fill buffer with zeros
	memcpy(padded_key + 32, key, 32);  // Right-align AES key

//  status = wc_RsaPublicEncrypt(key, 32, encrypted, sizeof(encrypted), &rsakey, &rng);
	word32 encrypted_size = 64;  // Must be a variable, not a constant
	xil_printf("Debug: RSA Encryption started\r\n");
	status = wc_RsaDirect(padded_key, 64, encrypted, &encrypted_size, &rsakey, RSA_PUBLIC_ENCRYPT, NULL);
    if (status < 0) {
        xil_printf("RSA Encryption failed: %d\n", status);
        return -1;
    }
    xil_printf("AES Key Encrypted Successfully!\r\n");

    xil_printf("Encrypted AES Key (Little-endian):");
    for (size_t i = 0; i < 64; i++) {
        xil_printf("%02X", encrypted[i]);
    }
    xil_printf("\r\n");

 /* Divide the 64-byte encrypted key into two 32-byte chunks */
    memcpy(chunk1, encrypted, 32);
    memcpy(chunk2, encrypted + 32, 32);

    /* Print the two 32-byte chunks */
    xil_printf("Encrypted Key Chunk 1: ");
    for (size_t i = 0; i < 32; i++) {
        xil_printf("%02X", chunk1[i]);
    }
    xil_printf("\r\n");

    xil_printf("Encrypted Key Chunk 2: ");
    for (size_t i = 0; i < 32; i++) {
        xil_printf("%02X", chunk2[i]);
    }
    xil_printf("\r\n");

    wc_FreeRsaKey(&rsakey);

	xil_printf("Debug: RSA Encyrption finishes\r\n");

	gcm_initialize();

	gcm_setkey( &ctx, key, (const uint)key_length_b );

	gcm_crypt_and_tag( &ctx, ENCRYPT, iv, iv_length_b, aad, aad_length_b, shift_left_bs_addr, enc_shift_left_bs_addr, shift_left_bs_size, tag_shift_left_bs_addr, tag_length_b);

	xil_printf("APPx Message Content:\r\n");

	MsgBuffer_apu[0] = 1;
	MsgBuffer_apu[1] = (u32)shift_left_bs_size;
	MsgBuffer_apu[2] = (u32)enc_shift_left_bs_addr;
	MsgBuffer_apu[3] = (u32)tag_shift_left_bs_addr;
	xil_printf("Requesting Shift Left Accelerator:      0x%08x\r\n", MsgBuffer_apu[0]);
	xil_printf("Accelerator's Bitstream Size in Bytes:  0x%08x\r\n", MsgBuffer_apu[1]);
	xil_printf("Accelerator's Enc. Bitstream Location:  0x%08x\r\n", MsgBuffer_apu[2]);
	xil_printf("Accelerator's Tag Location:             0x%08x\r\n", MsgBuffer_apu[3]);
	xil_printf("\n");

	memcpy(KeyBuffer_apu, chunk1, key_length_b);
	for (Index = 0; Index < TEST_KEY_LEN; Index++) {
		xil_printf("Key%d: 0x%08x\r\n", Index, KeyBuffer_apu[Index]);
	}
	xil_printf("\n");

	memcpy(KeyBuffer_apu_1, chunk2, key_length_b);
	for (Index = 0; Index < TEST_KEY_LEN; Index++) {
		xil_printf("Key%d: 0x%08x\r\n", Index, KeyBuffer_apu_1[Index]);
	}
	xil_printf("\n");

	memcpy(IvBuffer_apu, iv, iv_length_b);
	for (Index = 0; Index < TEST_IV_LEN; Index++) {
		xil_printf("IV%d: 0x%08x\r\n", Index, IvBuffer_apu[Index]);
	}
	xil_printf("\n");

	memcpy(AadBuffer_apu, aad, aad_length_b);
	for (Index = 0; Index < TEST_AAD_LEN; Index++) {
		xil_printf("AAD%d: 0x%08x\r\n", Index, AadBuffer_apu[Index]);
	}
	xil_printf("\n");


	memcpy(TagBuffer_apu, tag_shift_left_bs_addr, tag_length_b);
	for (Index = 0; Index < TEST_TAG_LEN; Index++) {
		xil_printf("TAG%d: 0x%08x\r\n", Index, TagBuffer_apu[Index]);
	}

	free(key);
	free(iv);
	free(aad);
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
	} while (msg_count == 0);

	if (MsgBuffer_rpu[0] == GCM_AUTH_FAILURE)
			xil_printf("Decryption & Authentication Failed!!\r\n");
	else xil_printf("Decryption & Authentication Successful!!\r\n");

	switch(MsgBuffer_rpu[1]) {
		case 7  : xil_printf("Accelerator is successfully configured\r\n");
			break;
		case 6  : xil_printf("Accelerator is being reset\r\n");
			break;
		case 5  : xil_printf("Software start-up step\r\n");
			break;
		case 4  : xil_printf("Loading new Accelerator\r\n");
			break;
		case 2  : xil_printf("Software Shutdown\r\n");
			break;
		case 1  : xil_printf("Hardware Shutdown\r\n");
			break;
		case 0  : xil_printf("No Accelerator Loaded\r\n");
			break;
		default : break;
	}

	switch(MsgBuffer_rpu[2]) {
		case (0x80) : xil_printf("vFPGA is in Shutdown Mode!\r\n");
	    	break;
		case (0xC0) : xil_printf("Incorrect compressed bit format is Found!\r\n");
			break;
		case (0xB8) : xil_printf("Incorrect compressed bit size is Found!\r\n");
	        break;
		case (0x78) : xil_printf("Unknown Error!\r\n");
			break;
		case (0xB0) : xil_printf("Lost + Fetch Error!\r\n");
	    	break;
		case (0xA8) : xil_printf("BS + Fetch Error!\r\n");
	    	break;
	    case (0xA0) : xil_printf("Fetch Error!\r\n");
	    	break;
	    case (0x98) : xil_printf("Lost Error!\r\n");
	        break;
	    case (0x90) : xil_printf("Bitstream Error!\r\n");
	        break;
	    case (0x88) : xil_printf("Bad Config Error!\r\n");
	        break;
	    default     : break;
	}
	xil_printf ("Configuration Time = %d[us]\r\n", (MsgBuffer_rpu[3])*5/1000);


	return Status;

}
