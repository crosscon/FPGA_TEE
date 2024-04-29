/******************************************************************************
*
* Copyright (C) 2009 - 2020 Xilinx, Inc.  All rights reserved.
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

#include <stdio.h>
#include "platform.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xil_io.h"
#include "xtmrctr.h"
#include "xprc.h"
#include "dfxc_demo.h"


int main()
{


  u32 Exit   = 0;
  u32 Option = 1;



  //For DFX Controller
  u32 prc_init;
  u32 prc_status;
  u32 prc_status_state;
  u32 prc_status_last_state;
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


  init_platform();

  //Timer Setting
  timer_init = XTmrCtr_Initialize(TmrCtrInstancePtr, TMRCTR_DEVICE_ID);
  if (timer_init != XST_SUCCESS) {
    print("Timer Initialization Fails\r\n");
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
  if (prc_init != XST_SUCCESS) {
    return XST_FAILURE;
  }
  //

  //DFX Controller initial setting
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


  print("\n\r\n\r*** Partial Configuration of vFPGAs by Shell  ***\n\r");

  while(Exit != 1) {
    do {
      print ("------------------ Menu ------------------\n\r");
      print ("    1: Enable Partial Reconfiguration on vFPGA_1\n\r");
      print ("    2: Enable Partial Reconfiguration on vFPGA_2\n\r");
      print ("    3: Load Accelerator on vFPGA_1 (Shift Left)\n\r");
      print ("    4: Load Accelerator on vFPGA_1 (Shift Right)\n\r");
      print ("    5: Load Accelerator on vFPGA_2 (Count Up)\n\r");
      print ("    6: Load Accelerator on vFPGA_2 (Count Down)\n\r");
      print ("    7: Lock vFPGA_1\n\r");
      print ("    8: Lock vFPGA_2\n\r");
      print ("    0: Exit\n\r");
      print ("> ");

      Option = inbyte();

      if (isalpha(Option)) {
        Option = toupper(Option);
      }

      xil_printf("%c\n\r", Option);

    } while (!isdigit(Option));
        print ("\n\r");

    switch (Option) {
      case '0' :
        Exit = 1;
        break;
      case '1' :
        print("Enabling Partial Reconfiguration on vFPGA_1\n\r\n\r");
        XPrc_SendRestartWithNoStatusCommand(&Prc, XDFXC_VS_SHIFT_ID);
        while(XPrc_IsVsmInShutdown(&Prc, XDFXC_VS_SHIFT_ID)==XPRC_SR_SHUTDOWN_ON);
        timer_skip = 1;
        break;
      case '2' :
        print("Enabling Partial Reconfiguration on vFPGA_2\n\r\n\r");
        XPrc_SendRestartWithNoStatusCommand(&Prc, XDFXC_VS_COUNT_ID);
        while(XPrc_IsVsmInShutdown(&Prc, XDFXC_VS_COUNT_ID)==XPRC_SR_SHUTDOWN_ON);
        timer_skip = 1;
        break;
      case '3' :
        print("Loading Shift Left on vFPGA_1 ...\n\r\n\r");
        if (XPrc_IsSwTriggerPending(&Prc, XDFXC_VS_SHIFT_ID, NULL)==XPRC_NO_SW_TRIGGER_PENDING) {
          print ("Starting Reconfiguration\n\r");
          XPrc_SendSwTrigger(&Prc, XDFXC_VS_SHIFT_ID, XDFXC_VS_SHIFT_RM_SHIFT_LEFT_ID);
        }
        shift_loading=1; rm_loading=1;
        break;
      case '4' :
        print("Loading Shift Right on vFPGA_1 ...\n\r\n\r");
        if (XPrc_IsSwTriggerPending(&Prc, XDFXC_VS_SHIFT_ID, NULL)==XPRC_NO_SW_TRIGGER_PENDING) {
          print ("Starting Reconfiguration\n\r");
          XPrc_SendSwTrigger(&Prc, XDFXC_VS_SHIFT_ID, XDFXC_VS_SHIFT_RM_SHIFT_RIGHT_ID);
        }
        shift_loading=1; rm_loading=1;
        break;
      case '5' :
        print("Loading Count Up on vFPGA_2 ...\n\r\n\r");
        if (XPrc_IsSwTriggerPending(&Prc, XDFXC_VS_COUNT_ID, NULL)==XPRC_NO_SW_TRIGGER_PENDING) {
          print ("Starting Reconfiguration\n\r");
          XPrc_SendSwTrigger(&Prc, XDFXC_VS_COUNT_ID, XDFXC_VS_COUNT_RM_COUNT_UP_ID);
        }
        count_loading=1; rm_loading=1;
        break;
      case '6' :
        print("Loading Count Down on vFPGA_2 ...\n\r\n\r");
        if (XPrc_IsSwTriggerPending(&Prc, XDFXC_VS_COUNT_ID, NULL)==XPRC_NO_SW_TRIGGER_PENDING) {
          print ("Starting Reconfiguration\n\r");
          XPrc_SendSwTrigger(&Prc, XDFXC_VS_COUNT_ID, XDFXC_VS_COUNT_RM_COUNT_DOWN_ID);
        }
        count_loading=1; rm_loading=1;
        break;
      case '7' :
        print("Locking vFPGA_1...\n\r\n\r");
        print("vFPGA_1 cannot be reconfigured...\n\r\n\r");
        XPrc_SendShutdownCommand(&Prc, XDFXC_VS_SHIFT_ID);
        while(XPrc_IsVsmInShutdown(&Prc, XDFXC_VS_SHIFT_ID)==XPRC_SR_SHUTDOWN_OFF);
        timer_skip = 1;
        break;
      case '8' :
        print("Locking vFPGA_2...\n\r\n\r");
        print("vFPGA_2 cannot be reconfigured...\n\r\n\r");
        XPrc_SendShutdownCommand(&Prc, XDFXC_VS_COUNT_ID);
        while(XPrc_IsVsmInShutdown(&Prc, XDFXC_VS_COUNT_ID)==XPRC_SR_SHUTDOWN_OFF);
        timer_skip = 1;
        break;
      default:
        break;
    }

    //Timer Value when reconfig start
    timer_start_shift = XTmrCtr_GetValue(TmrCtrInstancePtr, TIMER_COUNTER_0);
    timer_start_count = XTmrCtr_GetValue(TmrCtrInstancePtr, TIMER_COUNTER_1);

    while (rm_loading) {
      if (shift_loading==1) {
        prc_status=XPrc_ReadStatusReg(&Prc, XDFXC_VS_SHIFT_ID);
      }
      else {
        prc_status=XPrc_ReadStatusReg(&Prc, XDFXC_VS_COUNT_ID);
      }

      prc_status_state    =prc_status&0x07;
      prc_status_err      =prc_status&0xf8;

      switch(prc_status_err) {
        case (0x80) : print("The vFPGA is Locked!\n\r");
                      print("Enable Partial Reconfiguration!\n\r\n\r");
                      timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
                      break;
        case (0xC0) : print("Incorrect compressed bit format is Found!\n\r");
                      print("Set vFPGA Active mode and reconfig with correct bit!\n\r\n\r");
                      timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
                      break;
        case (0xB8) : print("Incorrect compressed bit size is Found!\n\r");
                    print("Set vFPGA Active mode and reconfig with correct bit!\n\r\n\r");
                      timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
                      break;
        case (0x78) : print("Unknown Error!\n\r");
              		  timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
                      break;
        case (0xB0) : print("Lost + Fetch Error!\n\r");
              		  timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
                      break;
        case (0xA8) : print("BS + Fetch Error!\n\r");
              		  timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
                      break;
        case (0xA0) : print("Fetch Error!\n\r");
              		  timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
                      break;
        case (0x98) : print("Lost Error!\n\r");
                  	  timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
                      break;
        case (0x90) : print("Bitstream Error!\n\r");
                  	  timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
                      break;
        case (0x88) : print("Bad Config Error!\n\r");
                  	  timer_skip=1; shift_loading=0; count_loading=0; rm_loading=0;
                      break;

        default     : break;
      }


      while (prc_status_state != prc_status_last_state) {
        switch(prc_status_state) {
          case 7  : print("  Accelerator is successfully loaded\n\r"); rm_loading=0; break;
          case 6  : print("  Accelerator is being reset\n\r");                       break;
          case 5  : print("  Software start-up step\n\r");                  break;
          case 4  : print("  Loading new Accelerator\n\r");                          break;
          case 2  : print("  Software Shutdown\n\r");                       break;
          case 1  : print("  Hardware Shutdown\n\r");                       break;
          default : break;
        }
        prc_status_last_state = prc_status_state;
      }
    }

    if ((timer_skip==0) && (prc_status_err==0)) {
      timer_end_shift = XTmrCtr_GetCaptureValue(TmrCtrInstancePtr, TIMER_COUNTER_0);
      timer_end_count = XTmrCtr_GetCaptureValue(TmrCtrInstancePtr, TIMER_COUNTER_1);
      if (shift_loading)
        xil_printf ("  PR Time = %d[us]\n\r", (timer_end_shift-timer_start_shift)*5/1000);
      else if (count_loading)
        xil_printf ("  PR Time = %d[us]\n\r", (timer_end_count-timer_start_count)*5/1000);

      shift_loading=0; count_loading=0;
      print("Demo is Done!\n\r\n\r");
    }

    else {
      timer_skip=0; prc_status_err=0;
    }

  }

  cleanup_platform();
  return 0;
}
