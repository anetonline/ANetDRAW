#ifdef ODPLAT_DOS
#include "common.h"

static void stores_the_vector_returned_by_dos(void)
{
   unsigned int segment = 0;
   unsigned int offset = 0;
   UTAsmInterruptStep *step;
   ut_asm_reset_steps();
   step = ut_asm_add_step(0x21);
   step->check = UTR_AX;
   step->expected.ax = 0x3521;
   step->set = UTR_ES | UTR_BX;
   step->output.es = 0xabcd;
   step->output.bx = 0x1234;
   _getvect(0x21, &segment, &offset);
   UT_ASSERT_EQ_UINT(0xabcd, segment);
   UT_ASSERT_EQ_UINT(0x1234, offset);
   ut_asm_assert_steps_complete();
}

static const UTTestCase ut_cases[] = {
   {"get vector", stores_the_vector_returned_by_dos}
};
#endif
