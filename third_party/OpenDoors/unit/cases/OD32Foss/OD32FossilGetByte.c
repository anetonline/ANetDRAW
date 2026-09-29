#include "register_call.h"

static void returns_low_byte_from_fossil(void)
{
   ut_reset_register_call(0x0200, 8);
   ut_return_eax = 0x123456abUL;
   UT_ASSERT_EQ_UINT(0xab, utt_OD32FossilGetByte(8));
   ut_assert_one_register_call();
}

static const UTTestCase ut_cases[] = {
   {"received byte", returns_low_byte_from_fossil}
};
