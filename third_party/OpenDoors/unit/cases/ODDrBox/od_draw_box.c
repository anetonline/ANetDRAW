#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_emulate
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_putch
#define UT_CUSTOM_MOCK_od_repeat
#define UT_CUSTOM_MOCK_od_set_cursor

#include <string.h>

static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_cursor_calls;
static unsigned ut_putch_calls;
static unsigned ut_repeat_calls;
static unsigned ut_emulate_calls;
static BOOL ut_init_succeeds;

static void reset_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   bODInitialized = TRUE;
   od_control.user_ansi = TRUE;
   od_control.od_box_chars[BOX_TOP] = '-';
   od_control.od_box_chars[BOX_BOTTOM] = '=';
   od_control.od_box_chars[BOX_LEFT] = '|';
   od_control.od_box_chars[BOX_RIGHT] = '!';
   od_control.od_box_chars[BOX_UPPERLEFT] = '1';
   od_control.od_box_chars[BOX_UPPERRIGHT] = '2';
   od_control.od_box_chars[BOX_LOWERLEFT] = '3';
   od_control.od_box_chars[BOX_LOWERRIGHT] = '4';
   ut_init_calls = ut_entries = ut_exits = 0;
   ut_cursor_calls = ut_putch_calls = 0;
   ut_repeat_calls = ut_emulate_calls = 0;
   ut_init_succeeds = TRUE;
}

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_fixture(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT(!utt_od_draw_box(2, 3, 8, 5));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_emulate_calls);
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

void ODCALL utm_od_set_cursor(INT row, INT column)
{
   UT_ASSERT(row >= 1);
   UT_ASSERT(column >= 1);
   ++ut_cursor_calls;
}

void ODCALL utm_od_putch(char character)
{
   (void)character;
   ++ut_putch_calls;
}

void ODCALL utm_od_repeat(char character, BYTE count)
{
   (void)character;
   (void)count;
   ++ut_repeat_calls;
}

void ODCALL utm_od_emulate(char value)
{
   (void)value;
   ++ut_emulate_calls;
}

static void rejects_display_without_graphics_and_initializes_defaults(void)
{
   reset_fixture();
   bODInitialized = FALSE;
   od_control.user_ansi = FALSE;
   od_control.user_avatar = FALSE;
   od_control.od_box_chars[BOX_BOTTOM] = 0;
   od_control.od_box_chars[BOX_RIGHT] = 0;
   UT_ASSERT(!utt_od_draw_box(1, 1, 5, 4));
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_INT('-', od_control.od_box_chars[BOX_BOTTOM]);
   UT_ASSERT_EQ_INT('|', od_control.od_box_chars[BOX_RIGHT]);
   UT_ASSERT_EQ_INT(ERR_NOGRAPHICS, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static void rejects_each_invalid_coordinate_independently(void)
{
   static const BYTE coordinates[][4] = {
      {0, 1, 5, 4}, {1, 0, 5, 4}, {1, 1, 81, 4},
      {1, 1, 5, 26}, {5, 1, 5, 4}, {1, 4, 5, 4}
   };
   unsigned index;
   for(index = 0; index < sizeof(coordinates) / sizeof(coordinates[0]);
      ++index)
   {
      reset_fixture();
      UT_ASSERT(!utt_od_draw_box(coordinates[index][0],
         coordinates[index][1], coordinates[index][2],
         coordinates[index][3]));
      UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
      UT_ASSERT_EQ_UINT(0, ut_cursor_calls);
   }
}

static void draws_ansi_box(void)
{
   reset_fixture();
   UT_ASSERT(utt_od_draw_box(2, 3, 8, 6));
   UT_ASSERT_EQ_UINT(0, ut_emulate_calls);
   UT_ASSERT_EQ_UINT(4, ut_cursor_calls);
   UT_ASSERT_EQ_UINT(4, ut_repeat_calls);
   UT_ASSERT_EQ_UINT(8, ut_putch_calls);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static void draws_avatar_boxes_with_and_without_remaining_lines(void)
{
   reset_fixture();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = TRUE;
   UT_ASSERT(utt_od_draw_box(2, 3, 8, 5));
   UT_ASSERT_EQ_UINT(5, ut_emulate_calls);

   reset_fixture();
   od_control.user_ansi = TRUE;
   od_control.user_avatar = TRUE;
   UT_ASSERT(utt_od_draw_box(2, 3, 8, 7));
   UT_ASSERT_EQ_UINT(5, ut_emulate_calls);
   UT_ASSERT(ut_cursor_calls > 4);
   UT_ASSERT_EQ_INT('=', od_control.od_box_chars[BOX_BOTTOM]);
   UT_ASSERT_EQ_INT('!', od_control.od_box_chars[BOX_RIGHT]);
}

static const UTTestCase ut_cases[] = {
   {"graphics required", rejects_display_without_graphics_and_initializes_defaults},
   {"coordinate validation", rejects_each_invalid_coordinate_independently},
   {"ANSI box", draws_ansi_box},
   {"AVATAR box", draws_avatar_boxes_with_and_without_remaining_lines},
   {"terminal session", terminal_session_is_rejected}
};
