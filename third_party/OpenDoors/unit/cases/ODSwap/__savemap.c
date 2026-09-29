#ifdef ODPLAT_DOS
#include "common.h"

static void passes_the_page_map_buffer_to_ems(void)
{
   char buffer[16];
   UTAsmInterruptStep *step;
   ut_asm_reset_steps();
   step = ut_asm_add_step(0x67);
   step->check = UTR_AX | UTR_DI | UTR_ES;
   step->expected.ax = 0x4e00;
   step->expected.di = UT_FP_OFF(buffer);
   step->expected.es = UT_FP_SEG(buffer);
   step->set = UTR_AX;
   step->output.ax = 0x8507;
   UT_ASSERT_EQ_INT(0x8507, _savemap(buffer));
   ut_asm_assert_steps_complete();
}

static const UTTestCase ut_cases[] = {
   {"save page map", passes_the_page_map_buffer_to_ems}
};
#endif
