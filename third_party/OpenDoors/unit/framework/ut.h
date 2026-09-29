/* OpenDoors isolated unit-test runtime.  This header is intentionally C89. */
#ifndef OPENDOORS_UNIT_UT_H
#define OPENDOORS_UNIT_UT_H

#include <stddef.h>

#define UT_MAX_FAILURES 32
#define UT_MAX_CALLS 512
#define UT_MAX_DECISION_DEPTH 16
#define UT_MAX_SWITCH_DEPTH 16
#define UT_MAX_FUNCTIONS 512
#define UT_MAX_STATES 512
#define UT_MAX_ASM_BRANCHES 512
#define UT_TEST_NAME_SIZE 48

typedef void (*UTTestFunction)(void);

typedef struct UTTestCase
{
   char name[UT_TEST_NAME_SIZE];
   UTTestFunction function;
} UTTestCase;

typedef struct UTDecisionObservation
{
   unsigned short decision;
   unsigned char condition_count;
   unsigned long evaluated;
   unsigned long values;
   unsigned char result;
} UTDecisionObservation;

typedef struct UTBranchObservation
{
   unsigned short branch;
   unsigned char outcome;
} UTBranchObservation;

extern unsigned short ut_failure_count;
extern unsigned short ut_mock_call_count;
extern unsigned short ut_mock_calls[UT_MAX_CALLS];

void ut_reset(void);
void ut_state_register(void *address, size_t size);
void ut_state_restore(void);
void ut_fail(const char *file, unsigned line, const char *expression);
void ut_mock_called(unsigned short mock);
void ut_unexpected_mock(unsigned short mock, const char *name);
unsigned short ut_mock_count(unsigned short mock);

void ut_cov_function(unsigned short function_id);
int ut_cov_begin(unsigned short decision);
int ut_cov_condition(unsigned short decision, unsigned char condition,
   int value);
int ut_cov_result(unsigned short decision, unsigned char condition_count,
   int value);
void ut_cov_branch(unsigned short branch, int outcome);
void ut_cov_asm_register(unsigned char *observations,
   unsigned short branch_count);
void ut_cov_switch_begin(unsigned short switch_id, unsigned long value);
void ut_cov_switch_case_value(unsigned short switch_id,
   unsigned short branch, unsigned long value);
unsigned long ut_cov_switch_result(unsigned short switch_id,
   unsigned short default_branch);
int ut_run(const UTTestCase *tests, unsigned short count,
   const char *report_path);

#define UT_ASSERT(expression) \
   ((expression) ? (void)0 : ut_fail(__FILE__, __LINE__, #expression))
#define UT_ASSERT_EQ_INT(expected, actual) \
   UT_ASSERT((int)(expected) == (int)(actual))
#define UT_ASSERT_EQ_UINT(expected, actual) \
   UT_ASSERT((unsigned)(expected) == (unsigned)(actual))
#define UT_ASSERT_EQ_PTR(expected, actual) \
   UT_ASSERT((const void *)(expected) == (const void *)(actual))
#define UT_ASSERT_NULL(actual) UT_ASSERT((actual) == NULL)
#define UT_ASSERT_NOT_NULL(actual) UT_ASSERT((actual) != NULL)

#endif
