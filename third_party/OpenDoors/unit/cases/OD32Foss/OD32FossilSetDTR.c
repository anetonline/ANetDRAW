#include "register_call.h"

static void maps_boolean_to_dtr_value(void)
{
   ut_reset_register_call(0x0600, 6);
   utt_OD32FossilSetDTR(6, FALSE);
   ut_assert_one_register_call();

   ut_reset_register_call(0x0601, 6);
   utt_OD32FossilSetDTR(6, TRUE);
   ut_assert_one_register_call();
}

static const UTTestCase ut_cases[] = {
   {"DTR registers", maps_boolean_to_dtr_value}
};
