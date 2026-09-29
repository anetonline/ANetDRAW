/* OpenDoors isolated 16-bit assembly test interface.  Keep this C89. */
#ifndef OPENDOORS_UNIT_UTASM_H
#define OPENDOORS_UNIT_UTASM_H

#ifdef __WATCOMC__
#include <i86.h>
#define UT_FP_SEG(pointer) _FP_SEG(pointer)
#define UT_FP_OFF(pointer) _FP_OFF(pointer)
#define UT_MK_FP(segment, offset) _MK_FP(segment, offset)
#else
#include <dos.h>
#define UT_FP_SEG(pointer) FP_SEG(pointer)
#define UT_FP_OFF(pointer) FP_OFF(pointer)
#define UT_MK_FP(segment, offset) MK_FP(segment, offset)
#endif

typedef struct UTAsmRegisters
{
   unsigned short ax;
   unsigned short cx;
   unsigned short dx;
   unsigned short bx;
   unsigned short bp;
   unsigned short si;
   unsigned short di;
   unsigned short ds;
   unsigned short es;
   unsigned short flags;
} UTAsmRegisters;

#ifdef __WATCOMC__
#define UTASMCALL __cdecl
#else
#define UTASMCALL
#endif

void UTASMCALL ut_asm_interrupt(unsigned interrupt_number,
   UTAsmRegisters far *registers);
void UTASMCALL ut_asm_dependency(unsigned dependency_index,
   UTAsmRegisters far *registers);

#endif
