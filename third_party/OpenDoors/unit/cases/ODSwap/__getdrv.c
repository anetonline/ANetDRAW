#ifdef ODPLAT_DOS
#include "common.h"

static void returns_the_default_drive_from_dos(void)
{
   UTAsmInterruptStep *step;
   ut_asm_reset_steps();
   step = ut_asm_add_step(0x21);
   step->set = UTR_AX;
   step->output.ax = 0x1902;
   UT_ASSERT_EQ_INT(2, _getdrv());
   ut_asm_assert_steps_complete();
}

static const UTTestCase ut_cases[] = {
   {"default drive", returns_the_default_drive_from_dos}
};
#endif
