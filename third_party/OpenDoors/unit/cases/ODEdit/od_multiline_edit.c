#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_ODEditSetupInstance
#define UT_CUSTOM_MOCK_ODEditCleanupInstance
#define UT_CUSTOM_MOCK_ODEditBufferFormatAndIndex
#define UT_CUSTOM_MOCK_ODStatStartArrowUse
#define UT_CUSTOM_MOCK_ODStatEndArrowUse
#define UT_CUSTOM_MOCK_ODWaitDrain
#define UT_CUSTOM_MOCK_ODEditRedrawArea
#define UT_CUSTOM_MOCK_ODEditMainLoop

static unsigned ut_init_calls;
static unsigned ut_entry_calls;
static unsigned ut_exit_calls;
static BOOL ut_setup_result;
static unsigned ut_setup_calls;
static char *ut_setup_buffer;
static UINT ut_setup_size;
static tODEditOptions *ut_setup_options;
static tODEditOptions ut_default_options;
static char ut_reallocated_buffer[8];
static unsigned ut_cleanup_calls;
static BOOL ut_format_result;
static unsigned ut_format_calls;
static unsigned ut_arrow_start_calls;
static unsigned ut_arrow_end_calls;
static unsigned ut_drain_calls;
static BOOL ut_end_session_during_drain;
static unsigned ut_redraw_calls;
static INT ut_main_result;
static unsigned ut_main_calls;
static BOOL ut_init_succeeds;

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void)
{
   ++ut_entry_calls;
}

void utm_ODSyncAPIExit(void)
{
   ++ut_exit_calls;
}

BOOL utm_ODEditSetupInstance(tEditInstance *instance, char *buffer,
   UINT size, tODEditOptions *options)
{
   ++ut_setup_calls;
   ut_setup_buffer = buffer;
   ut_setup_size = size;
   ut_setup_options = options;
   instance->pszEditBuffer = ut_reallocated_buffer;
   instance->unBufferSize = sizeof(ut_reallocated_buffer);
   instance->pUserOptions = options != NULL ? options : &ut_default_options;
   instance->pRememberBuffer = NULL;
   instance->papchStartOfLine = NULL;
   return(ut_setup_result);
}

void utm_ODEditCleanupInstance(tEditInstance *instance)
{
   UT_ASSERT_NOT_NULL(instance);
   ++ut_cleanup_calls;
}

BOOL utm_ODEditBufferFormatAndIndex(tEditInstance *instance)
{
   UT_ASSERT_NOT_NULL(instance);
   ++ut_format_calls;
   return(ut_format_result);
}

void utm_ODStatStartArrowUse(void)
{
   ++ut_arrow_start_calls;
}

void utm_ODStatEndArrowUse(void)
{
   ++ut_arrow_end_calls;
}

void utm_ODWaitDrain(tODMilliSec timeout)
{
   UT_ASSERT(timeout == PRE_DRAIN_TIME);
   ++ut_drain_calls;
   if(ut_end_session_during_drain)
      bODInitialized = FALSE;
}

void utm_ODEditRedrawArea(tEditInstance *instance)
{
   UT_ASSERT_NOT_NULL(instance);
   ++ut_redraw_calls;
}

INT utm_ODEditMainLoop(tEditInstance *instance)
{
   UT_ASSERT_NOT_NULL(instance);
   ++ut_main_calls;
   return(ut_main_result);
}

static void reset_multiline(void)
{
   memset(&ut_default_options, 0, sizeof(ut_default_options));
   memset(ut_reallocated_buffer, 0, sizeof(ut_reallocated_buffer));
   memset(&od_control, 0, sizeof(od_control));
   bODInitialized = TRUE;
   ut_init_succeeds = TRUE;
   od_control.user_ansi = TRUE;
   ut_init_calls = 0;
   ut_entry_calls = 0;
   ut_exit_calls = 0;
   ut_setup_result = TRUE;
   ut_setup_calls = 0;
   ut_setup_buffer = NULL;
   ut_setup_size = 0;
   ut_setup_options = NULL;
   ut_cleanup_calls = 0;
   ut_format_result = TRUE;
   ut_format_calls = 0;
   ut_arrow_start_calls = 0;
   ut_arrow_end_calls = 0;
   ut_drain_calls = 0;
   ut_end_session_during_drain = FALSE;
   ut_redraw_calls = 0;
   ut_main_result = OD_MULTIEDIT_SUCCESS;
   ut_main_calls = 0;
}

static void terminal_session_is_rejected(void)
{
   char buffer[4];
   reset_multiline(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT_EQ_INT(OD_MULTIEDIT_ERROR,
      utt_od_multiline_edit(buffer, sizeof(buffer), NULL));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entry_calls);
}

static void initializes_before_entering_the_api(void)
{
   char buffer[8] = "text";
   reset_multiline();
   bODInitialized = FALSE;
   ut_setup_result = FALSE;
   UT_ASSERT_EQ_INT(OD_MULTIEDIT_ERROR,
      utt_od_multiline_edit(buffer, sizeof(buffer), NULL));
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entry_calls);
   UT_ASSERT_EQ_UINT(1, ut_exit_calls);
}

static void rejects_each_invalid_buffer_parameter(void)
{
   char buffer[1] = "";
   reset_multiline();
   UT_ASSERT_EQ_INT(OD_MULTIEDIT_ERROR,
      utt_od_multiline_edit(NULL, sizeof(buffer), NULL));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_setup_calls);
   UT_ASSERT_EQ_UINT(1, ut_exit_calls);

   reset_multiline();
   UT_ASSERT_EQ_INT(OD_MULTIEDIT_ERROR,
      utt_od_multiline_edit(buffer, 0, NULL));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_setup_calls);
}

static void requires_ansi_or_avatar_graphics(void)
{
   char buffer[8] = "text";
   reset_multiline();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = FALSE;
   UT_ASSERT_EQ_INT(OD_MULTIEDIT_ERROR,
      utt_od_multiline_edit(buffer, sizeof(buffer), NULL));
   UT_ASSERT_EQ_INT(ERR_NOGRAPHICS, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_setup_calls);

   reset_multiline();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = TRUE;
   ut_setup_result = FALSE;
   utt_od_multiline_edit(buffer, sizeof(buffer), NULL);
   UT_ASSERT_EQ_UINT(1, ut_setup_calls);

   reset_multiline();
   od_control.user_ansi = TRUE;
   od_control.user_avatar = FALSE;
   ut_setup_result = FALSE;
   utt_od_multiline_edit(buffer, sizeof(buffer), NULL);
   UT_ASSERT_EQ_UINT(1, ut_setup_calls);
}

static void cleans_up_a_setup_failure(void)
{
   char buffer[8] = "text";
   tODEditOptions options;
   reset_multiline();
   memset(&options, 0, sizeof(options));
   ut_setup_result = FALSE;
   UT_ASSERT_EQ_INT(OD_MULTIEDIT_ERROR,
      utt_od_multiline_edit(buffer, sizeof(buffer), &options));
   UT_ASSERT_EQ_PTR(buffer, ut_setup_buffer);
   UT_ASSERT_EQ_UINT(sizeof(buffer), ut_setup_size);
   UT_ASSERT_EQ_PTR(&options, ut_setup_options);
   UT_ASSERT_EQ_UINT(1, ut_cleanup_calls);
   UT_ASSERT_EQ_UINT(0, ut_format_calls);
}

static void cleans_up_a_formatting_failure(void)
{
   char buffer[8] = "text";
   reset_multiline();
   ut_format_result = FALSE;
   UT_ASSERT_EQ_INT(OD_MULTIEDIT_ERROR,
      utt_od_multiline_edit(buffer, sizeof(buffer), NULL));
   UT_ASSERT_EQ_UINT(1, ut_format_calls);
   UT_ASSERT_EQ_UINT(1, ut_cleanup_calls);
   UT_ASSERT_EQ_UINT(0, ut_arrow_start_calls);
}

static void returns_success_if_the_session_ends_while_draining(void)
{
   char buffer[8] = "text";
   reset_multiline();
   ut_end_session_during_drain = TRUE;
   UT_ASSERT_EQ_INT(OD_MULTIEDIT_SUCCESS,
      utt_od_multiline_edit(buffer, sizeof(buffer), NULL));
   UT_ASSERT_EQ_UINT(1, ut_arrow_start_calls);
   UT_ASSERT_EQ_UINT(1, ut_drain_calls);
   UT_ASSERT_EQ_UINT(1, ut_arrow_end_calls);
   UT_ASSERT_EQ_UINT(1, ut_cleanup_calls);
   UT_ASSERT_EQ_UINT(0, ut_redraw_calls);
   UT_ASSERT_EQ_UINT(0, ut_main_calls);
}

static void returns_the_loop_result_and_publishes_final_buffer(void)
{
   char buffer[8] = "text";
   tODEditOptions options;
   reset_multiline();
   memset(&options, 0, sizeof(options));
   ut_main_result = OD_MULTIEDIT_ERROR;
   UT_ASSERT_EQ_INT(OD_MULTIEDIT_ERROR,
      utt_od_multiline_edit(buffer, sizeof(buffer), &options));
   UT_ASSERT_EQ_UINT(1, ut_redraw_calls);
   UT_ASSERT_EQ_UINT(1, ut_main_calls);
   UT_ASSERT_EQ_UINT(1, ut_arrow_end_calls);
   UT_ASSERT_EQ_PTR(ut_reallocated_buffer, options.pszFinalBuffer);
   UT_ASSERT_EQ_UINT(sizeof(ut_reallocated_buffer), options.unFinalBufferSize);
   UT_ASSERT_EQ_UINT(1, ut_cleanup_calls);
   UT_ASSERT_EQ_UINT(1, ut_exit_calls);
}

static const UTTestCase ut_cases[] = {
   {"initialization", initializes_before_entering_the_api},
   {"buffer validation", rejects_each_invalid_buffer_parameter},
   {"graphics", requires_ansi_or_avatar_graphics},
   {"setup failure", cleans_up_a_setup_failure},
   {"format failure", cleans_up_a_formatting_failure},
   {"ended drain", returns_success_if_the_session_ends_while_draining},
   {"normal edit", returns_the_loop_result_and_publishes_final_buffer},
   {"terminal session", terminal_session_is_rejected}
};
