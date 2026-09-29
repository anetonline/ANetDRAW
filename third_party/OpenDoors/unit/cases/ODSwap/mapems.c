#ifdef ODPLAT_DOS
#include "common.h"

static void returns_the_first_nonbusy_manager_status(void)
{
   UTAsmInterruptStep *step;
   ut_asm_reset_steps();
   step = ut_asm_add_step(0x67);
   step->check = UTR_AX | UTR_BX | UTR_DX;
   step->expected.ax = 0x4400;
   step->expected.bx = 3;
   step->expected.dx = 4;
   step->set = UTR_AX;
   step->output.ax = 0x0000;
   ut_asm_target_registers.bx = 3;
   ut_asm_target_registers.dx = 4;
   ut_asm_call_target();
   UT_ASSERT_EQ_UINT(0, ut_asm_target_registers.ax);
   ut_asm_assert_steps_complete();
}

static void retries_while_the_memory_manager_is_busy(void)
{
   UTAsmInterruptStep *step;
   ut_asm_reset_steps();
   step = ut_asm_add_step(0x67);
   step->set = UTR_AX;
   step->output.ax = 0x8200;
   step = ut_asm_add_step(0x67);
   step->set = UTR_AX;
   step->output.ax = 0x8100;
   ut_asm_target_registers.bx = 7;
   ut_asm_target_registers.dx = 8;
   ut_asm_call_target();
   UT_ASSERT_EQ_UINT(0x8100, ut_asm_target_registers.ax);
   ut_asm_assert_steps_complete();
}

static const UTTestCase ut_cases[] = {
   {"nonbusy status", returns_the_first_nonbusy_manager_status},
   {"busy retry", retries_while_the_memory_manager_is_busy}
};
#endif
