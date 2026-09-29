#include "register_call.h"

static void passes_settings_and_port(void)
{
   ut_reset_register_call(0x00e3, 3);
   utt_OD32FossilInitialize(3, 0xe3);
   ut_assert_one_register_call();
}

static const UTTestCase ut_cases[] = {
   {"initialize registers", passes_settings_and_port}
};
