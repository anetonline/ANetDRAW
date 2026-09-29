#ifdef ODPLAT_DOS
#include "common.h"

static void returns_the_complete_child_status_word(void)
{
   UTAsmInterruptStep *step;
   ut_asm_reset_steps();
   step = ut_asm_add_step(0x21);
   step->set = UTR_AX;
   step->output.ax = 0x027f;
   UT_ASSERT_EQ_INT(0x027f, _getrc());
   ut_asm_assert_steps_complete();
}

static const UTTestCase ut_cases[] = {
   {"child status", returns_the_complete_child_status_word}
};
#endif
