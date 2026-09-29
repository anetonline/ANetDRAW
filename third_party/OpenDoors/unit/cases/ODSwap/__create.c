#ifdef ODPLAT_DOS
#include "common.h"

static void add_version_step(unsigned int major_version)
{
   UTAsmInterruptStep *step = ut_asm_add_step(0x21);
   step->check = UTR_AX;
   step->expected.ax = 0x3000;
   step->set = UTR_AX;
   step->output.ax = major_version;
}

static void add_create_step(unsigned int request, unsigned int result,
   unsigned int carry)
{
   UTAsmInterruptStep *step = ut_asm_add_step(0x21);
   step->check = UTR_AX | UTR_CX;
   step->expected.ax = request;
   step->expected.cx = 0;
   step->set = UTR_AX | UTR_FLAGS;
   step->output.ax = result;
   step->output.flags = carry ? UTF_CARRY : 0;
}

static void uses_create_new_on_dos_three_and_later(void)
{
   int handle = -1;
   char name[] = "SWAP.$$$";
   ut_asm_reset_steps();
   add_version_step(3);
   add_create_step(0x5b03, 9, 0);
   UT_ASSERT_EQ_INT(0, _create(name, &handle));
   UT_ASSERT_EQ_INT(9, handle);
   ut_asm_assert_steps_complete();
}

static void uses_create_on_dos_two(void)
{
   int handle = -1;
   char name[] = "SWAP.$$$";
   ut_asm_reset_steps();
   add_version_step(2);
   add_create_step(0x3c02, 7, 0);
   UT_ASSERT_EQ_INT(0, _create(name, &handle));
   UT_ASSERT_EQ_INT(7, handle);
   ut_asm_assert_steps_complete();
}

static void returns_the_dos_error_without_changing_the_handle(void)
{
   int handle = -1;
   char name[] = "SWAP.$$$";
   ut_asm_reset_steps();
   add_version_step(3);
   add_create_step(0x5b03, 5, 1);
   UT_ASSERT_EQ_INT(5, _create(name, &handle));
   UT_ASSERT_EQ_INT(-1, handle);
   ut_asm_assert_steps_complete();
}

static const UTTestCase ut_cases[] = {
   {"DOS 3 create new", uses_create_new_on_dos_three_and_later},
   {"DOS 2 create", uses_create_on_dos_two},
   {"DOS error", returns_the_dos_error_without_changing_the_handle}
};
#endif
