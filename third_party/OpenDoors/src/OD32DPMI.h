/* OpenDoors 32-bit DOS DPMI helpers. */

#ifndef _INC_OD32DPMI
#define _INC_OD32DPMI

#include "OpenDoor.h"

#ifdef ODPLAT_DOS32

#pragma pack( __push, 1 )
typedef struct
{
   DWORD edi;
   DWORD esi;
   DWORD ebp;
   DWORD reserved;
   DWORD ebx;
   DWORD edx;
   DWORD ecx;
   DWORD eax;
   WORD flags;
   WORD es;
   WORD ds;
   WORD fs;
   WORD gs;
   WORD ip;
   WORD cs;
   WORD sp;
   WORD ss;
} tOD32RealModeRegisters;
#pragma pack( __pop )

BOOL OD32DPMIRealModeInterrupt(BYTE btInterrupt,
   tOD32RealModeRegisters *pRealRegisters);

#endif /* ODPLAT_DOS32 */

#endif /* !_INC_OD32DPMI */
