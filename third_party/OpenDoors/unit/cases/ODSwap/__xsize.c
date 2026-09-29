#ifdef ODPLAT_DOS
#include "common.h"

/* UT_ASM_EXTERNAL_RANGE fnish7 fnish11 */

unsigned int ODSWAPCALL ut_asm_get_word_psp(void);
unsigned int ODSWAPCALL ut_asm_get_word_parsz(void);
unsigned int ODSWAPCALL ut_asm_get_word_cntl(void);
unsigned int ODSWAPCALL ut_asm_get_word_cnth(void);
unsigned int ODSWAPCALL ut_asm_get_word_ttlsz(void);

static unsigned char arena_space[1024];

static unsigned int aligned_arena_segment(void)
{
   unsigned int segment = UT_FP_SEG(arena_space);
   unsigned int offset = UT_FP_OFF(arena_space);
   return((unsigned int)(segment + (offset + 15U) / 16U));
}

static void write_word(unsigned char far *location, unsigned int value)
{
   location[0] = (unsigned char)value;
   location[1] = (unsigned char)(value >> 8);
}

static void write_arena(unsigned int segment, unsigned char signature,
   unsigned int owner, unsigned int paragraphs)
{
   unsigned char far *header = (unsigned char far *)UT_MK_FP(segment, 0);
   header[0] = signature;
   write_word(header + 1, owner);
   write_word(header + 3, paragraphs);
}

static void prepare_valid_arenas(unsigned int psp, unsigned int second_owner)
{
   write_arena((unsigned int)(psp - 1), 'M', psp, 0x20);
   write_arena((unsigned int)(psp + 0x20), 'Z', second_owner, 0x10);
}

static void checks_sizes(unsigned int second_owner)
{
   unsigned int psp = (unsigned int)(aligned_arena_segment() + 1);
   long swap_size = -1;
   long total_size = -1;
   prepare_valid_arenas(psp, second_owner == 0xffff ? psp + 1 : 0);
   ut_asm_reset_steps();
   UT_ASSERT_EQ_INT(0, _xsize(psp, &swap_size, &total_size));
   UT_ASSERT(swap_size == 256L);
   UT_ASSERT(total_size == (second_owner == 0xffff ? 512L : 784L));
   UT_ASSERT_EQ_UINT(psp, ut_asm_get_word_psp());
   UT_ASSERT_EQ_UINT(0x20, ut_asm_get_word_parsz());
   UT_ASSERT_EQ_UINT(256, ut_asm_get_word_cntl());
   UT_ASSERT_EQ_UINT(0, ut_asm_get_word_cnth());
   UT_ASSERT_EQ_UINT(second_owner == 0xffff ? 0x20 : 0x31,
      ut_asm_get_word_ttlsz());
   ut_asm_assert_steps_complete();
}

static void sizes_a_last_owned_block_followed_by_free_memory(void)
{
   checks_sizes(0);
}

static void sizes_an_owned_block_followed_by_foreign_memory(void)
{
   checks_sizes(0xffff);
}

static void add_corrupt_arena_steps(unsigned int psp_low_byte)
{
   UTAsmInterruptStep *step = ut_asm_add_step(0x21);
   step->check = UTR_AX | UTR_BX;
   step->expected.ax = (unsigned int)(0x4800 | psp_low_byte);
   step->mask.ax = 0xff00;
   step->expected.bx = 0xffff;
   step->set = UTR_AX | UTR_FLAGS;
   step->output.ax = 8;
   step->output.flags = UTF_CARRY;
   step = ut_asm_add_step(0x21);
   step->check = UTR_AX;
   step->expected.ax = 0x3000;
   step->set = UTR_AX;
   step->output.ax = 3;
}

static void returns_seven_for_an_invalid_arena_signature(void)
{
   unsigned int psp = (unsigned int)(aligned_arena_segment() + 1);
   long first = -1;
   long second = -1;
   write_arena((unsigned int)(psp - 1), 'X', psp, 0x20);
   ut_asm_reset_steps();
   add_corrupt_arena_steps(psp & 0xff);
   UT_ASSERT_EQ_INT(7, _xsize(psp, &first, &second));
   ut_asm_assert_steps_complete();
}

static void returns_seven_when_arena_arithmetic_wraps(void)
{
   unsigned int psp = (unsigned int)(aligned_arena_segment() + 1);
   long first = -1;
   long second = -1;
   write_arena((unsigned int)(psp - 1), 'M', psp, 0xffff);
   ut_asm_reset_steps();
   add_corrupt_arena_steps(psp & 0xff);
   UT_ASSERT_EQ_INT(7, _xsize(psp, &first, &second));
   ut_asm_assert_steps_complete();
}

static const UTTestCase ut_cases[] = {
   {"free memory", sizes_a_last_owned_block_followed_by_free_memory},
   {"foreign memory", sizes_an_owned_block_followed_by_foreign_memory},
   {"invalid signature", returns_seven_for_an_invalid_arena_signature},
   {"arena wrap", returns_seven_when_arena_arithmetic_wraps}
};
#endif
