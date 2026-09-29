#define UT_CUSTOM_MOCK_ODSessionScreenAvailable
#define UT_CUSTOM_MOCK_ODSessionScreenGetInfo
#define UT_CUSTOM_MOCK_ODScrnGetTextInfo
#define UT_CUSTOM_MOCK_ODEditRememberBufferSize
#define UT_CUSTOM_MOCK_malloc

static BOOL ut_session_available;
static INT ut_session_width;
static INT ut_session_height;
static unsigned ut_session_info_calls;
static unsigned ut_local_info_calls;
static size_t ut_remember_size;
static unsigned ut_remember_size_calls;
static void *ut_malloc_result;
static size_t ut_malloc_size;
static unsigned ut_malloc_calls;
static char ut_remember_storage;

BOOL utm_ODSessionScreenAvailable(void)
{
   return(ut_session_available);
}

void utm_ODSessionScreenGetInfo(tODVScreenInfo *info)
{
   ++ut_session_info_calls;
   info->winleft = 4;
   info->wintop = 3;
   info->winright = ut_session_width + 3;
   info->winbottom = ut_session_height + 2;
}

void utm_ODScrnGetTextInfo(tODScrnTextInfo *info)
{
   ++ut_local_info_calls;
   info->winleft = 2;
   info->wintop = 2;
   info->winright = 81;
   info->winbottom = 26;
}

size_t utm_ODEditRememberBufferSize(tEditInstance *instance)
{
   ++ut_remember_size_calls;
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT(instance->unAreaWidth > 0);
   UT_ASSERT(instance->unAreaHeight > 0);
   return(ut_remember_size);
}

void *utm_malloc(size_t size)
{
   ++ut_malloc_calls;
   ut_malloc_size = size;
   return(ut_malloc_result);
}

static void reset_setup(void)
{
   ut_session_available = TRUE;
   ut_session_width = 300;
   ut_session_height = 300;
   ut_session_info_calls = 0;
   ut_local_info_calls = 0;
   ut_remember_size = 123;
   ut_remember_size_calls = 0;
   ut_malloc_result = &ut_remember_storage;
   ut_malloc_size = 0;
   ut_malloc_calls = 0;
   od_control.od_error = 0;
   od_control.user_avatar = FALSE;
   od_control.baud = 38400;
}

static void valid_options(tODEditOptions *options)
{
   options->nAreaLeft = 1;
   options->nAreaTop = 1;
   options->nAreaRight = 10;
   options->nAreaBottom = 5;
   options->TextFormat = FORMAT_PARAGRAPH_BREAKS;
   options->pfMenuCallback = NULL;
   options->pfBufferRealloc = NULL;
   options->dwEditFlags = EFLAG_NORMAL;
   options->pszFinalBuffer = NULL;
   options->unFinalBufferSize = 0;
}

static BOOL setup_with(tEditInstance *instance, tODEditOptions *options)
{
   static char buffer[32];
   memset(instance, 0x5a, sizeof(*instance));
   return(utt_ODEditSetupInstance(instance, buffer, sizeof(buffer), options));
}

static void initializes_default_options_and_instance_state(void)
{
   tEditInstance instance;
   reset_setup();
   UT_ASSERT_EQ_INT(TRUE, setup_with(&instance, NULL));
   UT_ASSERT_EQ_PTR(&ODEditOptionsDefault, instance.pUserOptions);
   UT_ASSERT_EQ_UINT(80, instance.unAreaWidth);
   UT_ASSERT_EQ_UINT(23, instance.unAreaHeight);
   UT_ASSERT_EQ_UINT(0, instance.unCurrentLine);
   UT_ASSERT_EQ_UINT(0, instance.unCurrentColumn);
   UT_ASSERT_EQ_UINT(0, instance.unLineScrolledToTop);
   UT_ASSERT_EQ_UINT(0, instance.unLineArraySize);
   UT_ASSERT_EQ_UINT(0, instance.unLinesInBuffer);
   UT_ASSERT_EQ_INT(TRUE, instance.bInsertMode);
   UT_ASSERT_EQ_UINT(DEFAULT_TAB_STOP_SIZE, instance.unTabStopSize);
   UT_ASSERT_NULL(instance.papchStartOfLine);
   UT_ASSERT_EQ_PTR(&ut_remember_storage, instance.pRememberBuffer);
   UT_ASSERT_EQ_UINT(1, ut_session_info_calls);
   UT_ASSERT_EQ_UINT(0, ut_local_info_calls);
   UT_ASSERT_EQ_UINT(1, ut_remember_size_calls);
   UT_ASSERT_EQ_UINT(1, ut_malloc_calls);
   UT_ASSERT(ut_malloc_size == ut_remember_size);
}

static void fills_unspecified_coordinates_and_uses_local_dimensions(void)
{
   tEditInstance instance;
   tODEditOptions options;
   reset_setup();
   valid_options(&options);
   options.nAreaLeft = 0;
   options.nAreaTop = 0;
   options.nAreaRight = 0;
   options.nAreaBottom = 0;
   ut_session_available = FALSE;
   UT_ASSERT_EQ_INT(TRUE, setup_with(&instance, &options));
   UT_ASSERT_EQ_INT(ODEditOptionsDefault.nAreaLeft, options.nAreaLeft);
   UT_ASSERT_EQ_INT(ODEditOptionsDefault.nAreaTop, options.nAreaTop);
   UT_ASSERT_EQ_INT(ODEditOptionsDefault.nAreaRight, options.nAreaRight);
   UT_ASSERT_EQ_INT(ODEditOptionsDefault.nAreaBottom, options.nAreaBottom);
   UT_ASSERT_EQ_UINT(0, ut_session_info_calls);
   UT_ASSERT_EQ_UINT(1, ut_local_info_calls);
}

static void rejects_each_invalid_coordinate_independently(void)
{
   tEditInstance instance;
   tODEditOptions options;

   reset_setup(); valid_options(&options); options.nAreaLeft = -1;
   UT_ASSERT_EQ_INT(FALSE, setup_with(&instance, &options));
   reset_setup(); valid_options(&options); options.nAreaTop = -1;
   UT_ASSERT_EQ_INT(FALSE, setup_with(&instance, &options));
   reset_setup(); valid_options(&options); options.nAreaRight = 1;
   UT_ASSERT_EQ_INT(FALSE, setup_with(&instance, &options));
   reset_setup(); valid_options(&options); options.nAreaBottom = 1;
   UT_ASSERT_EQ_INT(FALSE, setup_with(&instance, &options));
   reset_setup(); valid_options(&options); options.nAreaRight = 301;
   UT_ASSERT_EQ_INT(FALSE, setup_with(&instance, &options));
   reset_setup(); valid_options(&options); options.nAreaBottom = 301;
   UT_ASSERT_EQ_INT(FALSE, setup_with(&instance, &options));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_malloc_calls);
   UT_ASSERT_NULL(instance.papchStartOfLine);
   UT_ASSERT_NULL(instance.pRememberBuffer);
}

static void applies_avatar_coordinate_limits_only_in_avatar_mode(void)
{
   tEditInstance instance;
   tODEditOptions options;

   reset_setup(); valid_options(&options); options.nAreaRight = 256;
   UT_ASSERT_EQ_INT(TRUE, setup_with(&instance, &options));

   reset_setup(); valid_options(&options); options.nAreaRight = 256;
   od_control.user_avatar = TRUE;
   UT_ASSERT_EQ_INT(FALSE, setup_with(&instance, &options));

   reset_setup(); valid_options(&options); options.nAreaRight = 255;
   options.nAreaBottom = 256; od_control.user_avatar = TRUE;
   UT_ASSERT_EQ_INT(FALSE, setup_with(&instance, &options));

   reset_setup(); valid_options(&options); options.nAreaRight = 255;
   options.nAreaBottom = 255; od_control.user_avatar = TRUE;
   UT_ASSERT_EQ_INT(TRUE, setup_with(&instance, &options));
}

static void configures_every_supported_text_format(void)
{
   tEditInstance instance;
   tODEditOptions options;

   reset_setup(); valid_options(&options); options.TextFormat = FORMAT_FTSC_MESSAGE;
   UT_ASSERT_EQ_INT(TRUE, setup_with(&instance, &options));
   UT_ASSERT_EQ_INT(0, strcmp("", instance.pszLineBreak));
   UT_ASSERT_EQ_INT(0, strcmp("\r", instance.pszParagraphBreak));
   UT_ASSERT_EQ_INT(TRUE, instance.bWordWrapLongLines);

   reset_setup(); valid_options(&options);
   options.TextFormat = FORMAT_PARAGRAPH_BREAKS;
   UT_ASSERT_EQ_INT(TRUE, setup_with(&instance, &options));
   UT_ASSERT_EQ_INT(0, strcmp("", instance.pszLineBreak));
   UT_ASSERT_NULL(instance.pszParagraphBreak);
   UT_ASSERT_EQ_INT(TRUE, instance.bWordWrapLongLines);

   reset_setup(); valid_options(&options); options.TextFormat = FORMAT_LINE_BREAKS;
   UT_ASSERT_EQ_INT(TRUE, setup_with(&instance, &options));
   UT_ASSERT_NULL(instance.pszLineBreak);
   UT_ASSERT_EQ_INT(0, strcmp("", instance.pszParagraphBreak));
   UT_ASSERT_EQ_INT(TRUE, instance.bWordWrapLongLines);

   reset_setup(); valid_options(&options); options.TextFormat = FORMAT_NO_WORDWRAP;
   UT_ASSERT_EQ_INT(TRUE, setup_with(&instance, &options));
   UT_ASSERT_NULL(instance.pszLineBreak);
   UT_ASSERT_EQ_INT(0, strcmp("", instance.pszParagraphBreak));
   UT_ASSERT_EQ_INT(FALSE, instance.bWordWrapLongLines);
}

static void rejects_an_invalid_text_format(void)
{
   tEditInstance instance;
   tODEditOptions options;
   reset_setup();
   valid_options(&options);
   options.TextFormat = (tODEditTextFormat)99;
   UT_ASSERT_EQ_INT(FALSE, setup_with(&instance, &options));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_malloc_calls);
}

static void reports_remember_buffer_allocation_failure(void)
{
   tEditInstance instance;
   tODEditOptions options;
   reset_setup();
   valid_options(&options);
   ut_malloc_result = NULL;
   UT_ASSERT_EQ_INT(FALSE, setup_with(&instance, &options));
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_remember_size_calls);
   UT_ASSERT_EQ_UINT(1, ut_malloc_calls);
}

static void selects_each_scroll_distance_case(void)
{
   tEditInstance instance;
   tODEditOptions options;

   reset_setup(); valid_options(&options); od_control.user_avatar = TRUE;
   UT_ASSERT_EQ_INT(TRUE, setup_with(&instance, &options));
   UT_ASSERT_EQ_UINT(1, instance.unScrollDistance);

   reset_setup(); valid_options(&options); od_control.baud = 0;
   UT_ASSERT_EQ_INT(TRUE, setup_with(&instance, &options));
   UT_ASSERT_EQ_UINT(1, instance.unScrollDistance);

   reset_setup(); valid_options(&options); options.nAreaBottom = 5;
   UT_ASSERT_EQ_INT(TRUE, setup_with(&instance, &options));
   UT_ASSERT_EQ_UINT(4, instance.unScrollDistance);

   reset_setup(); valid_options(&options); options.nAreaBottom = 20;
   UT_ASSERT_EQ_INT(TRUE, setup_with(&instance, &options));
   UT_ASSERT_EQ_UINT(ANSI_SCROLL_DISTANCE, instance.unScrollDistance);
}

static const UTTestCase ut_cases[] = {
   {"default setup", initializes_default_options_and_instance_state},
   {"coordinate defaults", fills_unspecified_coordinates_and_uses_local_dimensions},
   {"invalid coordinates", rejects_each_invalid_coordinate_independently},
   {"avatar limits", applies_avatar_coordinate_limits_only_in_avatar_mode},
   {"text formats", configures_every_supported_text_format},
   {"invalid text format", rejects_an_invalid_text_format},
   {"allocation failure", reports_remember_buffer_allocation_failure},
   {"scroll distance", selects_each_scroll_distance_case}
};
