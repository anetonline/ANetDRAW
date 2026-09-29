#include "register_call.h"

static void passes_close_function_and_port(void)
{
   ut_reset_register_call(0x0500, 1);
   utt_OD32FossilClose(1);
   ut_assert_one_register_call();
}

static const UTTestCase ut_cases[] = {
   {"close registers", passes_close_function_and_port}
};
