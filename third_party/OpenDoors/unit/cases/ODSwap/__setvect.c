#ifdef ODPLAT_DOS
#include <string.h>
#include "common.h"

static void passes_the_vector_table_to_the_installer(void)
{
   VECTOR table[2];
   UTAsmInterruptStep *step;
   memset(table, 0, sizeof(table));
   table[0].number = 0x21;
   table[1].flag = 3;
   ut_asm_reset_steps();
   step = ut_asm_add_dependency_step(UT_ASM_DEPENDENCY_SETVECTSUB);
   step->check = UTR_ES | UTR_BX;
   step->expected.es = UT_FP_SEG(table);
   step->expected.bx = UT_FP_OFF(table);
   _setvect(table);
   ut_asm_assert_steps_complete();
}

static const UTTestCase ut_cases[] = {
   {"install vectors", passes_the_vector_table_to_the_installer}
};
#endif
