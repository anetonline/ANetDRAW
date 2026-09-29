#ifdef ODPLAT_DOS
#include <string.h>
#include "common.h"

static void installs_current_entries_and_skips_free_entries(void)
{
   VECTOR table[3];
   UTAsmInterruptStep *step;
   memset(table, 0, sizeof(table));
   table[0].number = 0x21;
   table[0].flag = 0;
   table[0].vseg = 0xabcd;
   table[0].voff = 0x1234;
   table[1].number = 0x22;
   table[1].flag = 2;
   table[2].flag = 3;
   ut_asm_reset_steps();
   step = ut_asm_add_step(0x21);
   step->check = UTR_AX | UTR_DX | UTR_DS;
   step->expected.ax = 0x2521;
   step->expected.dx = 0x1234;
   step->expected.ds = 0xabcd;
   ut_asm_target_registers.es = UT_FP_SEG(table);
   ut_asm_target_registers.bx = UT_FP_OFF(table);
   ut_asm_call_target();
   ut_asm_assert_steps_complete();
}

static const UTTestCase ut_cases[] = {
   {"vector table", installs_current_entries_and_skips_free_entries}
};
#endif
