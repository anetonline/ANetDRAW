#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_ODScrnEnableScrolling
#define UT_CUSTOM_MOCK_ODScrnGetTextInfo
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos
#define UT_CUSTOM_MOCK_ODSessionScreenAvailable
#define UT_CUSTOM_MOCK_ODSessionScreenClearToEndOfLine
#define UT_CUSTOM_MOCK_ODSessionScreenGetInfo
#define UT_CUSTOM_MOCK_ODSessionScreenPresent
#define UT_CUSTOM_MOCK_ODSessionScreenSetCursorPos
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_disp
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_sprintf
#define UT_CUSTOM_MOCK_strlen

#include <stdarg.h>

static BOOL ut_session_available;
static tODVScreenInfo ut_session_info;
static tODScrnTextInfo ut_text_info;
static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_screen_queries;
static unsigned ut_session_info_calls;
static unsigned ut_text_info_calls;
static unsigned ut_clear_calls;
static unsigned ut_present_calls;
static unsigned ut_session_cursor_calls;
static unsigned ut_local_cursor_calls;
static INT ut_cursor_column;
static INT ut_cursor_row;
static unsigned ut_display_calls;
static INT ut_display_length;
static BOOL ut_display_local;
static char ut_display_text[80];
static BOOL ut_scrolling_values[4];
static unsigned ut_scrolling_calls;
static BOOL ut_init_succeeds;

static void reset_graph_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
#ifdef UT_GRAPH_CLEAR_LINE
   memset(szODWorkString, 0, sizeof(szODWorkString));
   memset(&ODTextInfo, 0, sizeof(ODTextInfo));
#endif
   memset(&ut_session_info, 0, sizeof(ut_session_info));
   memset(&ut_text_info, 0, sizeof(ut_text_info));
   memset(ut_display_text, 0, sizeof(ut_display_text));
   memset(ut_scrolling_values, 0, sizeof(ut_scrolling_values));
   bODInitialized = TRUE;
   ut_session_available = FALSE;
   ut_session_info.winleft = 1;
   ut_session_info.wintop = 1;
   ut_session_info.winright = 80;
   ut_session_info.winbottom = 25;
   ut_session_info.curx = 1;
   ut_session_info.cury = 1;
   ut_text_info.winleft = 1;
   ut_text_info.wintop = 1;
   ut_text_info.winright = 80;
   ut_text_info.winbottom = 25;
   ut_text_info.curx = 1;
   ut_text_info.cury = 1;
   ut_init_calls = ut_entries = ut_exits = 0;
   ut_screen_queries = ut_session_info_calls = ut_text_info_calls = 0;
   ut_clear_calls = ut_present_calls = 0;
   ut_session_cursor_calls = ut_local_cursor_calls = 0;
   ut_cursor_column = ut_cursor_row = 0;
   ut_display_calls = 0;
   ut_display_length = 0;
   ut_display_local = FALSE;
   ut_scrolling_calls = 0;
   ut_init_succeeds = TRUE;
}

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

BOOL utm_ODSessionScreenAvailable(void)
{
   ++ut_screen_queries;
   return ut_session_available;
}

void utm_ODSessionScreenGetInfo(tODVScreenInfo *info)
{
   *info = ut_session_info;
   ++ut_session_info_calls;
}

void utm_ODScrnGetTextInfo(tODScrnTextInfo *info)
{
   *info = ut_text_info;
#ifdef UT_GRAPH_CLEAR_LINE
   ODTextInfo = ut_text_info;
#endif
   ++ut_text_info_calls;
}

void utm_ODSessionScreenClearToEndOfLine(void) { ++ut_clear_calls; }
void utm_ODSessionScreenPresent(void) { ++ut_present_calls; }

void utm_ODSessionScreenSetCursorPos(INT column, INT row)
{
   ++ut_session_cursor_calls;
   ut_cursor_column = column;
   ut_cursor_row = row;
}

void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{
   ++ut_local_cursor_calls;
   ut_cursor_column = column;
   ut_cursor_row = row;
}

void ODCALL utm_ODScrnDisplayString(const char *text)
{
   size_t index = 0;
   while(text[index] != '\0' && index + 1 < sizeof(ut_display_text))
   {
      ut_display_text[index] = text[index];
      ++index;
   }
   ut_display_text[index] = '\0';
   ++ut_display_calls;
}

void utm_ODScrnEnableScrolling(BOOL enabled)
{
   UT_ASSERT(ut_scrolling_calls < 4);
   ut_scrolling_values[ut_scrolling_calls++] = enabled;
}

void ODCALL utm_od_disp(const char *text, INT length, BOOL local)
{
   INT index;
   UT_ASSERT(length >= 0);
   ut_display_length = length;
   ut_display_local = local;
   for(index = 0; index < length && index + 1 < (INT)sizeof(ut_display_text);
      ++index)
      ut_display_text[index] = text[index];
   ut_display_text[index] = '\0';
   ++ut_display_calls;
}

size_t utm_strlen(const char *text)
{
   const char *end = text;
   while(*end != '\0') ++end;
   return (size_t)(end - text);
}

int utm_sprintf(char *buffer, const char *format, ...)
{
   int result;
   va_list args;
   va_start(args, format);
   result = vsprintf(buffer, format, args);
   va_end(args);
   return result;
}
