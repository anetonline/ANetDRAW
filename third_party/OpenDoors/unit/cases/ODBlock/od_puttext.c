#define UT_CUSTOM_MOCK_ODScrnGetText
#define UT_CUSTOM_MOCK_ODScrnGetTextInfo
#define UT_CUSTOM_MOCK_ODScrnPutText
#define UT_CUSTOM_MOCK_ODSessionScreenAvailable
#define UT_CUSTOM_MOCK_ODSessionScreenGetInfo
#define UT_CUSTOM_MOCK_ODSessionScreenGetText
#define UT_CUSTOM_MOCK_ODSessionScreenPresent
#define UT_CUSTOM_MOCK_ODSessionScreenPutText
#define UT_CUSTOM_MOCK_ODSizeMultiply
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_od_disp
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_set_attrib
#define UT_CUSTOM_MOCK_od_set_cursor
#define UT_CUSTOM_MOCK_sprintf
#define UT_CUSTOM_MOCK_strlen

#include <stdarg.h>

#define MOCK_ATTRIB 320
#define MOCK_CURSOR 321
#define MOCK_DISP 322
#define MOCK_FREE 323
#define MOCK_GET 324
#define MOCK_INIT 325
#define MOCK_MALLOC 326
#define MOCK_PRESENT 327
#define MOCK_PUT 328

static unsigned char ut_old_block[2048];
static unsigned char ut_new_block[2048];
static unsigned char ut_allocation[2048];
static BOOL ut_session_available;
static INT ut_width;
static INT ut_height;
static BOOL ut_get_result;
static BOOL ut_put_result;
static BOOL ut_size_result;
static BOOL ut_malloc_fails;
static size_t ut_expected_bytes;
static INT ut_disp_count;
static BOOL ut_init_succeeds;

static void reset_fixture(void)
{
   ut_mock_call_count = 0;
   memset(&od_control, 0, sizeof(od_control));
   memset(&ODTextInfo, 0, sizeof(ODTextInfo));
   memset(ut_old_block, 0, sizeof(ut_old_block));
   memset(ut_new_block, 0, sizeof(ut_new_block));
   memset(ut_allocation, 0, sizeof(ut_allocation));
   memset(szODWorkString, 0, sizeof(szODWorkString));
   bODInitialized = TRUE;
   bScrollAction = TRUE;
   od_control.user_ansi = TRUE;
   ut_session_available = TRUE;
   ut_width = 300;
   ut_height = 300;
   ut_get_result = TRUE;
   ut_put_result = TRUE;
   ut_size_result = TRUE;
   ut_malloc_fails = FALSE;
   ut_expected_bytes = 0;
   ut_disp_count = 0;
   ut_init_succeeds = TRUE;
}

static void set_cell(unsigned char *block, INT column, char value,
   unsigned char attribute)
{
   block[column * 2] = (unsigned char)value;
   block[column * 2 + 1] = attribute;
}

BOOL ODCALL utm_ODScrnGetText(BYTE left, BYTE top, BYTE right, BYTE bottom,
   void *buffer)
{
   size_t bytes = (size_t)(right - left + 1) *
      (size_t)(bottom - top + 1) * 2U;
   memcpy(buffer, ut_old_block, bytes);
   ut_mock_called(MOCK_GET);
   return ut_get_result;
}

void utm_ODScrnGetTextInfo(tODScrnTextInfo *info)
{
   memset(info, 0, sizeof(*info));
   info->winleft = 1;
   info->winright = ut_width;
   info->wintop = 1;
   info->winbottom = ut_height;
   info->curx = 9;
   info->cury = 8;
   info->attribute = 7;
}

BOOL ODCALL utm_ODScrnPutText(BYTE left, BYTE top, BYTE right, BYTE bottom,
   void *buffer)
{
   (void)left; (void)top; (void)right; (void)bottom;
   UT_ASSERT_EQ_PTR(ut_new_block, buffer);
   ut_mock_called(MOCK_PUT);
   return ut_put_result;
}

BOOL utm_ODSessionScreenAvailable(void)
{
   return ut_session_available;
}

void utm_ODSessionScreenGetInfo(tODVScreenInfo *info)
{
   memset(info, 0, sizeof(*info));
   info->winleft = 1;
   info->winright = ut_width;
   info->wintop = 1;
   info->winbottom = ut_height;
   info->curx = 9;
   info->cury = 8;
   info->attribute = 7;
}

BOOL utm_ODSessionScreenGetText(INT left, INT top, INT right, INT bottom,
   void *buffer)
{
   size_t bytes = (size_t)(right - left + 1) *
      (size_t)(bottom - top + 1) * 2U;
   memcpy(buffer, ut_old_block, bytes);
   ut_mock_called(MOCK_GET);
   return ut_get_result;
}

void utm_ODSessionScreenPresent(void)
{
   ut_mock_called(MOCK_PRESENT);
}

BOOL utm_ODSessionScreenPutText(INT left, INT top, INT right, INT bottom,
   const void *buffer)
{
   (void)left; (void)top; (void)right; (void)bottom;
   UT_ASSERT_EQ_PTR(ut_new_block, buffer);
   ut_mock_called(MOCK_PUT);
   return ut_put_result;
}

int utm_ODSizeMultiply(size_t left, size_t right, size_t *result)
{
   UT_ASSERT_NOT_NULL(result);
   if(!ut_size_result)
      return FALSE;
   *result = left * right;
   ut_expected_bytes = *result;
   return TRUE;
}

void utm_ODSyncAPIEntry(void) { }
void utm_ODSyncAPIExit(void) { }

void utm_free(void *memory)
{
   UT_ASSERT_EQ_PTR(ut_allocation, memory);
   ut_mock_called(MOCK_FREE);
}

void *utm_malloc(size_t size)
{
   ut_expected_bytes = size;
   ut_mock_called(MOCK_MALLOC);
   return ut_malloc_fails ? NULL : ut_allocation;
}

void ODCALL utm_od_disp(const char *buffer, INT size, BOOL local_echo)
{
   UT_ASSERT_NOT_NULL(buffer);
   UT_ASSERT(size > 0);
   UT_ASSERT_EQ_INT(FALSE, local_echo);
   ++ut_disp_count;
   ut_mock_called(MOCK_DISP);
}

void ODCALL utm_od_init(void)
{
   if(ut_init_succeeds) bODInitialized = TRUE;
   ut_mock_called(MOCK_INIT);
}

static void terminal_session_is_rejected(void)
{
   reset_fixture(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_puttext(1, 1, 1, 1, ut_new_block));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_PUT));
}

void ODCALL utm_od_set_attrib(INT colour)
{
   (void)colour;
   ut_mock_called(MOCK_ATTRIB);
}

void ODCALL utm_od_set_cursor(INT row, INT column)
{
   UT_ASSERT_EQ_INT(8, row);
   UT_ASSERT_EQ_INT(9, column);
   ut_mock_called(MOCK_CURSOR);
}

int utm_sprintf(char *buffer, const char *format, ...)
{
   va_list arguments;
   int result;
   va_start(arguments, format);
   result = vsprintf(buffer, format, arguments);
   va_end(arguments);
   return result;
}

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

static void assert_invalid(INT left, INT top, INT right, INT bottom,
   void *buffer)
{
   reset_fixture();
   UT_ASSERT_EQ_INT(FALSE,
      utt_od_puttext(left, top, right, bottom, buffer));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_PUT));
}

static void each_parameter_error_is_rejected(void)
{
   assert_invalid(0, 1, 1, 1, ut_new_block);
   assert_invalid(1, 0, 1, 1, ut_new_block);
   assert_invalid(1, 1, 301, 1, ut_new_block);
   assert_invalid(1, 1, 1, 301, ut_new_block);
   assert_invalid(1, 2, 1, 1, ut_new_block);
   assert_invalid(2, 1, 1, 1, ut_new_block);
   assert_invalid(1, 1, 1, 1, NULL);
}

static void avatar_limits_each_dimension(void)
{
   reset_fixture();
   od_control.user_avatar = TRUE;
   od_control.user_ansi = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_puttext(1, 1, 256, 1, ut_new_block));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);

   reset_fixture();
   od_control.user_avatar = TRUE;
   od_control.user_ansi = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_puttext(1, 1, 1, 256, ut_new_block));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);

   reset_fixture();
   od_control.user_avatar = FALSE;
   UT_ASSERT_EQ_INT(TRUE, utt_od_puttext(1, 1, 256, 1, ut_new_block));
}

static void size_and_graphics_failures_are_reported(void)
{
   reset_fixture();
   ut_size_result = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_puttext(1, 1, 2, 2, ut_new_block));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);

   reset_fixture();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_puttext(1, 1, 1, 1, ut_new_block));
   UT_ASSERT_EQ_INT(ERR_NOGRAPHICS, od_control.od_error);
}

static void local_put_uses_each_screen_backend(void)
{
   reset_fixture();
   bODInitialized = FALSE;
   od_control.baud = 0;
   UT_ASSERT_EQ_INT(TRUE, utt_od_puttext(1, 1, 1, 1, ut_new_block));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_PRESENT));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_INIT));

   reset_fixture();
   ut_session_available = FALSE;
   od_control.baud = 0;
   UT_ASSERT_EQ_INT(TRUE, utt_od_puttext(1, 1, 1, 1, ut_new_block));
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_PRESENT));

   reset_fixture();
   ut_put_result = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_puttext(1, 1, 1, 1, ut_new_block));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
}

static void remote_allocation_and_screen_failures_are_reported(void)
{
   reset_fixture();
   od_control.baud = 38400;
   ut_malloc_fails = TRUE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_puttext(1, 1, 1, 1, ut_new_block));
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);

   reset_fixture();
   od_control.baud = 38400;
   ut_get_result = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_puttext(1, 1, 1, 1, ut_new_block));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_FREE));

   reset_fixture();
   od_control.baud = 38400;
   ut_session_available = FALSE;
   ut_get_result = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_puttext(1, 1, 1, 1, ut_new_block));

   reset_fixture();
   od_control.baud = 38400;
   ut_put_result = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_puttext(1, 1, 1, 1, ut_new_block));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_FREE));

   reset_fixture();
   od_control.baud = 38400;
   ut_session_available = FALSE;
   ut_put_result = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_od_puttext(1, 1, 1, 1, ut_new_block));
}

static void unchanged_remote_block_emits_no_text(void)
{
   reset_fixture();
   od_control.baud = 38400;
   set_cell(ut_old_block, 0, ' ', 7);
   set_cell(ut_new_block, 0, 0, 7);
   set_cell(ut_old_block, 1, 0, 7);
   set_cell(ut_new_block, 1, ' ', 7);
   set_cell(ut_old_block, 2, 'Q', 7);
   set_cell(ut_new_block, 2, 'Q', 7);

   UT_ASSERT_EQ_INT(TRUE, utt_od_puttext(1, 1, 3, 1, ut_new_block));
   UT_ASSERT_EQ_INT(0, ut_disp_count);
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_CURSOR));
   UT_ASSERT_EQ_UINT(1, ut_mock_count(MOCK_FREE));
}

static void ansi_remote_block_skips_long_unchanged_run(void)
{
   INT index;
   reset_fixture();
   od_control.baud = 38400;
   for(index = 0; index < 13; ++index)
   {
      set_cell(ut_old_block, index, ' ', 7);
      set_cell(ut_new_block, index, ' ', 7);
   }
   set_cell(ut_new_block, 0, 'A', 2);
   set_cell(ut_new_block, 1, 'B', 2);
   set_cell(ut_old_block, 10, 'X', 7);
   set_cell(ut_new_block, 10, ' ', 7);
   set_cell(ut_old_block, 11, ' ', 7);
   set_cell(ut_new_block, 11, ' ', 0x71);
   set_cell(ut_new_block, 12, 'C', 2);

   UT_ASSERT_EQ_INT(TRUE, utt_od_puttext(1, 1, 13, 1, ut_new_block));
   UT_ASSERT(ut_disp_count >= 5);
   UT_ASSERT_EQ_UINT(5, ut_mock_count(MOCK_ATTRIB));
}

static void avatar_changed_run_repositions_after_shorter_gap(void)
{
   INT index;
   reset_fixture();
   od_control.baud = 38400;
   od_control.user_ansi = FALSE;
   od_control.user_avatar = TRUE;
   for(index = 0; index < 7; ++index)
   {
      set_cell(ut_old_block, index, ' ', 7);
      set_cell(ut_new_block, index, ' ', 7);
   }
   set_cell(ut_new_block, 0, 'A', 3);
   set_cell(ut_new_block, 6, 'B', 3);

   UT_ASSERT_EQ_INT(TRUE, utt_od_puttext(1, 1, 7, 1, ut_new_block));
   UT_ASSERT(ut_disp_count >= 4);
}

static void avatar_full_put_and_scroll_suppression_are_honoured(void)
{
   reset_fixture();
   od_control.baud = 38400;
   od_control.user_ansi = FALSE;
   od_control.user_avatar = TRUE;
   od_control.od_full_put = TRUE;
   bScrollAction = FALSE;
   set_cell(ut_new_block, 0, 'A', 3);
   set_cell(ut_new_block, 1, 'B', 3);

   UT_ASSERT_EQ_INT(TRUE, utt_od_puttext(1, 1, 2, 1, ut_new_block));
   UT_ASSERT(ut_disp_count >= 3);
   UT_ASSERT_EQ_UINT(0, ut_mock_count(MOCK_CURSOR));
}

static const UTTestCase ut_cases[] = {
   {"parameter validation", each_parameter_error_is_rejected},
   {"avatar limits", avatar_limits_each_dimension},
   {"size and graphics failures", size_and_graphics_failures_are_reported},
   {"local screen backends", local_put_uses_each_screen_backend},
   {"remote setup failures", remote_allocation_and_screen_failures_are_reported},
   {"unchanged remote block", unchanged_remote_block_emits_no_text},
   {"ANSI changed runs", ansi_remote_block_skips_long_unchanged_run},
   {"AVATAR changed runs", avatar_changed_run_repositions_after_shorter_gap},
   {"AVATAR full put", avatar_full_put_and_scroll_suppression_are_honoured},
   {"terminal session", terminal_session_is_rejected}
};
