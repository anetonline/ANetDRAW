#include "register_call.h"

static void reports_interrupt_and_signature_results(void)
{
   ut_reset_register_call(0x0400, 2);
   ut_call_result = FALSE;
   UT_ASSERT(!utt_OD32FossilDetect(2));
   ut_assert_one_register_call();

   ut_reset_register_call(0x0400, 2);
   ut_return_eax = 0x1953;
   UT_ASSERT(!utt_OD32FossilDetect(2));
   ut_assert_one_register_call();

   ut_reset_register_call(0x0400, 2);
   ut_return_eax = 0xffff1954UL;
   UT_ASSERT(utt_OD32FossilDetect(2));
   ut_assert_one_register_call();
}

static const UTTestCase ut_cases[] = {
   {"detect result", reports_interrupt_and_signature_results}
};
