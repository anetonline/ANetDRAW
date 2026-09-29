#include "register_call.h"

static void returns_failure_or_low_status_word(void)
{
   ut_reset_register_call(0x0300, 5);
   ut_call_result = FALSE;
   UT_ASSERT_EQ_UINT(0, utt_OD32FossilStatus(5));
   ut_assert_one_register_call();

   ut_reset_register_call(0x0300, 5);
   ut_return_eax = 0xabcd1234UL;
   UT_ASSERT_EQ_UINT(0x1234, utt_OD32FossilStatus(5));
   ut_assert_one_register_call();
}

static const UTTestCase ut_cases[] = {
   {"status result", returns_failure_or_low_status_word}
};
