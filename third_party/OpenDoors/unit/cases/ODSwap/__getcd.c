#ifdef ODPLAT_DOS
#include "common.h"

static void add_get_directory_step(char *buffer, unsigned int drive,
   unsigned int result, unsigned int carry)
{
   UTAsmInterruptStep *step = ut_asm_add_step(0x21);
   step->check = UTR_DX | UTR_DS | UTR_SI;
   step->expected.dx = drive;
   step->mask.dx = 0x00ff;
   step->expected.ds = UT_FP_SEG(buffer);
   step->expected.si = UT_FP_OFF(buffer);
   step->set = UTR_AX | UTR_FLAGS;
   step->output.ax = result;
   step->output.flags = carry ? UTF_CARRY : 0;
}

static void returns_zero_after_dos_reports_success(void)
{
   char buffer[64] = "CURRENT";
   ut_asm_reset_steps();
   add_get_directory_step(buffer, 5, 0x4700, 0);
   UT_ASSERT_EQ_INT(0, _getcd(5, buffer));
   ut_asm_assert_steps_complete();
}

static void returns_the_dos_error(void)
{
   char buffer[64] = "UNCHANGED";
   ut_asm_reset_steps();
   add_get_directory_step(buffer, 27, 15, 1);
   UT_ASSERT_EQ_INT(15, _getcd(27, buffer));
   ut_asm_assert_steps_complete();
}

static const UTTestCase ut_cases[] = {
   {"success", returns_zero_after_dos_reports_success},
   {"DOS error", returns_the_dos_error}
};
#endif
