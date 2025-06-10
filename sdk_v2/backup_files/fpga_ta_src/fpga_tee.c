#include <stdio.h>
#include "platform.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xil_io.h"
#include "xtmrctr.h"
#include "xprc.h"
#include "fpga_tee.h"

#include "stdlib.h"
#include "xil_types.h"
#include <unistd.h>
#include <string.h>

#include "xparameters.h"
#include "xil_exception.h"
#include "xil_cache.h"
#include "xscugic.h"
#include "xipipsu.h"
#include "xipipsu_hw.h"
#include "gcm.h"

/************************* Test Configuration ********************************/
/* IPI device ID to use for this test */
#define TEST_CHANNEL_ID	XPAR_XIPIPSU_0_DEVICE_ID
/* Test message length in words. Max is 8 words (32 bytes) */
#define TEST_MSG_LEN	4
#define TEST_KEY_LEN	8
#define TEST_IV_LEN		3
#define TEST_AAD_LEN	4
#define TEST_TAG_LEN	4

#define TEST_MSG_LEN_B	16
#define TEST_KEY_LEN_B	32
#define TEST_IV_LEN_B	12
#define TEST_AAD_LEN_B	16
#define TEST_TAG_LEN_B	16
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
u32 CtBuffer_rpu[TEST_MSG_LEN];
u32 KeyBuffer_rpu[TEST_KEY_LEN];
u32 IvBuffer_rpu[TEST_IV_LEN];
u32 TagBuffer_rpu[TEST_TAG_LEN];
u32 AadBuffer_rpu[TEST_AAD_LEN];

u32 msg_count = 0;
/**
 * Interrupt Handler :
 * -Polls for each of the valid sources
 * -Checks if there is a message
 * -Reads the message
 */
void IpiIntrHandler(void *XIpiPsuPtr)
{

	u32 IpiSrcMask; /**< Holds the IPI status register value */
	u32 Index;

	//u32 TmpBufPtr[TEST_MSG_LEN] = { 0 }; /**< Holds the received Message, later inverted and sent back as response*/
	u32 Status;
	u32 SrcIndex;
	XIpiPsu *InstancePtr = (XIpiPsu *) XIpiPsuPtr;

	xil_printf("\n\n-----Enter Interrupt Handler\r\n");

	Xil_AssertVoid(InstancePtr!=NULL);

	IpiSrcMask = XIpiPsu_GetInterruptStatus(InstancePtr);

	/* Poll for each source and send Response (Response = ~Msg) */

	for (SrcIndex = 0U; SrcIndex < InstancePtr->Config.TargetCount;
			SrcIndex++) {

		if (IpiSrcMask & InstancePtr->Config.TargetList[SrcIndex].Mask) {

			/*  Read Incoming Message Buffer Corresponding to Source CPU */
			/*Status = XIpiPsu_ReadMessage(InstancePtr,
					InstancePtr->Config.TargetList[SrcIndex].Mask, TmpBufPtr,
					TEST_MSG_LEN, XIPIPSU_BUF_TYPE_MSG);*/

			/*if (Status == XST_FAILURE) {
				xil_printf("     Message is not Received from APPx:\r\n");
			} else {
				msg_count = msg_count + 1;
			}*/
			if (msg_count == 0) {
				Status = XIpiPsu_ReadMessage(InstancePtr,
						InstancePtr->Config.TargetList[SrcIndex].Mask, KeyBuffer_rpu,
						TEST_KEY_LEN, XIPIPSU_BUF_TYPE_MSG);
				if (Status == XST_FAILURE) {
					xil_printf("     Key is not Received from APPx:\r\n");
				} else {
					msg_count = msg_count + 1;
					xil_printf("     Key Received from APPx:\r\n");
					for (Index = 0; Index < TEST_KEY_LEN; Index++)
						xil_printf("Key%d: 0x%08x\r\n", Index, KeyBuffer_rpu[Index]);
				}
			} else if (msg_count == 1) {
				Status = XIpiPsu_ReadMessage(InstancePtr,
						InstancePtr->Config.TargetList[SrcIndex].Mask, IvBuffer_rpu,
						TEST_IV_LEN, XIPIPSU_BUF_TYPE_MSG);
				if (Status == XST_FAILURE) {
					xil_printf("     IV is not Received from APPx:\r\n");
				} else {
					msg_count = msg_count + 1;
					xil_printf("     IV Received from APPx:\r\n");
				for (Index = 0; Index < TEST_IV_LEN; Index++)
					xil_printf("IV%d: 0x%08x\r\n", Index, IvBuffer_rpu[Index]);
				}
			} else if (msg_count == 2) {
				Status = XIpiPsu_ReadMessage(InstancePtr,
						InstancePtr->Config.TargetList[SrcIndex].Mask, AadBuffer_rpu,
						TEST_AAD_LEN, XIPIPSU_BUF_TYPE_MSG);
				if (Status == XST_FAILURE) {
					xil_printf("     AAD is not Received from APPx:\r\n");
				} else {
					msg_count = msg_count + 1;
					xil_printf("     AAD Received from APPx:\r\n");
				for (Index = 0; Index < TEST_AAD_LEN; Index++)
					xil_printf("AAD%d: 0x%08x\r\n", Index, AadBuffer_rpu[Index]);
				}
			} else if (msg_count == 3) {
				Status = XIpiPsu_ReadMessage(InstancePtr,
						InstancePtr->Config.TargetList[SrcIndex].Mask, CtBuffer_rpu,
						TEST_MSG_LEN, XIPIPSU_BUF_TYPE_MSG);
				if (Status == XST_FAILURE) {
					xil_printf("     CipherText is not Received from APPx:\r\n");
				} else {
					msg_count = msg_count + 1;
					xil_printf("     CipherText Received from APPx:\r\n");
				for (Index = 0; Index < TEST_MSG_LEN; Index++)
					xil_printf("CipherText%d: 0x%08x\r\n", Index, CtBuffer_rpu[Index]);
				}
			} else if (msg_count == 4) {
				Status = XIpiPsu_ReadMessage(InstancePtr,
						InstancePtr->Config.TargetList[SrcIndex].Mask, TagBuffer_rpu,
						TEST_TAG_LEN, XIPIPSU_BUF_TYPE_MSG);
				if (Status == XST_FAILURE) {
					xil_printf("     Tag is not Received from APPx:\r\n");
				} else {
					msg_count = msg_count + 1;
					xil_printf("     Tag Received from APPx:\r\n");
				for (Index = 0; Index < TEST_TAG_LEN; Index++)
					xil_printf("TAG%d: 0x%08x\r\n", Index, TagBuffer_rpu[Index]);
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


static XStatus DoIpiTest(XIpiPsu *InstancePtr)
{

	XIpiPsu_Config *DestCfgPtr;
	u32 Status = 0;
	DestCfgPtr = XIpiPsu_LookupConfig(TEST_CHANNEL_ID);

	/**
	 * Send a Message to TEST_TARGET and WAIT for ACK
	 */

	XIpiPsu_WriteMessage(InstancePtr,
			DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXA53_0_CH0_INDEX].Mask,
			MsgBuffer_apu,
			TEST_MSG_LEN,
			XIPIPSU_BUF_TYPE_MSG);


	Status = XIpiPsu_TriggerIpi(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXA53_0_CH0_INDEX].Mask);

	if (Status == !XST_SUCCESS) {
		xil_printf("---FPGA_TA Message to APPx Not Sent!--- \r\n");
	} else {
		xil_printf("<---FPGA_TA Message to APPx Sent--- \r\n");
	}

	Status = XIpiPsu_PollForAck(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXA53_0_CH0_INDEX].Mask,TIMEOUT_COUNT);

		if (Status == !XST_SUCCESS) {
			xil_printf("---ACK TIMEOUT: FPGA_TA Message to APPx Not Acknowledged in Time Alloted (TIMEOUT_COUNT)!--- \r\n");
		} else {
			xil_printf("\n\n\n\n\n\n\n\n\n\n\n\n---FPGA_TA Message to APPx Received--- \r\n\n");
		}

	return XST_SUCCESS;
}

int main() {

	//SEM
	  usleep(700000);
	//SEM

	XIpiPsu_Config *CfgPtr;
	u32 Index;
	int ret;
	gcm_context ctx;            // includes the AES context structure

	int Status = XST_FAILURE;
	xil_printf("\n\n\nFPGA_TA [Build: %s %s]\r\n", __DATE__, __TIME__);

	Xil_DCacheDisable();

	/* Look Up the config data */
	CfgPtr = XIpiPsu_LookupConfig(TEST_CHANNEL_ID);

	/* Init with the Cfg Data */
	XIpiPsu_CfgInitialize(&IpiInst, CfgPtr, CfgPtr->BaseAddress);

	/* Setup the GIC */
	SetupInterruptSystem(&GicInst, &IpiInst,65);

	/* Enable reception of IPIs from all CPUs */
	XIpiPsu_InterruptEnable(&IpiInst, XIPIPSU_ALL_MASK);

	/* Clear Any existing Interrupts */
	XIpiPsu_ClearInterruptStatus(&IpiInst, XIPIPSU_ALL_MASK);

	do {
		__asm("wfi");
	} while (msg_count < 5);

	gcm_initialize();
	gcm_setkey( &ctx, (uchar*)&KeyBuffer_rpu[0], (const uint)TEST_KEY_LEN_B );
	ret = gcm_auth_decrypt( &ctx, (uchar*)&IvBuffer_rpu[0], TEST_IV_LEN_B, (uchar*)&AadBuffer_rpu[0], TEST_AAD_LEN_B,
		(uchar*)&CtBuffer_rpu[0], (uchar*)&MsgBuffer_apu[0], TEST_MSG_LEN_B, (uchar*)&TagBuffer_rpu[0], TEST_TAG_LEN_B);
	if (ret == GCM_AUTH_FAILURE)
		xil_printf("Decryption & Authentication Failed!!\n\r");
	msg_count = 0;
	xil_printf("APU Message Content:\n\r");
	for (Index = 0; Index < TEST_MSG_LEN; Index++) {
		xil_printf("W%d: 0x%08x\r\n", Index, MsgBuffer_apu[Index]);
	}

	//SEM
	usleep(700000);
	//SEM

	/* Call the test routine */
	Status = DoIpiTest(&IpiInst);

	//do {
		/**
		 * Do Nothing
		 * We need to loop on to receive IPIs and respond to them
		 */
		//__asm("wfi");
	//} while (1);

	/* Control never reaches here */
	return Status;

}

