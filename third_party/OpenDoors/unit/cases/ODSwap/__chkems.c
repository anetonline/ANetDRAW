#ifdef ODPLAT_DOS
#include "common.h"

void ODSWAPCALL ut_asm_set_word_emsseg(unsigned int value);
unsigned int ODSWAPCALL ut_asm_get_word_emsseg(void);

static UTAsmInterruptStep *add_call(unsigned int interrupt_number,
   unsigned int expected_ax)
{
   UTAsmInterruptStep *step = ut_asm_add_step(interrupt_number);
   step->check = UTR_AX;
   step->expected.ax = expected_ax;
   return(step);
}

static void set_result(UTAsmInterruptStep *step, unsigned int ax,
   unsigned int carry)
{
   step->set = UTR_AX | UTR_FLAGS;
   step->output.ax = ax;
   step->output.flags = carry ? UTF_CARRY : 0;
}

static void add_open(char *device, unsigned int result, unsigned int carry)
{
   UTAsmInterruptStep *step = add_call(0x21, 0x3d00);
   step->check |= UTR_DS | UTR_DX;
   step->expected.ds = UT_FP_SEG(device);
   step->expected.dx = UT_FP_OFF(device);
   set_result(step, result, carry);
}

static void add_close(unsigned int expected_ax)
{
   UTAsmInterruptStep *step = add_call(0x21, expected_ax);
   step->check |= UTR_BX;
   step->expected.bx = 5;
}

static void configure_failure(char *device, unsigned int stage)
{
   UTAsmInterruptStep *step;
   add_open(device, stage == 0 ? 2 : 5, stage == 0);
   if(stage == 0)
      return;

   step = add_call(0x21, 0x4400);
   step->check |= UTR_BX;
   step->expected.bx = 5;
   step->set = UTR_AX | UTR_DX | UTR_FLAGS;
   step->output.ax = stage == 1 ? 5 : 0;
   step->output.dx = stage == 2 ? 0 : 0x0080;
   step->output.flags = stage == 1 ? UTF_CARRY : 0;
   if(stage == 1 || stage == 2)
   {
      add_close(stage == 1 ? 0x3e05 : 0x3e00);
      return;
   }

   step = add_call(0x21, 0x4407);
   step->check |= UTR_BX;
   step->expected.bx = 5;
   set_result(step, stage == 3 ? 5 : (stage == 4 ? 0 : 1),
      stage == 3);
   add_close(stage == 3 ? 0x3e05 :
      (stage == 4 ? 0x3e00 : 0x3e01));
   if(stage <= 4)
      return;

   step = add_call(0x67, 0x4001);
   set_result(step, stage == 5 ? 0x0100 : 0, 0);
   if(stage == 5)
      return;

   step = add_call(0x67, 0x4600);
   set_result(step, stage == 6 ? 0x0100 :
      (stage == 7 ? 0x0031 : 0x0032), 0);
   if(stage == 6 || stage == 7)
      return;

   step = add_call(0x67, 0x4132);
   step->set = UTR_AX | UTR_BX;
   step->output.ax = stage == 8 ? 0x0100 : 0;
   step->output.bx = 0xabcd;
   if(stage == 8)
      return;

   step = add_call(0x67, 0x4e03);
   set_result(step, stage == 9 ? 0x0100 : 32, 0);
}

static void rejects_each_unavailable_or_invalid_emm_stage(void)
{
   char device[] = "EMMXXXX0";
   unsigned int stage;
   int map_size;
   for(stage = 0; stage < 10; ++stage)
   {
      map_size = 0x5555;
      ut_asm_set_word_emsseg(0);
      ut_asm_reset_steps();
      configure_failure(device, stage);
      UT_ASSERT_EQ_INT(1, _chkems(device, &map_size));
      UT_ASSERT_EQ_INT(0x5555, map_size);
      ut_asm_assert_steps_complete();
   }
}

static void returns_the_page_map_size_for_a_functional_emm(void)
{
   char device[] = "EMMXXXX0";
   int map_size = 0;
   ut_asm_set_word_emsseg(0);
   ut_asm_reset_steps();
   configure_failure(device, 10);
   UT_ASSERT_EQ_INT(0, _chkems(device, &map_size));
   UT_ASSERT_EQ_INT(32, map_size);
   UT_ASSERT_EQ_UINT(0xabcd, ut_asm_get_word_emsseg());
   ut_asm_assert_steps_complete();
}

static const UTTestCase ut_cases[] = {
   {"unavailable stages", rejects_each_unavailable_or_invalid_emm_stage},
   {"functional EMM", returns_the_page_map_size_for_a_functional_emm}
};
#endif
