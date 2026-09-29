/* OpenDoors 32-bit DOS FOSSIL support. */

#define BUILDING_OPENDOORS

#include <i86.h>
#include <string.h>

#include "OpenDoor.h"
#include "OD32DPMI.h"
#include "OD32Foss.h"

#ifdef ODPLAT_DOS32

static BOOL OD32FossilCall(tOD32RealModeRegisters *pRegisters)
{
   return(OD32DPMIRealModeInterrupt(0x14, pRegisters));
}

BOOL OD32FossilDetect(BYTE btPort)
{
   tOD32RealModeRegisters Registers;

   memset(&Registers, 0, sizeof(Registers));
   Registers.eax = 0x0400;
   Registers.edx = btPort;
   if(!OD32FossilCall(&Registers))
      return(FALSE);
   return((WORD)Registers.eax == 0x1954);
}

void OD32FossilInitialize(BYTE btPort, BYTE btSettings)
{
   tOD32RealModeRegisters Registers;

   memset(&Registers, 0, sizeof(Registers));
   Registers.eax = btSettings;
   Registers.edx = btPort;
   OD32FossilCall(&Registers);
}

void OD32FossilClose(BYTE btPort)
{
   tOD32RealModeRegisters Registers;

   memset(&Registers, 0, sizeof(Registers));
   Registers.eax = 0x0500;
   Registers.edx = btPort;
   OD32FossilCall(&Registers);
}

void OD32FossilSetFlow(BYTE btPort, BYTE btFlow)
{
   tOD32RealModeRegisters Registers;

   memset(&Registers, 0, sizeof(Registers));
   Registers.eax = 0x0f00 | btFlow;
   Registers.edx = btPort;
   OD32FossilCall(&Registers);
}

WORD OD32FossilStatus(BYTE btPort)
{
   tOD32RealModeRegisters Registers;

   memset(&Registers, 0, sizeof(Registers));
   Registers.eax = 0x0300;
   Registers.edx = btPort;
   if(!OD32FossilCall(&Registers))
      return(0);
   return((WORD)Registers.eax);
}

void OD32FossilSetDTR(BYTE btPort, BOOL bHigh)
{
   tOD32RealModeRegisters Registers;

   memset(&Registers, 0, sizeof(Registers));
   Registers.eax = 0x0600 | (bHigh ? 1 : 0);
   Registers.edx = btPort;
   OD32FossilCall(&Registers);
}

static void OD32FossilSimpleCall(BYTE btPort, BYTE btFunction)
{
   tOD32RealModeRegisters Registers;

   memset(&Registers, 0, sizeof(Registers));
   Registers.eax = (WORD)btFunction << 8;
   Registers.edx = btPort;
   OD32FossilCall(&Registers);
}

void OD32FossilClearOutbound(BYTE btPort)
{
   OD32FossilSimpleCall(btPort, 9);
}

void OD32FossilClearInbound(BYTE btPort)
{
   OD32FossilSimpleCall(btPort, 10);
}

BYTE OD32FossilGetByte(BYTE btPort)
{
   tOD32RealModeRegisters Registers;

   memset(&Registers, 0, sizeof(Registers));
   Registers.eax = 0x0200;
   Registers.edx = btPort;
   OD32FossilCall(&Registers);
   return((BYTE)Registers.eax);
}

BOOL OD32FossilSendByte(BYTE btPort, BYTE btValue)
{
   tOD32RealModeRegisters Registers;

   memset(&Registers, 0, sizeof(Registers));
   Registers.eax = 0x0b00 | btValue;
   Registers.edx = btPort;
   if(!OD32FossilCall(&Registers))
      return(FALSE);
   return((WORD)Registers.eax != 0);
}

BOOL OD32FossilBufferAllocate(tOD32FossilBuffer *pBuffer, WORD wSize)
{
   union REGS Registers;
   DWORD dwLinear;

   if(pBuffer == NULL || wSize == 0)
      return(FALSE);
   memset(pBuffer, 0, sizeof(*pBuffer));
   memset(&Registers, 0, sizeof(Registers));
   Registers.w.ax = 0x0100;
   Registers.w.bx = (wSize + 15U) / 16U;
   int386(0x31, &Registers, &Registers);
   if(Registers.x.cflag != 0)
      return(FALSE);

   pBuffer->wRealSegment = Registers.w.ax;
   pBuffer->wSelector = Registers.w.dx;
   pBuffer->wSize = wSize;

   memset(&Registers, 0, sizeof(Registers));
   Registers.w.ax = 0x0006;
   Registers.w.bx = pBuffer->wSelector;
   int386(0x31, &Registers, &Registers);
   if(Registers.x.cflag != 0)
   {
      OD32FossilBufferFree(pBuffer);
      return(FALSE);
   }
   dwLinear = ((DWORD)Registers.w.cx << 16) | Registers.w.dx;
   pBuffer->pLinear = (BYTE *)dwLinear;
   return(TRUE);
}

void OD32FossilBufferFree(tOD32FossilBuffer *pBuffer)
{
   union REGS Registers;

   if(pBuffer == NULL)
      return;
   if(pBuffer->wSelector != 0)
   {
      memset(&Registers, 0, sizeof(Registers));
      Registers.w.ax = 0x0101;
      Registers.w.dx = pBuffer->wSelector;
      int386(0x31, &Registers, &Registers);
   }
   memset(pBuffer, 0, sizeof(*pBuffer));
}

INT OD32FossilReceiveBlock(BYTE btPort, tOD32FossilBuffer *pBuffer,
   BYTE *pDestination, INT nSize)
{
   tOD32RealModeRegisters Registers;
   INT nTransfer;
   INT nReceived;

   if(pBuffer == NULL || pBuffer->pLinear == NULL || pDestination == NULL
      || nSize <= 0)
      return(-1);
   nTransfer = nSize < pBuffer->wSize ? nSize : pBuffer->wSize;
   memset(&Registers, 0, sizeof(Registers));
   Registers.eax = 0x1800;
   Registers.ecx = (WORD)nTransfer;
   Registers.edx = btPort;
   Registers.es = pBuffer->wRealSegment;
   Registers.edi = 0;
   if(!OD32FossilCall(&Registers))
      return(-1);
   nReceived = (WORD)Registers.eax;
   if(nReceived > nTransfer)
      return(-1);
   memcpy(pDestination, pBuffer->pLinear, (size_t)nReceived);
   return(nReceived);
}

INT OD32FossilSendBlock(BYTE btPort, tOD32FossilBuffer *pBuffer,
   const BYTE *pSource, INT nSize)
{
   tOD32RealModeRegisters Registers;
   INT nTransfer;
   INT nSent;

   if(pBuffer == NULL || pBuffer->pLinear == NULL || pSource == NULL
      || nSize <= 0)
      return(-1);
   nTransfer = nSize < pBuffer->wSize ? nSize : pBuffer->wSize;
   memcpy(pBuffer->pLinear, pSource, (size_t)nTransfer);
   memset(&Registers, 0, sizeof(Registers));
   Registers.eax = 0x1900;
   Registers.ecx = (WORD)nTransfer;
   Registers.edx = btPort;
   Registers.es = pBuffer->wRealSegment;
   Registers.edi = 0;
   if(!OD32FossilCall(&Registers))
      return(-1);
   nSent = (WORD)Registers.eax;
   if(nSent == 0 || nSent > nTransfer)
      return(-1);
   return(nSent);
}

#endif /* ODPLAT_DOS32 */
