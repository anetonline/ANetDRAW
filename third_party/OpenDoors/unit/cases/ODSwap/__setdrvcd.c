#ifdef ODPLAT_DOS
#include "common.h"

static void selects_the_drive_then_changes_directory(void)
{
   char directory[] = "\\DOORS";
   UTAsmInterruptStep *step;
   ut_asm_reset_steps();
   step = ut_asm_add_step(0x21);
   (void)step;
   step = ut_asm_add_step(0x21);
   step->check = UTR_DX | UTR_DS;
   step->expected.dx = UT_FP_OFF(directory);
   step->expected.ds = UT_FP_SEG(directory);
   _setdrvcd(2, directory);
   ut_asm_assert_steps_complete();
}

static const UTTestCase ut_cases[] = {
   {"drive and directory", selects_the_drive_then_changes_directory}
};
#endif
