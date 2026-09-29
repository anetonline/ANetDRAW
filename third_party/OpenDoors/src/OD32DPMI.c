/* OpenDoors 32-bit DOS DPMI helpers. */

#define BUILDING_OPENDOORS

#include <i86.h>
#include <string.h>

#include "OpenDoor.h"
#include "OD32DPMI.h"

#ifdef ODPLAT_DOS32

BOOL OD32DPMIRealModeInterrupt(BYTE btInterrupt,
   tOD32RealModeRegisters *pRealRegisters)
{
   union REGS Registers;
   struct SREGS SegmentRegisters;

   memset(&Registers, 0, sizeof(Registers));
   segread(&SegmentRegisters);
   Registers.w.ax = 0x0300;
   Registers.h.bl = btInterrupt;
   Registers.h.bh = 0;
   Registers.w.cx = 0;
   Registers.x.edi = FP_OFF(pRealRegisters);
   SegmentRegisters.es = FP_SEG(pRealRegisters);
   int386x(0x31, &Registers, &Registers, &SegmentRegisters);
   return(Registers.x.cflag == 0);
}

#endif /* ODPLAT_DOS32 */
