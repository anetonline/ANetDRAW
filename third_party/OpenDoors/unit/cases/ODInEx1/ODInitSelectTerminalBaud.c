#define UT_CUSTOM_MOCK_ODInitTerminalSpeedToBaud

static DWORD ut_results[2];
static speed_t ut_speeds[2];
static unsigned ut_calls;

DWORD utm_ODInitTerminalSpeedToBaud(speed_t speed)
{
   unsigned index = ut_calls++;
   UT_ASSERT(index < 2);
   ut_speeds[index] = speed;
   return ut_results[index];
}

static void uses_a_recognized_input_speed_without_testing_output(void)
{
   ut_calls = 0;
   ut_results[0] = 9600;
   UT_ASSERT_EQ_UINT(9600,
      utt_ODInitSelectTerminalBaud((speed_t)1, (speed_t)2));
   UT_ASSERT_EQ_UINT(1, ut_calls);
   UT_ASSERT_EQ_UINT(1, ut_speeds[0]);
}

static void falls_back_to_a_recognized_output_speed(void)
{
   ut_calls = 0;
   ut_results[0] = 0;
   ut_results[1] = 38400;
   UT_ASSERT_EQ_UINT(38400,
      utt_ODInitSelectTerminalBaud((speed_t)3, (speed_t)4));
   UT_ASSERT_EQ_UINT(2, ut_calls);
   UT_ASSERT_EQ_UINT(3, ut_speeds[0]);
   UT_ASSERT_EQ_UINT(4, ut_speeds[1]);
}

static void uses_the_nominal_fallback_when_both_are_unknown(void)
{
   ut_calls = 0;
   ut_results[0] = 0;
   ut_results[1] = 0;
   UT_ASSERT_EQ_UINT(19200,
      utt_ODInitSelectTerminalBaud((speed_t)5, (speed_t)6));
   UT_ASSERT_EQ_UINT(2, ut_calls);
}

static const UTTestCase ut_cases[] = {
   {"input speed", uses_a_recognized_input_speed_without_testing_output},
   {"output speed", falls_back_to_a_recognized_output_speed},
   {"nominal fallback", uses_the_nominal_fallback_when_both_are_unknown}
};
