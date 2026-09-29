/* OpenDoors isolated unit-test runtime.  Keep this file valid C89. */
#include "ut.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct UTDecisionFrame
{
   unsigned short decision;
   unsigned long evaluated;
   unsigned long values;
} UTDecisionFrame;

typedef struct UTSwitchFrame
{
   unsigned short switch_id;
   unsigned short selected_branch;
   unsigned long value;
} UTSwitchFrame;

typedef struct UTStateEntry
{
   void *address;
   void *initial;
   size_t size;
} UTStateEntry;

unsigned short ut_failure_count;
unsigned short ut_mock_call_count;
unsigned short ut_mock_calls[UT_MAX_CALLS];

static UTDecisionFrame ut_decision_stack[UT_MAX_DECISION_DEPTH];
static unsigned short ut_decision_depth;
static UTSwitchFrame ut_switch_stack[UT_MAX_SWITCH_DEPTH];
static unsigned short ut_switch_depth;
static UTStateEntry ut_states[UT_MAX_STATES];
static unsigned short ut_state_count;
static int ut_state_error;
static FILE *ut_coverage_output;
static unsigned char *ut_asm_observations;
static unsigned short ut_asm_branch_count;

void ut_reset(void)
{
   ut_state_restore();
   ut_failure_count = 0;
   ut_mock_call_count = 0;
   ut_decision_depth = 0;
   ut_switch_depth = 0;
}

void ut_state_register(void *address, size_t size)
{
   UTStateEntry *entry;
   unsigned short index;
   for(index = 0; index < ut_state_count; ++index)
   {
      if(ut_states[index].address == address)
         return;
   }
   if(ut_state_count >= UT_MAX_STATES)
   {
      printf("too many registered source-state objects\n");
      ut_state_error = 1;
      return;
   }
   entry = &ut_states[ut_state_count];
   entry->initial = malloc(size == 0 ? 1 : size);
   if(entry->initial == NULL)
   {
      printf("unable to snapshot source-state object\n");
      ut_state_error = 1;
      return;
   }
   entry->address = address;
   entry->size = size;
   if(size != 0)
      memcpy(entry->initial, address, size);
   ++ut_state_count;
}

void ut_state_restore(void)
{
   unsigned short index;
   for(index = 0; index < ut_state_count; ++index)
   {
      if(ut_states[index].size != 0)
         memcpy(ut_states[index].address, ut_states[index].initial,
            ut_states[index].size);
   }
}

void ut_fail(const char *file, unsigned line, const char *expression)
{
   if(ut_failure_count < UT_MAX_FAILURES)
   {
      printf("%s:%u: assertion failed: %s\n", file, line,
         expression);
   }
   ++ut_failure_count;
}

void ut_mock_called(unsigned short mock)
{
   if(ut_mock_call_count >= UT_MAX_CALLS)
   {
      ut_fail(__FILE__, __LINE__, "mock call log is full");
      return;
   }
   ut_mock_calls[ut_mock_call_count++] = mock;
}

void ut_unexpected_mock(unsigned short mock, const char *name)
{
   ut_mock_called(mock);
   if(ut_failure_count < UT_MAX_FAILURES)
      printf("unexpected mock call: %s\n", name);
   ++ut_failure_count;
}

unsigned short ut_mock_count(unsigned short mock)
{
   unsigned short index;
   unsigned short count = 0;
   for(index = 0; index < ut_mock_call_count; ++index)
   {
      if(ut_mock_calls[index] == mock)
         ++count;
   }
   return count;
}

void ut_cov_function(unsigned short function_id)
{
   if(function_id >= UT_MAX_FUNCTIONS)
   {
      ut_fail(__FILE__, __LINE__, "function coverage id is out of range");
      return;
   }
   if(ut_coverage_output != NULL)
      fprintf(ut_coverage_output, "F %u\n", function_id);
}

int ut_cov_begin(unsigned short decision)
{
   UTDecisionFrame *frame;
   if(ut_decision_depth >= UT_MAX_DECISION_DEPTH)
   {
      ut_fail(__FILE__, __LINE__, "decision stack is full");
      return 0;
   }
   frame = &ut_decision_stack[ut_decision_depth++];
   frame->decision = decision;
   frame->evaluated = 0;
   frame->values = 0;
   return 0;
}

int ut_cov_condition(unsigned short decision, unsigned char condition,
   int value)
{
   UTDecisionFrame *frame;
   unsigned long bit;
   if(ut_decision_depth == 0)
   {
      ut_fail(__FILE__, __LINE__, "condition without active decision");
      return value != 0;
   }
   frame = &ut_decision_stack[ut_decision_depth - 1];
   if(frame->decision != decision || condition >= sizeof(unsigned long) * 8)
   {
      ut_fail(__FILE__, __LINE__, "condition does not match decision");
      return value != 0;
   }
   bit = 1UL << condition;
   frame->evaluated |= bit;
   if(value)
      frame->values |= bit;
   return value != 0;
}

int ut_cov_result(unsigned short decision, unsigned char condition_count,
   int value)
{
   UTDecisionFrame *frame;
   if(ut_decision_depth == 0 ||
      ut_decision_stack[ut_decision_depth - 1].decision != decision)
   {
      ut_fail(__FILE__, __LINE__, "result does not match decision");
      return value != 0;
   }
   frame = &ut_decision_stack[--ut_decision_depth];
   if(ut_coverage_output != NULL)
   {
      fprintf(ut_coverage_output, "D %u %u %lu %lu %u\n", decision,
         (unsigned)condition_count, frame->evaluated, frame->values,
         (unsigned)(value != 0));
   }
   return value != 0;
}

void ut_cov_branch(unsigned short branch, int outcome)
{
   if(ut_coverage_output != NULL)
      fprintf(ut_coverage_output, "B %u %u\n", branch,
         (unsigned)(outcome != 0));
}

void ut_cov_asm_register(unsigned char *observations,
   unsigned short branch_count)
{
   if(branch_count > UT_MAX_ASM_BRANCHES)
   {
      ut_fail(__FILE__, __LINE__, "too many assembly branches");
      return;
   }
   ut_asm_observations = observations;
   ut_asm_branch_count = branch_count;
}

static void ut_cov_asm_flush(void)
{
   unsigned short branch;
   unsigned short outcome;
   if(ut_asm_observations == NULL)
      return;
   for(branch = 0; branch < ut_asm_branch_count; ++branch)
   {
      for(outcome = 0; outcome < 2; ++outcome)
      {
         unsigned short offset = (unsigned short)(branch * 2 + outcome);
         if(ut_asm_observations[offset] != 0)
         {
            if(ut_coverage_output != NULL)
               fprintf(ut_coverage_output, "A %u %u\n",
                  (unsigned)(branch + 1), (unsigned)outcome);
            ut_asm_observations[offset] = 0;
         }
      }
   }
}

void ut_cov_switch_begin(unsigned short switch_id, unsigned long value)
{
   UTSwitchFrame *frame;
   if(ut_switch_depth >= UT_MAX_SWITCH_DEPTH)
   {
      ut_fail(__FILE__, __LINE__, "switch stack is full");
      return;
   }
   frame = &ut_switch_stack[ut_switch_depth++];
   frame->switch_id = switch_id;
   frame->selected_branch = 0;
   frame->value = value;
}

void ut_cov_switch_case_value(unsigned short switch_id,
   unsigned short branch, unsigned long value)
{
   UTSwitchFrame *frame;
   if(ut_switch_depth == 0)
   {
      ut_fail(__FILE__, __LINE__, "switch case without active switch");
      return;
   }
   frame = &ut_switch_stack[ut_switch_depth - 1];
   if(frame->switch_id != switch_id)
   {
      ut_fail(__FILE__, __LINE__, "switch case does not match switch");
      return;
   }
   if(frame->selected_branch == 0 && frame->value == value)
      frame->selected_branch = branch;
}

unsigned long ut_cov_switch_result(unsigned short switch_id,
   unsigned short default_branch)
{
   UTSwitchFrame *frame;
   unsigned long value;
   if(ut_switch_depth == 0 ||
      ut_switch_stack[ut_switch_depth - 1].switch_id != switch_id)
   {
      ut_fail(__FILE__, __LINE__, "switch result does not match switch");
      return 0;
   }
   frame = &ut_switch_stack[--ut_switch_depth];
   value = frame->value;
   ut_cov_branch(frame->selected_branch != 0 ? frame->selected_branch :
      default_branch, 1);
   return value;
}

int ut_run(const UTTestCase *tests, unsigned short count,
   const char *report_path)
{
   unsigned short index;
   unsigned short failed_tests = 0;
   if(ut_state_error)
      return 1;
   ut_coverage_output = NULL;
   if(report_path != NULL)
   {
      ut_coverage_output = fopen(report_path, "w");
      if(ut_coverage_output == NULL)
      {
         printf("unable to write coverage report: %s\n", report_path);
         return 1;
      }
      fprintf(ut_coverage_output, "OPENDOORS-UNIT-COVERAGE 1\n");
   }
   for(index = 0; index < count; ++index)
   {
      ut_reset();
      tests[index].function();
      ut_cov_asm_flush();
      if(ut_failure_count)
      {
         ++failed_tests;
         printf("FAIL %s (%u assertion%s)\n", tests[index].name,
            ut_failure_count, ut_failure_count == 1 ? "" : "s");
      }
      else
      {
         printf("PASS %s\n", tests[index].name);
      }
   }
   if(ut_coverage_output != NULL && fclose(ut_coverage_output) != 0)
   {
      printf("unable to write coverage report: %s\n", report_path);
      ++failed_tests;
   }
   ut_coverage_output = NULL;
   printf("%u test%s, %u failure%s\n", count, count == 1 ? "" : "s",
      failed_tests, failed_tests == 1 ? "" : "s");
   return failed_tests == 0 ? 0 : 1;
}
