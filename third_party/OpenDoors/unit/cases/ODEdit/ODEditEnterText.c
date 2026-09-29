#define UT_CUSTOM_MOCK_ODEditRememberArea
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_ODEditBufferMakeSpace
#define UT_CUSTOM_MOCK_od_putch
#define UT_CUSTOM_MOCK_ODEditBufferGetCharacter
#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_ODEditBufferFormatAndIndex
#define UT_CUSTOM_MOCK_ODEditGetCurrentLineInArea
#define UT_CUSTOM_MOCK_ODEditScrollArea
#define UT_CUSTOM_MOCK_ODEditRedrawChanged

static tODResult ut_make_space_result;
static unsigned ut_make_space_calls;
static unsigned ut_beep_calls;
static BOOL ut_format_result;
static unsigned ut_format_calls;
static UINT ut_area_line;
static unsigned ut_area_calls;
static BOOL ut_scroll_result;
static INT ut_scroll_distance;
static unsigned ut_scroll_calls;
static UINT ut_redraw_upper[2];
static UINT ut_redraw_lower[2];
static unsigned ut_redraw_calls;

void utm_ODEditRememberArea(tEditInstance *instance, void *area)
{
   UT_ASSERT_EQ_PTR(instance->pRememberBuffer, area);
}

size_t utm_strlen(const char *text)
{
   size_t length;
   length = 0;
   while(text[length] != '\0')
      ++length;
   return(length);
}

tODResult utm_ODEditBufferMakeSpace(tEditInstance *instance, UINT line,
   UINT column, UINT count)
{
   ++ut_make_space_calls;
   UT_ASSERT_EQ_UINT(instance->unCurrentLine, line);
   UT_ASSERT_EQ_UINT(instance->unCurrentColumn, column);
   UT_ASSERT(count > 0);
   return(ut_make_space_result);
}

void ODCALL utm_od_putch(char character)
{
   ++ut_beep_calls;
   UT_ASSERT_EQ_INT('\a', character);
}

char *utm_ODEditBufferGetCharacter(tEditInstance *instance, UINT line,
   UINT column)
{
   UT_ASSERT_EQ_UINT(instance->unCurrentLine, line);
   UT_ASSERT_EQ_UINT(instance->unCurrentColumn, column);
   return(instance->pszEditBuffer + column);
}

void *utm_memcpy(void *destination, const void *source, size_t count)
{
   char *dest;
   const char *src;
   size_t index;
   dest = (char *)destination;
   src = (const char *)source;
   for(index = 0; index < count; ++index)
      dest[index] = src[index];
   return(destination);
}

BOOL utm_ODEditBufferFormatAndIndex(tEditInstance *instance)
{
   ++ut_format_calls;
   UT_ASSERT_NOT_NULL(instance);
   return(ut_format_result);
}

UINT utm_ODEditGetCurrentLineInArea(tEditInstance *instance)
{
   ++ut_area_calls;
   UT_ASSERT_NOT_NULL(instance);
   return(ut_area_line);
}

BOOL utm_ODEditScrollArea(tEditInstance *instance, INT distance)
{
   ++ut_scroll_calls;
   UT_ASSERT_NOT_NULL(instance);
   ut_scroll_distance = distance;
   return(ut_scroll_result);
}

void utm_ODEditRedrawChanged(tEditInstance *instance, void *area,
   UINT upper, UINT lower)
{
   UT_ASSERT_NOT_NULL(instance);
   UT_ASSERT_EQ_PTR(instance->pRememberBuffer, area);
   UT_ASSERT(ut_redraw_calls < 2);
   ut_redraw_upper[ut_redraw_calls] = upper;
   ut_redraw_lower[ut_redraw_calls] = lower;
   ++ut_redraw_calls;
}

static void reset_entry(tEditInstance *instance, char *buffer,
   void *remembered)
{
   instance->pszEditBuffer = buffer;
   instance->unCurrentLine = 0;
   instance->unCurrentColumn = 0;
   instance->unAreaHeight = 5;
   instance->unScrollDistance = 1;
   instance->pRememberBuffer = remembered;
   ut_make_space_result = kODRCSuccess;
   ut_make_space_calls = 0;
   ut_beep_calls = 0;
   ut_format_result = TRUE;
   ut_format_calls = 0;
   ut_area_line = 0;
   ut_area_calls = 0;
   ut_scroll_result = FALSE;
   ut_scroll_distance = 0;
   ut_scroll_calls = 0;
   ut_redraw_calls = 0;
}

static void returns_an_insert_failure_after_beeping(void)
{
   char buffer[8] = "abc";
   char remembered;
   tEditInstance instance;
   reset_entry(&instance, buffer, &remembered);
   ut_make_space_result = kODRCSafeFailure;
   UT_ASSERT_EQ_INT(kODRCSafeFailure,
      utt_ODEditEnterText(&instance, "x", TRUE));
   UT_ASSERT_EQ_UINT(1, ut_make_space_calls);
   UT_ASSERT_EQ_UINT(1, ut_beep_calls);
   UT_ASSERT_EQ_UINT(0, ut_format_calls);
   UT_ASSERT_EQ_UINT(0, ut_redraw_calls);
}

static void accepts_an_empty_overwrite(void)
{
   char buffer[8] = "abc";
   char remembered;
   tEditInstance instance;
   reset_entry(&instance, buffer, &remembered);
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODEditEnterText(&instance, "", FALSE));
   UT_ASSERT_EQ_UINT(0, ut_make_space_calls);
   UT_ASSERT_EQ_UINT(1, ut_format_calls);
   UT_ASSERT_EQ_UINT(1, ut_redraw_calls);
   UT_ASSERT_EQ_UINT(REDRAW_NO_BOUNDARY, ut_redraw_upper[0]);
   UT_ASSERT_EQ_UINT(REDRAW_NO_BOUNDARY, ut_redraw_lower[0]);
}

static void advances_over_characters_and_each_eol_pair_case(void)
{
   char buffer[32] = "................";
   char remembered;
   tEditInstance instance;

   reset_entry(&instance, buffer, &remembered);
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODEditEnterText(&instance, "a\r\nb", TRUE));
   UT_ASSERT_EQ_UINT(1, instance.unCurrentLine);
   UT_ASSERT_EQ_UINT(1, instance.unCurrentColumn);
   UT_ASSERT(memcmp(buffer, "a\r\nb", 4) == 0);

   reset_entry(&instance, buffer, &remembered);
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODEditEnterText(&instance, "\rx", FALSE));
   UT_ASSERT_EQ_UINT(1, instance.unCurrentLine);
   UT_ASSERT_EQ_UINT(1, instance.unCurrentColumn);

   reset_entry(&instance, buffer, &remembered);
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODEditEnterText(&instance, "\r", FALSE));
   UT_ASSERT_EQ_UINT(1, instance.unCurrentLine);
   UT_ASSERT_EQ_UINT(0, instance.unCurrentColumn);

   reset_entry(&instance, buffer, &remembered);
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODEditEnterText(&instance, "\n", FALSE));
   UT_ASSERT_EQ_UINT(1, instance.unCurrentLine);
   UT_ASSERT_EQ_UINT(0, instance.unCurrentColumn);

   reset_entry(&instance, buffer, &remembered);
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODEditEnterText(&instance, "\n\r", FALSE));
   UT_ASSERT_EQ_UINT(1, instance.unCurrentLine);
   UT_ASSERT_EQ_UINT(0, instance.unCurrentColumn);

   reset_entry(&instance, buffer, &remembered);
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODEditEnterText(&instance, "\r\r", FALSE));
   UT_ASSERT_EQ_UINT(2, instance.unCurrentLine);
   UT_ASSERT_EQ_UINT(0, instance.unCurrentColumn);
}

static void returns_an_unrecoverable_reindex_failure(void)
{
   char buffer[8] = "abc";
   char remembered;
   tEditInstance instance;
   reset_entry(&instance, buffer, &remembered);
   ut_format_result = FALSE;
   UT_ASSERT_EQ_INT(kODRCUnrecoverableFailure,
      utt_ODEditEnterText(&instance, "x", FALSE));
   UT_ASSERT_EQ_UINT(1, ut_format_calls);
   UT_ASSERT_EQ_UINT(0, ut_area_calls);
   UT_ASSERT_EQ_UINT(0, ut_redraw_calls);
}

static void uses_the_configured_scroll_distance_without_partial_redraw(void)
{
   char buffer[16] = "abc";
   char remembered;
   tEditInstance instance;
   reset_entry(&instance, buffer, &remembered);
   instance.unScrollDistance = 3;
   ut_area_line = 5;
   ut_scroll_result = FALSE;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODEditEnterText(&instance, "x", FALSE));
   UT_ASSERT_EQ_UINT(1, ut_scroll_calls);
   UT_ASSERT_EQ_INT(3, ut_scroll_distance);
   UT_ASSERT_EQ_UINT(1, ut_redraw_calls);
}

static void redraws_old_and_final_areas_after_an_efficient_scroll(void)
{
   char buffer[16] = "abc";
   char remembered;
   tEditInstance instance;
   reset_entry(&instance, buffer, &remembered);
   instance.unAreaHeight = 5;
   instance.unScrollDistance = 1;
   ut_area_line = 6;
   ut_scroll_result = TRUE;
   UT_ASSERT_EQ_INT(kODRCSuccess,
      utt_ODEditEnterText(&instance, "x", FALSE));
   UT_ASSERT_EQ_UINT(1, ut_scroll_calls);
   UT_ASSERT_EQ_INT(2, ut_scroll_distance);
   UT_ASSERT_EQ_UINT(2, ut_redraw_calls);
   UT_ASSERT_EQ_UINT(0, ut_redraw_upper[0]);
   UT_ASSERT_EQ_UINT(3, ut_redraw_lower[0]);
   UT_ASSERT_EQ_UINT(REDRAW_NO_BOUNDARY, ut_redraw_upper[1]);
   UT_ASSERT_EQ_UINT(REDRAW_NO_BOUNDARY, ut_redraw_lower[1]);
}

static const UTTestCase ut_cases[] = {
   {"insert failure", returns_an_insert_failure_after_beeping},
   {"empty overwrite", accepts_an_empty_overwrite},
   {"cursor advance", advances_over_characters_and_each_eol_pair_case},
   {"reindex failure", returns_an_unrecoverable_reindex_failure},
   {"full scroll redraw", uses_the_configured_scroll_distance_without_partial_redraw},
   {"efficient scroll", redraws_old_and_final_areas_after_an_efficient_scroll}
};
