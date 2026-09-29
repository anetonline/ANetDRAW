#ifdef ODPLAT_DOS
#include "common.h"

static void returns_the_ems_status_and_handle(void)
{
   int handle = 0;
   UTAsmInterruptStep *step;
   ut_asm_reset_steps();
   step = ut_asm_add_step(0x67);
   step->check = UTR_BX;
   step->expected.bx = 7;
   step->set = UTR_AX | UTR_DX;
   step->output.ax = 0x8a11;
   step->output.dx = 0x1234;
   UT_ASSERT_EQ_INT(0x8a00, _getems(7, &handle));
   UT_ASSERT_EQ_INT(0x1234, handle);
   ut_asm_assert_steps_complete();
}

static const UTTestCase ut_cases[] = {
   {"allocate EMS", returns_the_ems_status_and_handle}
};
#endif
