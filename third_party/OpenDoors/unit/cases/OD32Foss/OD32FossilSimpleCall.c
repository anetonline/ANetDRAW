#include "register_call.h"

static void places_function_in_ah(void)
{
   ut_reset_register_call(0xab00, 7);
   utt_OD32FossilSimpleCall(7, 0xab);
   ut_assert_one_register_call();
}

static const UTTestCase ut_cases[] = {
   {"simple-call registers", places_function_in_ah}
};
