#include "ut.h"

static int source_state = 5;

static void passing_assertions(void)
{
   int value = 7;
   UT_ASSERT(value != 0);
   UT_ASSERT_EQ_INT(7, value);
   UT_ASSERT_EQ_UINT(7U, (unsigned)value);
   UT_ASSERT_NOT_NULL(&value);
}

static void mock_log_is_ordered(void)
{
   ut_mock_called(4);
   ut_mock_called(9);
   ut_mock_called(4);
   UT_ASSERT_EQ_UINT(3, ut_mock_call_count);
   UT_ASSERT_EQ_UINT(4, ut_mock_calls[0]);
   UT_ASSERT_EQ_UINT(9, ut_mock_calls[1]);
   UT_ASSERT_EQ_UINT(2, ut_mock_count(4));
}

static void decisions_retain_short_circuit_masking(void)
{
   int result;
   result = (ut_cov_begin(11), ut_cov_result(11, 2,
      ut_cov_condition(11, 0, 0) && ut_cov_condition(11, 1, 1)));
   UT_ASSERT_EQ_INT(0, result);
   result = (ut_cov_begin(11), ut_cov_result(11, 2,
      ut_cov_condition(11, 0, 1) && ut_cov_condition(11, 1, 1)));
   UT_ASSERT_EQ_INT(1, result);
   ut_cov_branch(12, 0);
   ut_cov_branch(12, 1);
   ut_cov_function(3);
}

static void source_state_can_change_within_a_case(void)
{
   UT_ASSERT_EQ_INT(5, source_state);
   source_state = 9;
   UT_ASSERT_EQ_INT(9, source_state);
}

static void source_state_is_restored_between_cases(void)
{
   UT_ASSERT_EQ_INT(5, source_state);
}

static void switch_dispatch_records_selected_and_default_arms(void)
{
   unsigned long value;
   ut_cov_switch_begin(2, 7);
   ut_cov_switch_case_value(2, 21, 3);
   ut_cov_switch_case_value(2, 22, 7);
   value = ut_cov_switch_result(2, 23);
   UT_ASSERT_EQ_UINT(7, value);
   ut_cov_switch_begin(2, 9);
   ut_cov_switch_case_value(2, 21, 3);
   ut_cov_switch_case_value(2, 22, 7);
   value = ut_cov_switch_result(2, 23);
   UT_ASSERT_EQ_UINT(9, value);
}

int main(int argc, char **argv)
{
   static const UTTestCase tests[] = {
      {"passing assertions", passing_assertions},
      {"mock log is ordered", mock_log_is_ordered},
      {"decision masking", decisions_retain_short_circuit_masking},
      {"state changes", source_state_can_change_within_a_case},
      {"state restoration", source_state_is_restored_between_cases},
      {"switch dispatch", switch_dispatch_records_selected_and_default_arms}
   };
   const char *report = argc > 1 ? argv[1] : NULL;
   ut_state_register(&source_state, sizeof(source_state));
   return ut_run(tests, (unsigned short)(sizeof(tests) / sizeof(tests[0])),
      report);
}
