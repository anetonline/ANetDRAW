#ifdef ODPLAT_DOS
#include <string.h>
#include "common.h"

/* UT_ASM_MOCK_KERNEL_JUMP */
/* UT_ASM_EXTERNAL_RANGE fnish7 fnish11 */

void ODSWAPCALL ut_asm_set_word_psp(unsigned int value);
void ODSWAPCALL ut_asm_set_word_ttlsz(unsigned int value);
void ODSWAPCALL ut_asm_set_word_cntl(unsigned int value);
void ODSWAPCALL ut_asm_set_word_cnth(unsigned int value);
void ODSWAPCALL ut_asm_set_word_emsseg(unsigned int value);

static unsigned char psp_space[20000];
static unsigned char ems_space[18000];

static unsigned int aligned_psp_segment(void)
{
   unsigned int segment = UT_FP_SEG(psp_space);
   unsigned int offset = UT_FP_OFF(psp_space);
   return((unsigned int)(segment + (offset + 15U) / 16U));
}

static unsigned int aligned_ems_segment(void)
{
   unsigned int segment = UT_FP_SEG(ems_space);
   unsigned int offset = UT_FP_OFF(ems_space);
   return((unsigned int)(segment + (offset + 15U) / 16U));
}

static void prepare_vectors(VECTOR *vectors)
{
   memset(vectors, 0, 2 * sizeof(*vectors));
   vectors[0].number = 0x21;
   vectors[0].flag = 0;
   vectors[1].flag = 3;
}

static UTAsmInterruptStep *add_interrupt(unsigned int number,
   unsigned int ax)
{
   UTAsmInterruptStep *step = ut_asm_add_step(number);
   step->check = UTR_AX;
   step->expected.ax = ax;
   return(step);
}

static void add_filename_parsing(void)
{
   add_interrupt(0x21, 0x2901);
   add_interrupt(0x21, 0x2901);
}

static void add_safevect(void)
{
   ut_asm_add_dependency_step(UT_ASM_DEPENDENCY_SAFEVECT);
}

static void add_exec_result(unsigned int result, unsigned int carry)
{
   UTAsmInterruptStep *step = add_interrupt(0x21, 0x4b00);
   step->set = UTR_AX | UTR_FLAGS;
   step->output.ax = result;
   step->output.flags = carry ? UTF_CARRY : 0;
}

static void add_version(unsigned int major)
{
   UTAsmInterruptStep *step = add_interrupt(0x21, 0x3000);
   step->set = UTR_AX;
   step->output.ax = major;
}

static int call_without_swap(char *path, unsigned int result,
   unsigned int carry, unsigned int major)
{
   char command[3];
   char environment[2];
   char swap_file[1];
   VECTOR vectors[2];
   command[0] = 0;
   command[1] = '\r';
   command[2] = 0;
   environment[0] = 0;
   environment[1] = 0;
   swap_file[0] = 0;
   prepare_vectors(vectors);
   ut_asm_reset_steps();
   add_filename_parsing();
   add_safevect();
   add_exec_result(result, carry);
   if(carry)
      add_version(major);
   result = (unsigned int)_xspawn(path, command, environment, vectors,
      1, sizeof(environment), swap_file, 0);
   ut_asm_assert_steps_complete();
   return((int)result);
}

static void executes_without_swapping(void)
{
   char path[] = "CHILD.EXE";
   UT_ASSERT_EQ_INT(0, call_without_swap(path, 0, 0, 0));
}

static void preserves_known_dos_two_errors(void)
{
   char path[] = "";
   UT_ASSERT_EQ_INT(18, call_without_swap(path, 18, 1, 2));
}

static void maps_unknown_dos_two_errors(void)
{
   char path[] = "CHILD.EXE";
   UT_ASSERT_EQ_INT(19, call_without_swap(path, 20, 1, 2));
}

static void maps_dos_three_sharing_errors_to_access_denied(void)
{
   char path[] = "CHILD.EXE";
   UT_ASSERT_EQ_INT(5, call_without_swap(path, 32, 1, 3));
}

static void maps_unknown_dos_three_errors(void)
{
   char path[] = "CHILD.EXE";
   UT_ASSERT_EQ_INT(19, call_without_swap(path, 34, 1, 3));
}

static void preserves_known_dos_three_errors(void)
{
   char path[] = "CHILD.EXE";
   UT_ASSERT_EQ_INT(7, call_without_swap(path, 7, 1, 3));
}

static void add_masked_interrupt(unsigned int number, unsigned int ax)
{
   UTAsmInterruptStep *step = add_interrupt(number, ax);
   step->mask.ax = 0xff00;
}

static int call_too_large_swap(char *swap_file, unsigned int handle)
{
   char path[] = "CHILD.EXE";
   char command[3];
   char environment[2];
   VECTOR vectors[2];
   command[0] = 0;
   command[1] = '\r';
   command[2] = 0;
   environment[0] = 0;
   environment[1] = 0;
   prepare_vectors(vectors);
   ut_asm_set_word_psp(aligned_psp_segment());
   ut_asm_set_word_ttlsz(0);
   return(_xspawn(path, command, environment, vectors, 0,
      sizeof(environment), swap_file, handle));
}

static void releases_ems_when_the_parent_cannot_be_swapped(void)
{
   char swap_file[] = "";
   UTAsmInterruptStep *step;
   ut_asm_reset_steps();
   add_filename_parsing();
   step = ut_asm_add_step(0x67);
   step->set = UTR_AX;
   step->output.ax = 0x8200;
   step = ut_asm_add_step(0x67);
   step->set = UTR_AX;
   step->output.ax = 0;
   add_version(3);
   UT_ASSERT_EQ_INT(7, call_too_large_swap(swap_file, 4));
   ut_asm_assert_steps_complete();
}

static UTAsmInterruptStep *add_dependency(unsigned int index)
{
   return(ut_asm_add_dependency_step(index));
}

static void add_kernel_result(unsigned int result, unsigned int handle,
   unsigned int data_segment)
{
   UTAsmInterruptStep *step;
   add_safevect();
   add_safevect();
   step = add_dependency(UT_ASM_DEPENDENCY_XSPAWN);
   step->set = UTR_AX | UTR_BX | UTR_DX | UTR_DS;
   step->output.ax = result;
   step->output.bx = handle;
   step->output.dx = handle;
   step->output.ds = data_segment;
   add_safevect();
}

static void add_ems_release(unsigned int first_result,
   unsigned int second_result)
{
   UTAsmInterruptStep *step = ut_asm_add_step(0x67);
   step->set = UTR_AX;
   step->output.ax = first_result;
   if(first_result == 0x8200)
   {
      step = ut_asm_add_step(0x67);
      step->set = UTR_AX;
      step->output.ax = second_result;
   }
}

static void add_mapems_result(unsigned int result)
{
   UTAsmInterruptStep *step = add_dependency(UT_ASM_DEPENDENCY_MAPEMS);
   step->set = UTR_AX;
   step->output.ax = result;
}

static int call_swap(char *swap_file, unsigned int handle,
   unsigned int count_low, unsigned int count_high)
{
   char path[] = "CHILD.EXE";
   char command[3];
   char environment[2];
   VECTOR vectors[2];
   command[0] = 0;
   command[1] = '\r';
   command[2] = 0;
   environment[0] = 0;
   environment[1] = 0;
   prepare_vectors(vectors);
   ut_asm_set_word_psp(aligned_psp_segment());
   ut_asm_set_word_emsseg(aligned_ems_segment());
   ut_asm_set_word_ttlsz(0xffff);
   ut_asm_set_word_cntl(count_low);
   ut_asm_set_word_cnth(count_high);
   return(_xspawn(path, command, environment, vectors, 0,
      sizeof(environment), swap_file, handle));
}

static void swaps_an_empty_parent_through_ems(void)
{
   char swap_file[] = "";
   unsigned int ems_segment = aligned_ems_segment();
   ut_asm_reset_steps();
   add_filename_parsing();
   add_kernel_result(0, 4, ems_segment);
   add_mapems_result(0);
   add_ems_release(0, 0);
   UT_ASSERT_EQ_INT(0, call_swap(swap_file, 4, 0, 0));
   ut_asm_assert_steps_complete();
}

static void swaps_a_partial_ems_page(void)
{
   char swap_file[] = "";
   unsigned int ems_segment = aligned_ems_segment();
   ut_asm_reset_steps();
   add_filename_parsing();
   add_mapems_result(0);
   add_kernel_result(0, 4, ems_segment);
   add_mapems_result(0);
   add_ems_release(0, 0);
   UT_ASSERT_EQ_INT(0, call_swap(swap_file, 4, 1, 0));
   ut_asm_assert_steps_complete();
}

static void releases_ems_after_an_initial_mapping_failure(void)
{
   char swap_file[] = "";
   ut_asm_reset_steps();
   add_filename_parsing();
   add_mapems_result(0x0100);
   add_ems_release(0, 0);
   UT_ASSERT_EQ_INT(0, call_swap(swap_file, 4, 0, 1));
   ut_asm_assert_steps_complete();
}

static void tries_a_full_ems_page_before_a_mapping_failure(void)
{
   char swap_file[] = "";
   ut_asm_reset_steps();
   add_filename_parsing();
   add_mapems_result(0x0100);
   add_ems_release(0, 0);
   UT_ASSERT_EQ_INT(0, call_swap(swap_file, 4, 16384, 0));
   ut_asm_assert_steps_complete();
}

static void add_disk_cleanup(void)
{
   add_masked_interrupt(0x21, 0x3e00);
   add_masked_interrupt(0x21, 0x4100);
}

static void add_disk_restore(unsigned int read_result,
   unsigned int read_carry, unsigned int exact_length)
{
   UTAsmInterruptStep *step = add_interrupt(0x21, 0x4200);
   step->check |= UTR_CX | UTR_DX;
   step->expected.cx = 0;
   step->expected.dx = 0;
   step = add_interrupt(0x21, 0x3f00);
   step->mask.ax = 0xff00;
   step->set = UTR_AX | UTR_FLAGS;
   step->output.ax = read_result;
   step->output.flags = read_carry ? UTF_CARRY : 0;
   step->copy_cx_to_ax = exact_length;
   if(read_carry || !exact_length)
   {
      add_masked_interrupt(0x21, 0x4000);
      add_interrupt(0x21, 0x4c01);
   }
   add_disk_cleanup();
}

static void swaps_a_partial_disk_block_and_restores_it(void)
{
   char swap_file[] = "SWAP.$$$";
   UTAsmInterruptStep *step;
   ut_asm_reset_steps();
   add_filename_parsing();
   step = add_interrupt(0x21, 0x4000);
   step->mask.ax = 0xff00;
   step->check |= UTR_CX;
   step->expected.cx = 1;
   step->set = UTR_AX | UTR_FLAGS;
   step->output.ax = 1;
   step->output.flags = 0;
   add_masked_interrupt(0x21, 0x3e00);
   add_kernel_result(0, 9, 0);
   add_disk_restore(0, 0, 1);
   UT_ASSERT_EQ_INT(0, call_swap(swap_file, 9, 1, 0));
   ut_asm_assert_steps_complete();
}

static void reports_a_disk_write_error(void)
{
   char swap_file[] = "SWAP.$$$";
   UTAsmInterruptStep *step;
   ut_asm_reset_steps();
   add_filename_parsing();
   step = add_interrupt(0x21, 0x4000);
   step->mask.ax = 0xff00;
   step->set = UTR_AX | UTR_FLAGS;
   step->output.ax = 5;
   step->output.flags = UTF_CARRY;
   add_masked_interrupt(0x21, 0x3e00);
   add_version(3);
   UT_ASSERT_EQ_INT(5, call_swap(swap_file, 9, 0, 1));
   ut_asm_assert_steps_complete();
}

static void reports_a_short_disk_write(void)
{
   char swap_file[] = "SWAP.$$$";
   UTAsmInterruptStep *step;
   ut_asm_reset_steps();
   add_filename_parsing();
   step = add_interrupt(0x21, 0x4000);
   step->mask.ax = 0xff00;
   step->set = UTR_AX | UTR_FLAGS;
   step->output.ax = 1;
   step->output.flags = 0;
   add_masked_interrupt(0x21, 0x3e00);
   add_version(3);
   UT_ASSERT_EQ_INT(5, call_swap(swap_file, 9, 65520, 0));
   ut_asm_assert_steps_complete();
}

static void handles_a_disk_restore_error(void)
{
   char swap_file[] = "SWAP.$$$";
   ut_asm_reset_steps();
   add_filename_parsing();
   add_masked_interrupt(0x21, 0x3e00);
   add_kernel_result(0, 9, 0);
   add_disk_restore(5, 1, 0);
   UT_ASSERT_EQ_INT(0, call_swap(swap_file, 9, 0, 0));
   ut_asm_assert_steps_complete();
}

static void handles_a_short_disk_restore(void)
{
   char swap_file[] = "SWAP.$$$";
   ut_asm_reset_steps();
   add_filename_parsing();
   add_masked_interrupt(0x21, 0x3e00);
   add_kernel_result(0, 9, 0);
   add_disk_restore(1, 0, 0);
   UT_ASSERT_EQ_INT(0, call_swap(swap_file, 9, 0, 0));
   ut_asm_assert_steps_complete();
}

static void handles_an_ems_restore_mapping_failure(void)
{
   char swap_file[] = "";
   unsigned int ems_segment = aligned_ems_segment();
   ut_asm_reset_steps();
   add_filename_parsing();
   add_kernel_result(0, 4, ems_segment);
   add_mapems_result(0x0100);
   add_ems_release(0x8200, 0);
   add_masked_interrupt(0x21, 0x4000);
   add_interrupt(0x21, 0x4c01);
   add_disk_cleanup();
   UT_ASSERT_EQ_INT(0, call_swap(swap_file, 4, 0, 0));
   ut_asm_assert_steps_complete();
}

static void removes_the_swap_file_when_the_parent_cannot_be_swapped(void)
{
   char swap_file[] = "SWAP.$$$";
   ut_asm_reset_steps();
   add_filename_parsing();
   add_masked_interrupt(0x21, 0x3e00);
   add_masked_interrupt(0x21, 0x4100);
   add_version(3);
   UT_ASSERT_EQ_INT(7, call_too_large_swap(swap_file, 9));
   ut_asm_assert_steps_complete();
}

static const UTTestCase ut_cases[] = {
   {"no-swap success", executes_without_swapping},
   {"DOS 2 known error", preserves_known_dos_two_errors},
   {"DOS 2 unknown error", maps_unknown_dos_two_errors},
   {"DOS 3 sharing error", maps_dos_three_sharing_errors_to_access_denied},
   {"DOS 3 unknown error", maps_unknown_dos_three_errors},
   {"DOS 3 known error", preserves_known_dos_three_errors},
   {"oversized EMS swap", releases_ems_when_the_parent_cannot_be_swapped},
   {"oversized disk swap", removes_the_swap_file_when_the_parent_cannot_be_swapped},
   {"empty EMS swap", swaps_an_empty_parent_through_ems},
   {"partial EMS page", swaps_a_partial_ems_page},
   {"initial EMS map failure", releases_ems_after_an_initial_mapping_failure},
   {"full EMS page", tries_a_full_ems_page_before_a_mapping_failure},
   {"partial disk block", swaps_a_partial_disk_block_and_restores_it},
   {"disk write error", reports_a_disk_write_error},
   {"short disk write", reports_a_short_disk_write},
   {"disk read error", handles_a_disk_restore_error},
   {"short disk read", handles_a_short_disk_restore},
   {"EMS restore map failure", handles_an_ems_restore_mapping_failure}
};
#endif
