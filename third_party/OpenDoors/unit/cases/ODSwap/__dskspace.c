#ifdef ODPLAT_DOS
#include "common.h"

static void returns_cluster_size_and_available_clusters(void)
{
   unsigned int cluster_size = 0;
   unsigned int clusters = 0;
   UTAsmInterruptStep *step;
   ut_asm_reset_steps();
   step = ut_asm_add_step(0x21);
   step->set = UTR_AX | UTR_CX | UTR_BX;
   step->output.ax = 4;
   step->output.cx = 512;
   step->output.bx = 10;
   UT_ASSERT_EQ_INT(0, _dskspace(3, &cluster_size, &clusters));
   UT_ASSERT_EQ_UINT(2048, cluster_size);
   UT_ASSERT_EQ_UINT(10, clusters);
   ut_asm_assert_steps_complete();
}

static void reports_an_invalid_drive_without_changing_outputs(void)
{
   unsigned int cluster_size = 0x1111;
   unsigned int clusters = 0x2222;
   UTAsmInterruptStep *step;
   ut_asm_reset_steps();
   step = ut_asm_add_step(0x21);
   step->set = UTR_AX;
   step->output.ax = 0xffff;
   UT_ASSERT_EQ_UINT(0xffff,
      (unsigned int)_dskspace(27, &cluster_size, &clusters));
   UT_ASSERT_EQ_UINT(0x1111, cluster_size);
   UT_ASSERT_EQ_UINT(0x2222, clusters);
   ut_asm_assert_steps_complete();
}

static const UTTestCase ut_cases[] = {
   {"valid drive", returns_cluster_size_and_available_clusters},
   {"invalid drive", reports_an_invalid_drive_without_changing_outputs}
};
#endif
