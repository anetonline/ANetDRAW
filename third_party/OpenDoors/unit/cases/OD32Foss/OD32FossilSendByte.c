#include "register_call.h"

static void reports_interrupt_and_transmit_results(void)
{
   ut_reset_register_call(0x0b5a, 9);
   ut_call_result = FALSE;
   UT_ASSERT(!utt_OD32FossilSendByte(9, 0x5a));
   ut_assert_one_register_call();

   ut_reset_register_call(0x0b5a, 9);
   ut_return_eax = 0;
   UT_ASSERT(!utt_OD32FossilSendByte(9, 0x5a));
   ut_assert_one_register_call();

   ut_reset_register_call(0x0b5a, 9);
   ut_return_eax = 0xffff0001UL;
   UT_ASSERT(utt_OD32FossilSendByte(9, 0x5a));
   ut_assert_one_register_call();
}

static const UTTestCase ut_cases[] = {
   {"send result", reports_interrupt_and_transmit_results}
};
