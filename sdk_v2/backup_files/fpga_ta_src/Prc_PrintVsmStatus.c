#include "xil_types.h"
#include "xil_assert.h"
#include "xstatus.h"
#include "xprc.h"
#include "xprc_hw.h"
#include "xil_printf.h"


/**
*
* This function is used to Print the VSM Status.
*
* @param  InstancePtr is a pointer to the PRC instance.
* @param  VsmIdOrStatus is an identifier of the VSM to access or
*     a pre-read status word.
* @param  Prefix is a text string that will prefix each line of output.
*
* @return None.
*
* @note   None.
*
******************************************************************************/
void Prc_PrintVsmStatus(XPrc *InstancePtr, u32 VsmIdOrStatus, char * Prefix)
{
  u32 Status = VsmIdOrStatus;
  u8 Is_shutdown;
  u8 State;
  u8 Error;

  if (InstancePtr != NULL) {
    Status = XPrc_ReadStatusReg(InstancePtr, VsmIdOrStatus);
  }

  Is_shutdown = XPrc_IsVsmInShutdown(NULL, Status);
  State = XPrc_GetVsmState(NULL, Status);
  Error = XPrc_GetVsmErrorStatus(NULL, Status);

  if (Is_shutdown != 0) {
    /* The STATE field doesn't represent the state. Instead, bit 0
     * contains the value of RM_SHUTDOWN_ACK. That means we don't
     * tell RM_ID or BS_ID either because they are only valid if
     * the VSM isn'tempty, and we can't tell that in the shutdown
     * mode.
     */

    /* Shutdown mode */
	  xil_printf("%sMode : SHUTDOWN\n\r",
      Prefix);
	  xil_printf("%sRM_SHUTDOWN_ACK : %x\n\r",
      Prefix, State);
  }
  else {
    /* Active Mode */
    xil_printf("%sMode : ACTIVE\n\r", Prefix);
    switch (State) {
      case XPRC_SR_STATE_EMPTY:
        xil_printf("%sSTATE :"
          "EMPTY (%x)\n\r", Prefix, State);
        break;
      case XPRC_SR_STATE_HW_SHUTDOWN:
        xil_printf("%sSTATE : HW"
          "SHUTDOWN (%x)\n\r", Prefix, State);
        break;
      case XPRC_SR_STATE_SW_SHUTDOWN:
        xil_printf("%sSTATE : SW"
          "SHUTDOWN (%x)\n\r", Prefix, State);
        break;
      case XPRC_SR_STATE_RM_UNLOAD:
        xil_printf("%sSTATE : RM"
          "UNLOAD (%x)\n\r", Prefix, State);
        break;
      case XPRC_SR_STATE_RM_LOAD:
        xil_printf("%sSTATE : RM"
          "LOAD (%x)\n\r", Prefix, State);
        break;
      case XPRC_SR_STATE_SW_STARTUP:
        xil_printf("%sSTATE : SW"
          "STARTUP (%x)\n\r", Prefix, State);
        break;
      case XPRC_SR_STATE_RM_RESET:
        xil_printf("%sSTATE : RM"
          "RESET (%x)\n\r", Prefix, State);
        break;
      case XPRC_SR_STATE_FULL:
        xil_printf("%sSTATE : FULL"
          "(%x)\n\r", Prefix, State);
        break;
      default:
        break;
    }
    if (State != XPRC_SR_STATE_EMPTY) {
      xil_printf("%sRM_ID : %d\n\r",
        Prefix, XPrc_GetRmIdFromStatus(NULL, Status));
      xil_printf("%sBS_ID : %d\n\r",
        Prefix, XPrc_GetBsIdFromStatus(NULL, Status));
    }
  }
  switch (Error) {
    case XPRC_SR_BS_COMPATIBLE_ERROR:
      xil_printf("%sERROR : BS"
        "COMPATIBLE ERROR (%x)\n\r", Prefix, Error);
      break;
    case XPRC_SR_DECOMPRESS_BAD_FORMAT_ERROR:
      xil_printf("%sERROR : DECOMPRESS"
        "BAD FORMAT ERROR (%x)\n\r", Prefix, Error);
      break;
    case XPRC_SR_DECOMPRESS_BAD_SIZE_ERROR:
      xil_printf("%sERROR : DECOMPRESS"
        "BAD SIZE ERROR (%x)\n\r", Prefix, Error);
      break;
    case XPRC_SR_FETCH_AND_CP_LOST_ERROR:
      xil_printf("%sERROR : FETCH AND"
        "CP LOST_ERROR (%x)\n\r", Prefix, Error);
      break;
    case XPRC_SR_FETCH_AND_BS_ERROR:
      xil_printf("%sERROR : FETCH AND BS"
        "ERROR (%x)\n\r", Prefix, Error);
      break;
    case XPRC_SR_FETCH_ERROR:
      xil_printf("%sERROR : FETCH ERROR"
        "(%x)\n\r", Prefix, Error);
      break;
    case XPRC_SR_CP_LOST_ERROR:
      xil_printf("%sERROR : CP LOST"
        "ERROR (%x)\n\r", Prefix, Error);
      break;
    case XPRC_SR_BS_ERROR:
      xil_printf("%sERROR : BS ERROR"
        "(%x)\n\r", Prefix, Error);
      break;
    case XPRC_SR_BAD_CONFIG_ERROR:
      xil_printf("%sERROR : BAD CONFIG"
        "ERROR (%x)\n\r", Prefix, Error);
      break;
    case XPRC_SR_NO_ERROR:
      xil_printf("%sERROR : NO ERROR"
        "(%x)\n\r", Prefix, Error);
      break;
    default:
      xil_printf("%sERROR : UNKNOWN"
        "(%x)\n\r", Prefix, Error);
      break;
  }
}
