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


#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
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
#define TIMEOUT_COUNT 70000

/*****************************************************************************/

/* Global Instances of GIC and IPI devices */
XScuGic GicInst;
XIpiPsu IpiInst;

/* Buffers to store Test Data */
u32 MsgBuffer_rpu[TEST_MSG_LEN];
u32 KeyBuffer_rpu[TEST_KEY_LEN];
u32 KeyBuffer_rpu_1[TEST_KEY_LEN];
u32 IvBuffer_rpu[TEST_IV_LEN];
u32 TagBuffer_rpu[TEST_TAG_LEN];
u32 AadBuffer_rpu[TEST_AAD_LEN];
u32 MsgBuffer_apu[TEST_MSG_LEN];
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

	xil_printf("-----Enter Interrupt Handler\r\n");

	Xil_AssertVoid(InstancePtr!=NULL);

	IpiSrcMask = XIpiPsu_GetInterruptStatus(InstancePtr);

	/* Poll for each source and send Response (Response = ~Msg) */

	for (SrcIndex = 0U; SrcIndex < InstancePtr->Config.TargetCount;
			SrcIndex++) {

		if (IpiSrcMask & InstancePtr->Config.TargetList[SrcIndex].Mask) {

			/*  Read Incoming Message Buffer Corresponding to Source CPU */
			if (msg_count == 0) {
				Status = XIpiPsu_ReadMessage(InstancePtr,
						InstancePtr->Config.TargetList[SrcIndex].Mask, MsgBuffer_rpu,
						TEST_MSG_LEN, XIPIPSU_BUF_TYPE_MSG);
				if (Status == XST_FAILURE) {
					xil_printf("Message is not Received from APPx:\r\n");
				} else {
					msg_count = msg_count + 1;
					xil_printf("Message Received from APPx:\r\n");
					for (Index = 0; Index < TEST_MSG_LEN; Index++)
						xil_printf("Message%d: 0x%08x\r\n", Index, MsgBuffer_rpu[Index]);
				}
			}
			else if (msg_count == 1) {
				Status = XIpiPsu_ReadMessage(InstancePtr,
						InstancePtr->Config.TargetList[SrcIndex].Mask, KeyBuffer_rpu,
						TEST_KEY_LEN, XIPIPSU_BUF_TYPE_MSG);
				if (Status == XST_FAILURE) {
					xil_printf("AES Key's chunk_1 is not Received from APPx:\r\n");
				} else {
					msg_count = msg_count + 1;
					xil_printf("AES Key's chunk_1 Received from APPx:\r\n");
					for (Index = 0; Index < TEST_KEY_LEN; Index++)
						xil_printf("Key%d: 0x%08x\r\n", Index, KeyBuffer_rpu[Index]);
				}

			}
			else if (msg_count == 2) {
				Status = XIpiPsu_ReadMessage(InstancePtr,
						InstancePtr->Config.TargetList[SrcIndex].Mask, KeyBuffer_rpu_1,
						TEST_KEY_LEN, XIPIPSU_BUF_TYPE_MSG);
				if (Status == XST_FAILURE) {
					xil_printf("AES Key's chunk_2 is not Received from APPx:\r\n");
				}
				else {
					msg_count = msg_count + 1;
					xil_printf("AES Key's chunk_2 Received from APPx:\r\n");
					for (Index = 0; Index < TEST_KEY_LEN; Index++)
						xil_printf("Key%d: 0x%08x\r\n", Index, KeyBuffer_rpu_1[Index]);
				}

			}

			else if (msg_count == 3) {
				Status = XIpiPsu_ReadMessage(InstancePtr,
						InstancePtr->Config.TargetList[SrcIndex].Mask, IvBuffer_rpu,
						TEST_IV_LEN, XIPIPSU_BUF_TYPE_MSG);
				if (Status == XST_FAILURE) {
					xil_printf("IV is not Received from APPx:\r\n");
				} else {
					msg_count = msg_count + 1;
					xil_printf("IV Received from APPx:\r\n");
				for (Index = 0; Index < TEST_IV_LEN; Index++)
					xil_printf("IV%d: 0x%08x\r\n", Index, IvBuffer_rpu[Index]);
				}
			} else if (msg_count == 4) {
				Status = XIpiPsu_ReadMessage(InstancePtr,
						InstancePtr->Config.TargetList[SrcIndex].Mask, AadBuffer_rpu,
						TEST_AAD_LEN, XIPIPSU_BUF_TYPE_MSG);
				if (Status == XST_FAILURE) {
					xil_printf("AAD is not Received from APPx:\r\n");
				} else {
					msg_count = msg_count + 1;
					xil_printf("AAD Received from APPx:\r\n");
				for (Index = 0; Index < TEST_AAD_LEN; Index++)
					xil_printf("AAD%d: 0x%08x\r\n", Index, AadBuffer_rpu[Index]);
				}
			}
//			else if (msg_count == 4) {
//				Status = XIpiPsu_ReadMessage(InstancePtr,
//						InstancePtr->Config.TargetList[SrcIndex].Mask, KeyBuffer_rpu_1,
//						TEST_KEY_LEN, XIPIPSU_BUF_TYPE_MSG);
//				if (Status == XST_FAILURE) {
//					xil_printf("Key2 is not Received from APPx:\r\n");
//				}
//				else {
//					msg_count = msg_count + 1;
//					xil_printf("Key2 Received from APPx:\r\n");
//					for (Index = 0; Index < TEST_KEY_LEN; Index++)
//						xil_printf("Key%d: 0x%08x\r\n", Index, KeyBuffer_rpu_1[Index]);
//				}
//
//			}
			/*else if (msg_count == 4) {
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
		xil_printf("---TA_FPGA Message to APPx Not Sent!--- \r\n");
	} else {
		xil_printf("---TA_FPGA Message to APPx Sent---> \r\n");
	}

	Status = XIpiPsu_PollForAck(InstancePtr, DestCfgPtr->TargetList[XPAR_XIPIPS_TARGET_PSU_CORTEXA53_0_CH0_INDEX].Mask,TIMEOUT_COUNT);

		if (Status == !XST_SUCCESS) {
			xil_printf("---ACK TIMEOUT: TA_FPGA Message to APPx Not Acknowledged in Time Alloted (TIMEOUT_COUNT)!--- \r\n");
		} else {
			xil_printf("\n\n\n\n\n\n\n\n\n\n\n\n---TA_FPGA Message to APPx Received--- \r\n\n");
		}

	return XST_SUCCESS;
}

int main() {

	//SEM
	  //usleep(700000);
	//SEM

	XIpiPsu_Config *CfgPtr;
	//u32 Index;
	int ret;
	gcm_context ctx;            // includes the AES context structure
	size_t iv_length_b = 12;
	size_t aad_length_b = 16;
	size_t tag_length_b = 16;
	size_t bitstream_size;
	uchar *bitstream_addr;
	uchar *enc_bitstream_addr;
	uchar *tag_bitstream_addr;
	int Status = XST_FAILURE;
	u32 Option = 1;

    //For DFX Controller
    u32 prc_init;
    u32 prc_status;
    u32 prc_status_last;
    u32 prc_status_state;
    u32 prc_status_err;
    XPrc Prc;
    XPrc_Config *XPrcCfgPtr;

    u32 rm_loading    = 0;
    u32 shift_loading = 0;
    u32 count_loading = 0;

    //For Timer
    u32 timer_init;
    u32 timer_start_shift;
    u32 timer_start_count;
    u32 timer_end_shift;
    u32 timer_end_count;
    u32 timer_skip = 0;
    XTmrCtr TimerCounter; /* The instance of the Tmrctr Device */
    XTmrCtr *TmrCtrInstancePtr = &TimerCounter;

    // For PCAP
    u32 pcap_ctrl_reg;
    u32 mask_value = 0x00000001;

    init_platform();

	xil_printf("\n\n\nTA_FPGA [Build: %s %s]\r\n", __DATE__, __TIME__);

	Xil_DCacheDisable();

	//Timer Setting
	timer_init = XTmrCtr_Initialize(TmrCtrInstancePtr, TMRCTR_DEVICE_ID);
	if (timer_init != XST_SUCCESS) {
		xil_printf("Timer Initialization Fails\r\n");
	    return XST_FAILURE;
	}

	XTmrCtr_SetOptions(TmrCtrInstancePtr, TIMER_COUNTER_0, XTC_CAPTURE_MODE_OPTION+XTC_AUTO_RELOAD_OPTION);
	XTmrCtr_Start     (TmrCtrInstancePtr, TIMER_COUNTER_0);
	XTmrCtr_SetOptions(TmrCtrInstancePtr, TIMER_COUNTER_1, XTC_CAPTURE_MODE_OPTION+XTC_AUTO_RELOAD_OPTION);
	XTmrCtr_Start     (TmrCtrInstancePtr, TIMER_COUNTER_1);
	//

	//DFX Controller Driver initialize
	XPrcCfgPtr = XPrc_LookupConfig(XDFXC_DEVICE_ID);
	if (NULL == XPrcCfgPtr) {
		return XST_FAILURE;
	}

	prc_init = XPrc_CfgInitialize(&Prc, XPrcCfgPtr, XPrcCfgPtr->BaseAddress);
	//xil_printf("prc_init = 0x%08X\r\n", prc_init);
	//xil_printf("BaseAddress = 0x%08X\r\n", XPrcCfgPtr->BaseAddress);
	if (prc_init != XST_SUCCESS) {
		return XST_FAILURE;
	}
	//

	//DFX Controller setting
	XPrc_SendShutdownCommand(&Prc, XDFXC_VS_SHIFT_ID);
	while(XPrc_IsVsmInShutdown(&Prc, XDFXC_VS_SHIFT_ID)==XPRC_SR_SHUTDOWN_OFF);
	XPrc_SendShutdownCommand(&Prc, XDFXC_VS_COUNT_ID);
	while(XPrc_IsVsmInShutdown(&Prc, XDFXC_VS_COUNT_ID)==XPRC_SR_SHUTDOWN_OFF);

	XPrc_SetBsSize   (&Prc, XDFXC_VS_SHIFT_ID, XDFXC_VS_SHIFT_RM_SHIFT_LEFT_ID,  PARTIAL_SHIFT_LEFT_RM_SIZE);
	XPrc_SetBsSize   (&Prc, XDFXC_VS_SHIFT_ID, XDFXC_VS_SHIFT_RM_SHIFT_RIGHT_ID, PARTIAL_SHIFT_RIGHT_RM_SIZE);
	XPrc_SetBsSize   (&Prc, XDFXC_VS_COUNT_ID, XDFXC_VS_COUNT_RM_COUNT_UP_ID,    PARTIAL_COUNT_UP_RM_SIZE);
	XPrc_SetBsSize   (&Prc, XDFXC_VS_COUNT_ID, XDFXC_VS_COUNT_RM_COUNT_DOWN_ID,  PARTIAL_COUNT_DOWN_RM_SIZE);
	XPrc_SetBsAddress(&Prc, XDFXC_VS_SHIFT_ID, XDFXC_VS_SHIFT_RM_SHIFT_LEFT_ID,  PARTIAL_DDR_SHIFT_LEFT_ADDR);
	XPrc_SetBsAddress(&Prc, XDFXC_VS_SHIFT_ID, XDFXC_VS_SHIFT_RM_SHIFT_RIGHT_ID, PARTIAL_DDR_SHIFT_RIGHT_ADDR);
	XPrc_SetBsAddress(&Prc, XDFXC_VS_COUNT_ID, XDFXC_VS_COUNT_RM_COUNT_UP_ID,    PARTIAL_DDR_COUNT_UP_ADDR);
	XPrc_SetBsAddress(&Prc, XDFXC_VS_COUNT_ID, XDFXC_VS_COUNT_RM_COUNT_DOWN_ID,  PARTIAL_DDR_COUNT_DOWN_ADDR);


    XPrc_SendRestartWithNoStatusCommand(&Prc, XDFXC_VS_SHIFT_ID);
	while(XPrc_IsVsmInShutdown(&Prc, XDFXC_VS_SHIFT_ID)==XPRC_SR_SHUTDOWN_ON);
	XPrc_SendRestartWithNoStatusCommand(&Prc, XDFXC_VS_COUNT_ID);
	while(XPrc_IsVsmInShutdown(&Prc, XDFXC_VS_COUNT_ID)==XPRC_SR_SHUTDOWN_ON);


	//xil_printf("\n\nDeactivating PCAP Register Access\r\n");
	pcap_ctrl_reg = Xil_In32(0xFFCA3008); // read PCAP_CTRL
	pcap_ctrl_reg &= ~( 1 << 0 ); // clear PCAP_CTRL, bit 0 [ICAP]
	Xil_Out32(0xFFCA3008, pcap_ctrl_reg); // write PCAP_CTRL

	pcap_ctrl_reg = Xil_In32(0xFFCA3008); // read DATA_3 (GPIO) Register
	pcap_ctrl_reg = mask_value & pcap_ctrl_reg;

	/*if ( (pcap_ctrl_reg != 0x00000000) ) {
		xil_printf("\n\rError: PCAP is still enabled! PCAP_CTRL[0] was not cleared (0x%08X). To enable ICAP, Bit[0] should be set to 0.\n\r", pcap_ctrl_reg);
	} else {
		xil_printf("PCAP disabled, ICAP is enabled\n\r");
	}*/

	print ("------------------ Menu ------------------\n\r");
	print ("    1: Load Accelerator on vFPGA_1\n\r");
	print ("    2: Load Accelerator on vFPGA_2\n\r");
	//print ("    3: Load Accelerator (Count Up) on vFPGA_2\n\r");
	//print ("    4: Load Accelerator (Count Down) on vFPGA_2\n\r");

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
	} while (msg_count < 4);

	Option = (u32)MsgBuffer_rpu[0];
	bitstream_size = (size_t)MsgBuffer_rpu[1];
	enc_bitstream_addr = (uchar *)MsgBuffer_rpu[2];
	tag_bitstream_addr = (uchar *)MsgBuffer_rpu[3];

	switch (Option) {
		case 1: bitstream_addr =  (uchar *)0x80800000;
			xil_printf("Loading Shift Left on vFPGA 1...\n\r");
			break;
		case 2: bitstream_addr =  (uchar *)0x80c00000;
			xil_printf("Loading Shift Right on vFPGA 1...\n\r");
			break;
		case 3: bitstream_addr =  (uchar *)0x80400000;
		xil_printf("Loading Count Up on vFPGA 2...\n\r");
			break;
		case 4: bitstream_addr =  (uchar *)0x80000000;
		xil_printf("Loading Count Down on vFPGA 2...\n\r");
			break;
		default: //bitstream_addr =  (uchar *)0x80800000;
			break;
    }

	xil_printf("Reading Encrypted Bitstream from Location %p ...\n\r", (void *)enc_bitstream_addr);
	xil_printf("Reading Bitstream Tag from Location %p ...\n\r", (void *)tag_bitstream_addr);
	xil_printf("Decrypting & Verifying the Bitstream...\n\r");
	             /* Before gcm initializing we need activate RSA encryption */
	xil_printf("Debug: RSA n, e, d assgiment:\n\r");
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
    WC_RNG rng;
    byte encrypted_key [64];
    byte raw_decrypted_key [64];
    byte decrypted_key [32];

	/* Initialize RSA Key */
	wc_InitRsaKey(&rsakey, NULL);
	xil_printf("Debug: RSA Encyrption begins\r\n");
    /* Set RSA key manually */
    status = mp_read_unsigned_bin(&rsakey.n, n, sizeof(n));
    if (status != 0) {
        xil_printf("Failed to set modulus (n): %d\n", status);
        return status;
    }

    status = mp_read_unsigned_bin(&rsakey.e, e, sizeof(e));
    if (status != 0) {
        xil_printf("Failed to set public exponent (e): %d\n", status);
        return status;
    }

    status = mp_read_unsigned_bin(&rsakey.d, d, sizeof(d));
    if (status != 0) {
        xil_printf("Failed to set private exponent (d): %d\n", status);
        return status;
    }

    xil_printf("Hardcoded key imported successfully!\r\n");
//    /* Initialize RNG (needed for encryption) */
    wc_InitRng(&rng);
	/* Reconstruct Full 64-byte Encrypted Key */
    xil_printf("chunk1:");
    for (size_t i = 0; i < 32; i++) {
        xil_printf("%02X", KeyBuffer_rpu[i]);
    }
    xil_printf("\r\n");
    xil_printf("chunk2:");
    for (size_t i = 0; i < 32; i++) {
        xil_printf("%02X", KeyBuffer_rpu_1[i]);
    }
    xil_printf("\r\n");
	memcpy(encrypted_key, KeyBuffer_rpu, 32);
	memcpy(encrypted_key + 32, KeyBuffer_rpu_1, 32);

    xil_printf("Reconstructed Full Key:");
    for (size_t i = 0; i < 64; i++) {
        xil_printf("%02X", encrypted_key[i]);
    }
    xil_printf("\r\n");

	/* Decrypt the RSA-512 Encrypted Key to get back the AES Key */
//	status = wc_RsaPrivateDecrypt(encrypted_key, 64, decrypted_key, 32 , &rsakey);
    word32 decrypted_size = 64;
    status = wc_RsaDirect(encrypted_key, 64, raw_decrypted_key, &decrypted_size, &rsakey, RSA_PRIVATE_DECRYPT, &rng);
	if (status < 0) {
	    printf("RSA Decryption failed: %d\r\n", status);
     return -1;
	}

	/* Print Reconstructed AES Key */
	printf("Decrypted AES Key: (Raw 64-byte output) ");
	for (size_t i = 0; i < 64; i++) {
	    printf("%02X", raw_decrypted_key[i]);  // Print as Hex
	}
	printf("\n");

	memcpy(decrypted_key, raw_decrypted_key + 32, 32);  // Extract the actual AES key
	printf("Decrypted AES Key:");
	for (size_t i = 0; i < 32; i++) {
	    printf("%02X", decrypted_key[i]);  // Print as Hex
	}
	printf("\n");

	wc_FreeRsaKey(&rsakey);
	xil_printf("Debug: RSA Encyrption finishes\r\n");
	gcm_initialize();
	gcm_setkey( &ctx, (uchar*)&decrypted_key[0], (const uint)TEST_KEY_LEN_B );
	ret = gcm_auth_decrypt( &ctx, (uchar*)&IvBuffer_rpu[0], iv_length_b, (uchar*)&AadBuffer_rpu[0], aad_length_b,
			enc_bitstream_addr, bitstream_addr, bitstream_size, tag_bitstream_addr, tag_length_b);
	msg_count = 0;
    MsgBuffer_apu [0] = ret;
    printf("Writing Decrypted Bitstream to Location %p ... \n\r", (void *)bitstream_addr);
    /*if (ret == GCM_AUTH_FAILURE)
		xil_printf("Decryption & Authentication Failed!!\n\r");
	else xil_printf("Decryption & Authentication Successful!!\n\r");*/

    switch (Option) {
		case 1 :
			//print("Loading Shift Left on vFPGA 1...\n");
		    if (XPrc_IsSwTriggerPending(&Prc, XDFXC_VS_SHIFT_ID, NULL)==XPRC_NO_SW_TRIGGER_PENDING) {
		    	printf ("Starting Reconfiguration of Shift Left on vFPGA 1...\n\r");
		    	XPrc_SendSwTrigger(&Prc, XDFXC_VS_SHIFT_ID, XDFXC_VS_SHIFT_RM_SHIFT_LEFT_ID);
		    }
		    shift_loading=1;
		    rm_loading=1;
		    count_loading=0;
		    break;

		case 2 :
		   	//print("Loading Shift Right on vFPGA 1...\n\r");
		    if (XPrc_IsSwTriggerPending(&Prc, XDFXC_VS_SHIFT_ID, NULL)==XPRC_NO_SW_TRIGGER_PENDING) {
		        print ("Starting Reconfiguration of Shift Right on vFPGA 1...\n\r");
		        XPrc_SendSwTrigger(&Prc, XDFXC_VS_SHIFT_ID, XDFXC_VS_SHIFT_RM_SHIFT_RIGHT_ID);
		    }
		    shift_loading=1;
		    rm_loading=1;
		    count_loading=0;
		    break;

		case 3 :
			//print("Loading Count Up on vFPGA 2...\n\r");
		    if (XPrc_IsSwTriggerPending(&Prc, XDFXC_VS_COUNT_ID, NULL)==XPRC_NO_SW_TRIGGER_PENDING) {
		    	print ("Starting Reconfiguration of Count Up on vFPGA 2...\n\r");
		        XPrc_SendSwTrigger(&Prc, XDFXC_VS_COUNT_ID, XDFXC_VS_COUNT_RM_COUNT_UP_ID);
		    }
		    count_loading=1;
		    rm_loading=1;
		    shift_loading=0;
		    break;

		case 4 :
			//print("Loading Count Down on vFPGA 2...\n\r");
		    if (XPrc_IsSwTriggerPending(&Prc, XDFXC_VS_COUNT_ID, NULL)==XPRC_NO_SW_TRIGGER_PENDING) {
		    	print ("Starting Reconfiguration of Count Down on vFPGA 2...\n\r");
		        XPrc_SendSwTrigger(&Prc, XDFXC_VS_COUNT_ID, XDFXC_VS_COUNT_RM_COUNT_DOWN_ID);
		    }
		    count_loading=1;
		    rm_loading=1;
		    shift_loading=0;
		    break;
		default:
			rm_loading = 0;
		    break;
	}
	timer_start_shift = XTmrCtr_GetValue(TmrCtrInstancePtr, TIMER_COUNTER_0);
	timer_start_count = XTmrCtr_GetValue(TmrCtrInstancePtr, TIMER_COUNTER_1);
	while (rm_loading) {
		if (shift_loading==1)
			prc_status=XPrc_ReadStatusReg(&Prc, XDFXC_VS_SHIFT_ID);
		else prc_status=XPrc_ReadStatusReg(&Prc, XDFXC_VS_COUNT_ID);
		prc_status_state    =prc_status&0x07;
		prc_status_err      =prc_status&0xf8;

		switch(prc_status_err) {
			case (0x80) : //print("vFPGA is in Shutdown Mode!\n\r");
		    	//print("Set vFPGA Active mode to reconfig!\n\r\n\r");
		    	timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
		        break;
			case (0xC0) : //print("Incorrect compressed bit format is Found!\n\r");
				//print("Set vFPGA Active mode and reconfig with correct bit!\n\r\n\r");
				timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
				break;
			case (0xB8) : //print("Incorrect compressed bit size is Found!\n\r");
				//print("Set vFPGA Active mode and reconfig with correct bit!\n\r\n\r");
				timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
		        break;
			case (0x78) : //print("Unknown Error!\n\r");
				timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
				break;
			case (0xB0) : //print("Lost + Fetch Error!\n\r");
		    	timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
		    	break;
			case (0xA8) : //print("BS + Fetch Error!\n\r");
				timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
		    	break;
		    case (0xA0) : //print("Fetch Error!\n\r");
		    	timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
		    	break;
		    case (0x98) : //print("Lost Error!\n\r");
		    	timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
		        break;
		    case (0x90) : //print("Bitstream Error!\n\r");
		    	timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
		        break;
		    case (0x88) : //print("Bad Config Error!\n\r");
		    	timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
		        break;
		    default     : break;
		}
		//while (prc_status_state != prc_status_last_state) {
		while (prc_status != prc_status_last) {
			switch(prc_status_state) {
		    	case 7  : //print("  Accelerator is successfully loaded\n\r");
                        rm_loading=0; break;
		        case 6  : //print("  Accelerator is being reset\n\r");
                        break;
		        case 5  : //print("  Software start-up step\n\r");
                        break;
		        case 4  : //print("  Loading new Accelerator\n\r");
                        break;
		        case 2  : //print("  Software Shutdown\n\r");
                        break;
		        case 1  : //print("  Hardware Shutdown\n\r");
                        break;
		        case 0  : //print("  No Accelerator Loaded\n\r");
                        break;
		        default : break;
			}
		    prc_status_last = prc_status;
		}
	}
    MsgBuffer_apu[1] = prc_status_state;
    MsgBuffer_apu[2] = prc_status_err;
	if ((timer_skip==0) && (prc_status_err==0)) {
		timer_end_shift = XTmrCtr_GetCaptureValue(TmrCtrInstancePtr, TIMER_COUNTER_0);
		timer_end_count = XTmrCtr_GetCaptureValue(TmrCtrInstancePtr, TIMER_COUNTER_1);
		if (shift_loading) {
            MsgBuffer_apu[3] = timer_end_shift-timer_start_shift;
			//xil_printf ("Configuration Time = %d[us]\n\r", (timer_end_shift-timer_start_shift)*5/1000);
        } else if (count_loading) {
			//xil_printf (" Configuration Time = %d[us]\n\r", (timer_end_count-timer_start_count)*5/1000);
            MsgBuffer_apu[3] = timer_end_count-timer_start_count;
        }
        shift_loading=0; count_loading=0;
	}
	else {
		timer_skip=0;
		prc_status_err=0;
	}

	/* Call the test routine */
	Status = DoIpiTest(&IpiInst);


	return Status;

}

