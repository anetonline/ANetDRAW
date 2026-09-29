#ifdef ODPLAT_DOS
#include <string.h>
#include "common.h"

void ODSWAPCALL ut_asm_set_word_vtabseg(unsigned int value);
void ODSWAPCALL ut_asm_set_word_vtaboff(unsigned int value);

static void substitutes_the_safe_handler_and_installs_the_table(void)
{
   VECTOR table[3];
   UTAsmInterruptStep *step;
   unsigned int table_segment;
   unsigned int table_offset;
   memset(table, 0, sizeof(table));
   table[0].number = 0x21;
   table[0].flag = 1;
   table[1].number = 0x22;
   table[1].flag = 0;
   table[1].vseg = 0x1111;
   table[1].voff = 0x2222;
   table[2].flag = 3;
   table_segment = UT_FP_SEG(table);
   table_offset = UT_FP_OFF(table);
   ut_asm_set_word_vtabseg(table_segment);
   ut_asm_set_word_vtaboff(table_offset);
   ut_asm_reset_steps();
   step = ut_asm_add_dependency_step(UT_ASM_DEPENDENCY_SETVECTSUB);
   step->check = UTR_ES | UTR_BX;
   step->expected.es = table_segment;
   step->expected.bx = table_offset;
   ut_asm_target_registers.cx = 0xabcd;
   ut_asm_target_registers.dx = 0x1234;
   ut_asm_call_target();
   UT_ASSERT_EQ_UINT(0xabcd, table[0].vseg);
   UT_ASSERT_EQ_UINT(0x1234, table[0].voff);
   UT_ASSERT_EQ_UINT(0x1111, table[1].vseg);
   UT_ASSERT_EQ_UINT(0x2222, table[1].voff);
   ut_asm_assert_steps_complete();
}

static const UTTestCase ut_cases[] = {
   {"safe vector table", substitutes_the_safe_handler_and_installs_the_table}
};
#endif
