#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed

static void *ut_freed[3];
static unsigned ut_free_count;
static BOOL ut_public_call_allowed = TRUE;

BOOL utm_ODSyncPublicCallAllowed(void)
{
   if(!ut_public_call_allowed)
      od_control.od_error = ERR_GENERALFAILURE;
   return ut_public_call_allowed;
}

void utm_free(void *memory)
{
   if(ut_free_count < sizeof(ut_freed) / sizeof(ut_freed[0]))
      ut_freed[ut_free_count++] = memory;
}

static void frees_both_allocations_and_vector(void)
{
   char first;
   char second;
   char *arguments[3];
   ut_free_count = 0;
   arguments[0] = &first;
   arguments[1] = &second;
   utt_od_free_split_cmd_line(arguments);
   UT_ASSERT_EQ_UINT(3, ut_free_count);
   UT_ASSERT_EQ_PTR(&first, ut_freed[0]);
   UT_ASSERT_EQ_PTR(&second, ut_freed[1]);
   UT_ASSERT_EQ_PTR(arguments, ut_freed[2]);
}

static void rejects_null_vector(void)
{
   od_control.od_error = 0;
   ut_free_count = 0;
   utt_od_free_split_cmd_line(NULL);
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_free_count);
}

static void rejects_a_terminal_session(void)
{
   char *arguments[3];
   ut_public_call_allowed = FALSE;
   ut_free_count = 0;
   utt_od_free_split_cmd_line(arguments);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_free_count);
   ut_public_call_allowed = TRUE;
}

static const UTTestCase ut_cases[] = {
   {"terminal session", rejects_a_terminal_session},
   {"free split command line", frees_both_allocations_and_vector},
   {"null vector", rejects_null_vector}
};
