/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
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

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "xil_io.h"
#include "platform.h"
#include "xil_printf.h"


int main()
{
	u32 pcap_ctrl_reg;
	u32 mask = 0x00000001;
	init_platform();

	//   xil_printf("*** PCAP Register Access  ***\r\n");
	pcap_ctrl_reg = Xil_In32(0xFFCA3008); // read PCAP_CTRL
	pcap_ctrl_reg &= ~( 1 << 0 ); // clear PCAP_CTRL, bit 0 [ICAP]
	Xil_Out32(0xFFCA3008, pcap_ctrl_reg); // write PCAP_CTRL


	pcap_ctrl_reg = Xil_In32(0xFFCA3008); // read DATA_3 (GPIO) Register
	pcap_ctrl_reg = mask & pcap_ctrl_reg;

	if ( (pcap_ctrl_reg != 0x00000000) ) {
		xil_printf("ERROR: PCAP_CTRL[0] is not cleared (0x%08XBit[0]), it should be set to 0 to enable ICAP\n", pcap_ctrl_reg);
	} else {
		print("PCAP disabled, ICAP is enabled\n");
	}

    cleanup_platform();
    return 0;
}
