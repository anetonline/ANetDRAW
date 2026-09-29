#include "register_call.h"

static void combines_flow_mode_with_function(void)
{
   ut_reset_register_call(0x0fab, 4);
   utt_OD32FossilSetFlow(4, 0xab);
   ut_assert_one_register_call();
}

static const UTTestCase ut_cases[] = {
   {"flow registers", combines_flow_mode_with_function}
};
