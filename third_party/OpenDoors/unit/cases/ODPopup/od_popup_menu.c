#define UT_CUSTOM_MOCK_ODPopupCheckForKey
#define UT_CUSTOM_MOCK_ODPopupDisplayMenuItem
#define UT_CUSTOM_MOCK_ODStatEndArrowUse
#define UT_CUSTOM_MOCK_ODStatStartArrowUse
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_ODWaitDrain
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_od_disp
#define UT_CUSTOM_MOCK_od_gettext
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_putch
#define UT_CUSTOM_MOCK_od_puttext
#define UT_CUSTOM_MOCK_od_repeat
#define UT_CUSTOM_MOCK_od_set_attrib
#define UT_CUSTOM_MOCK_od_set_cursor
#define UT_CUSTOM_MOCK_strlen

#define UT_LEAVE_VALUE 30000
#define UT_MAX_CHECKS 64

typedef struct {
   INT command;
   INT correct_item;
   INT initialized;
} UTPopupAction;

static tMenuItem ut_menu_storage[MAX_MENU_ITEMS];
static unsigned char ut_window_storage[5000];
static UTPopupAction ut_actions[UT_MAX_CHECKS];
static unsigned ut_check_calls;
static BOOL ut_check_wait[UT_MAX_CHECKS];
static unsigned ut_display_calls;
static BOOL ut_display_highlight[32];
static BOOL ut_display_position[32];
static BYTE ut_display_item[32];
static unsigned ut_malloc_calls;
static unsigned ut_malloc_fail_call;
static size_t ut_malloc_sizes[3];
static unsigned ut_free_calls;
static void *ut_freed[3];
static BOOL ut_gettext_result;
static unsigned ut_gettext_calls;
static unsigned ut_puttext_calls;
static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static unsigned ut_arrow_starts;
static unsigned ut_arrow_ends;
static unsigned ut_drain_calls;
static BOOL ut_end_session_during_drain;
static unsigned ut_cursor_calls;
static unsigned ut_attrib_calls;
static unsigned ut_putch_calls;
static unsigned ut_repeat_calls;
static unsigned ut_disp_calls;
static BOOL ut_init_succeeds;

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

void *utm_malloc(size_t size)
{
   ++ut_malloc_calls;
   if(ut_malloc_calls <= sizeof(ut_malloc_sizes) / sizeof(ut_malloc_sizes[0]))
      ut_malloc_sizes[ut_malloc_calls - 1] = size;
   if(ut_malloc_calls == ut_malloc_fail_call)
      return NULL;
   if(ut_malloc_calls == 1)
      return ut_menu_storage;
   return ut_window_storage;
}

void utm_free(void *memory)
{
   if(memory == NULL)
      return;
   UT_ASSERT(ut_free_calls < sizeof(ut_freed) / sizeof(ut_freed[0]));
   ut_freed[ut_free_calls++] = memory;
}

void ODCALL utm_od_init(void)
{
   if(ut_init_succeeds) bODInitialized = TRUE;
   ++ut_init_calls;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

BOOL ODCALL utm_od_gettext(INT left, INT top, INT right, INT bottom,
   void *buffer)
{
   (void)left;
   (void)top;
   (void)right;
   (void)bottom;
   UT_ASSERT_EQ_PTR(ut_window_storage, buffer);
   ++ut_gettext_calls;
   return ut_gettext_result;
}

BOOL ODCALL utm_od_puttext(INT left, INT top, INT right, INT bottom,
   void *buffer)
{
   (void)left;
   (void)top;
   (void)right;
   (void)bottom;
   UT_ASSERT_EQ_PTR(ut_window_storage, buffer);
   ++ut_puttext_calls;
   return TRUE;
}

void utm_ODPopupCheckForKey(BOOL wait)
{
   UTPopupAction *action;
   UT_ASSERT(ut_check_calls < UT_MAX_CHECKS);
   ut_check_wait[ut_check_calls] = wait;
   action = &ut_actions[ut_check_calls++];
   if(action->command != UT_LEAVE_VALUE)
      nCommand = action->command;
   if(action->correct_item != UT_LEAVE_VALUE)
      btCorrectItem = (BYTE)action->correct_item;
   if(action->initialized != UT_LEAVE_VALUE)
      bODInitialized = action->initialized;
}

void utm_ODPopupDisplayMenuItem(BYTE left, BYTE top, tMenuItem *items,
   BYTE item, BOOL highlighted, BYTE width, BOOL position)
{
   (void)left;
   (void)top;
   (void)width;
   UT_ASSERT_EQ_PTR(ut_menu_storage, items);
   UT_ASSERT(ut_display_calls < sizeof(ut_display_item));
   ut_display_item[ut_display_calls] = item;
   ut_display_highlight[ut_display_calls] = highlighted;
   ut_display_position[ut_display_calls] = position;
   ++ut_display_calls;
}

void utm_ODStatStartArrowUse(void) { ++ut_arrow_starts; }
void utm_ODStatEndArrowUse(void) { ++ut_arrow_ends; }

void utm_ODWaitDrain(tODMilliSec milliseconds)
{
   UT_ASSERT_EQ_UINT(25, milliseconds);
   ++ut_drain_calls;
   if(ut_end_session_during_drain)
      bODInitialized = FALSE;
}

void ODCALL utm_od_set_cursor(INT row, INT column)
{
   (void)row;
   (void)column;
   ++ut_cursor_calls;
}

void ODCALL utm_od_set_attrib(INT attribute)
{
   (void)attribute;
   ++ut_attrib_calls;
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

void ODCALL utm_od_disp(const char *text, INT length, BOOL local)
{
   (void)text;
   (void)length;
   (void)local;
   ++ut_disp_calls;
}

static void reset_popup(void)
{
   unsigned index;
   memset(&od_control, 0, sizeof(od_control));
   memset(MenuLevelInfo, 0, sizeof(MenuLevelInfo));
   memset(ut_menu_storage, 0, sizeof(ut_menu_storage));
   memset(ut_window_storage, 0, sizeof(ut_window_storage));
   memset(ut_check_wait, 0, sizeof(ut_check_wait));
   memset(ut_display_highlight, 0, sizeof(ut_display_highlight));
   memset(ut_display_position, 0, sizeof(ut_display_position));
   memset(ut_display_item, 0, sizeof(ut_display_item));
   memset(ut_malloc_sizes, 0, sizeof(ut_malloc_sizes));
   memset(ut_freed, 0, sizeof(ut_freed));
   for(index = 0; index < UT_MAX_CHECKS; ++index)
   {
      ut_actions[index].command = UT_LEAVE_VALUE;
      ut_actions[index].correct_item = UT_LEAVE_VALUE;
      ut_actions[index].initialized = UT_LEAVE_VALUE;
   }
   od_control.user_ansi = TRUE;
   od_control.od_box_chars[BOX_TOP] = '-';
   od_control.od_box_chars[BOX_BOTTOM] = '-';
   od_control.od_box_chars[BOX_LEFT] = '|';
   od_control.od_box_chars[BOX_RIGHT] = '|';
   od_control.od_cur_attrib = 7;
   bODInitialized = TRUE;
   ut_gettext_result = TRUE;
   ut_check_calls = 0;
   ut_display_calls = 0;
   ut_malloc_calls = 0;
   ut_malloc_fail_call = 0;
   ut_free_calls = 0;
   ut_gettext_calls = 0;
   ut_puttext_calls = 0;
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   ut_arrow_starts = 0;
   ut_arrow_ends = 0;
   ut_drain_calls = 0;
   ut_end_session_during_drain = FALSE;
   ut_cursor_calls = 0;
   ut_attrib_calls = 0;
   ut_putch_calls = 0;
   ut_repeat_calls = 0;
   ut_disp_calls = 0;
   ut_init_succeeds = TRUE;
}

static void terminal_session_is_rejected(void)
{
   reset_popup(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT_EQ_INT(0, utt_od_popup_menu(NULL, "One", 1, 1, -1, 0));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void command_on_check(unsigned call, INT command)
{
   UT_ASSERT(call > 0 && call <= UT_MAX_CHECKS);
   ut_actions[call - 1].command = command;
}

static void rejects_both_out_of_range_levels_and_defaults_box_sides(void)
{
   reset_popup();
   bODInitialized = FALSE;
   od_control.od_box_chars[BOX_BOTTOM] = 0;
   od_control.od_box_chars[BOX_RIGHT] = 0;
   UT_ASSERT_EQ_INT(POPUP_ERROR,
      utt_od_popup_menu(NULL, "One", 1, 1, -1, 0));
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_INT('-', od_control.od_box_chars[BOX_BOTTOM]);
   UT_ASSERT_EQ_INT('|', od_control.od_box_chars[BOX_RIGHT]);
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);

   reset_popup();
   UT_ASSERT_EQ_INT(POPUP_ERROR,
      utt_od_popup_menu(NULL, "One", 1, 1, MENU_LEVELS, 0));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
}

static void rejects_null_text_and_menu_allocation_failure(void)
{
   reset_popup();
   UT_ASSERT_EQ_INT(POPUP_ERROR,
      utt_od_popup_menu(NULL, NULL, 1, 1, 0, 0));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_malloc_calls);

   reset_popup();
   ut_malloc_fail_call = 1;
   UT_ASSERT_EQ_INT(POPUP_ERROR,
      utt_od_popup_menu(NULL, "One", 1, 1, 0, 0));
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_free_calls);
}

static void rejects_an_empty_menu_and_lack_of_graphics(void)
{
   reset_popup();
   UT_ASSERT_EQ_INT(POPUP_ERROR,
      utt_od_popup_menu(NULL, "", 1, 1, 0, 0));
   UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_free_calls);

   reset_popup();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = FALSE;
   UT_ASSERT_EQ_INT(POPUP_ERROR,
      utt_od_popup_menu("Title", "^One|Two", 1, 1, 0, 0));
   UT_ASSERT_EQ_INT(ERR_NOGRAPHICS, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_free_calls);

   reset_popup();
   od_control.user_ansi = FALSE;
   od_control.user_avatar = TRUE;
   ut_malloc_fail_call = 2;
   UT_ASSERT_EQ_INT(POPUP_ERROR,
      utt_od_popup_menu(NULL, "One", 1, 1, 0, 0));
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);
}

static void rejects_each_invalid_window_edge(void)
{
   INT positions[6][2] = {
      {0, 1}, {1, 0}, {78, 1}, {1, 24}, {255, 1}, {1, 255}
   };
   unsigned index;
   for(index = 0; index < 6; ++index)
   {
      reset_popup();
      UT_ASSERT_EQ_INT(POPUP_ERROR, utt_od_popup_menu(
         NULL, "One", positions[index][0], positions[index][1], 0, 0));
      UT_ASSERT_EQ_INT(ERR_PARAMETER, od_control.od_error);
      UT_ASSERT_EQ_UINT(1, ut_free_calls);
   }
}

static void reports_window_allocation_and_capture_failures(void)
{
   reset_popup();
   ut_malloc_fail_call = 2;
   UT_ASSERT_EQ_INT(POPUP_ERROR,
      utt_od_popup_menu(NULL, "One", 1, 1, 0, 0));
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);
   UT_ASSERT_EQ_UINT(1, ut_free_calls);

   reset_popup();
   ut_gettext_result = FALSE;
   od_control.od_error = 123;
   UT_ASSERT_EQ_INT(POPUP_ERROR,
      utt_od_popup_menu(NULL, "One", 1, 1, 0, 0));
   UT_ASSERT_EQ_INT(123, od_control.od_error);
   UT_ASSERT_EQ_UINT(2, ut_free_calls);
}

static void exercises_parser_limits_and_terminating_delimiters(void)
{
   char repeated_width[] = "AAA|BBB|";
   char long_item[MAX_ITEM_WIDTH + 4];
   char maximum_items[MAX_MENU_ITEMS + 2];
   unsigned index;

   reset_popup();
   od_control.user_ansi = FALSE;
   UT_ASSERT_EQ_INT(POPUP_ERROR,
      utt_od_popup_menu(NULL, repeated_width, 1, 1, 0, 0));
   UT_ASSERT_EQ_INT(ERR_NOGRAPHICS, od_control.od_error);

   for(index = 0; index < MAX_ITEM_WIDTH; ++index)
      long_item[index] = 'x';
   long_item[MAX_ITEM_WIDTH] = '^';
   long_item[MAX_ITEM_WIDTH + 1] = 'y';
   long_item[MAX_ITEM_WIDTH + 2] = '\0';
   reset_popup();
   od_control.user_ansi = FALSE;
   UT_ASSERT_EQ_INT(POPUP_ERROR,
      utt_od_popup_menu(NULL, long_item, 1, 1, 0, 0));
   UT_ASSERT_EQ_INT(ERR_NOGRAPHICS, od_control.od_error);

   for(index = 0; index < MAX_MENU_ITEMS; ++index)
      maximum_items[index] = '|';
   maximum_items[MAX_MENU_ITEMS] = 'x';
   maximum_items[MAX_MENU_ITEMS + 1] = '\0';
   reset_popup();
   command_on_check(2, 1);
   UT_ASSERT_EQ_INT(1,
      utt_od_popup_menu(NULL, maximum_items, 1, 1, 0, 0));
   UT_ASSERT_EQ_UINT(MAX_MENU_ITEMS, btCurrentNumMenuItems);
}

static void handles_short_and_overlong_titles(void)
{
   char long_title[101];
   unsigned index;
   reset_popup();
   command_on_check(3, 1);
   UT_ASSERT_EQ_INT(1,
      utt_od_popup_menu("A", "Longer", 1, 1, 0, 0));

   for(index = 0; index < sizeof(long_title) - 1; ++index)
      long_title[index] = 'T';
   long_title[sizeof(long_title) - 1] = '\0';
   reset_popup();
   command_on_check(3, 1);
   UT_ASSERT_EQ_INT(1,
      utt_od_popup_menu(long_title, "X", 1, 1, 0, 0));
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
}

static void honors_commands_during_initial_rendering(void)
{
   reset_popup();
   command_on_check(2, 1);
   UT_ASSERT_EQ_INT(1,
      utt_od_popup_menu(NULL, "One|Two", 1, 1, 0, 0));
   UT_ASSERT_EQ_UINT(0, ut_arrow_starts);

   reset_popup();
   command_on_check(2, 1);
   UT_ASSERT_EQ_INT(1,
      utt_od_popup_menu(NULL, "One|Two", 1, 1, 0, MENU_KEEP));
   UT_ASSERT_EQ_PTR(ut_window_storage, MenuLevelInfo[0].pWindow);
}

static void renders_a_titled_menu_and_returns_a_selection(void)
{
   reset_popup();
   command_on_check(4, 2);
   UT_ASSERT_EQ_INT(2,
      utt_od_popup_menu("Menu", "^One|Two", 2, 3, 0, 0));
   UT_ASSERT_EQ_UINT(2, ut_display_calls);
   UT_ASSERT(ut_display_highlight[0]);
   UT_ASSERT(!ut_display_highlight[1]);
   UT_ASSERT_EQ_UINT(1, ut_arrow_starts);
   UT_ASSERT_EQ_UINT(1, ut_arrow_ends);
   UT_ASSERT_EQ_UINT(1, ut_puttext_calls);
   UT_ASSERT_EQ_UINT(2, ut_free_calls);
   UT_ASSERT_EQ_INT(7, od_control.od_cur_attrib);
   UT_ASSERT(ut_disp_calls > 0);
}

static void keeps_reuses_and_explicitly_destroys_a_menu(void)
{
   reset_popup();
   command_on_check(4, 1);
   UT_ASSERT_EQ_INT(1,
      utt_od_popup_menu(NULL, "One|Two", 2, 3, 1, MENU_KEEP));
   UT_ASSERT_EQ_PTR(ut_window_storage, MenuLevelInfo[1].pWindow);
   UT_ASSERT_EQ_PTR(ut_menu_storage, MenuLevelInfo[1].paMenuItems);
   UT_ASSERT_EQ_UINT(0, ut_free_calls);

   ut_check_calls = 0;
   command_on_check(1, 2);
   UT_ASSERT_EQ_INT(2,
      utt_od_popup_menu(NULL, NULL, 0, 0, 1, 0));
   UT_ASSERT_EQ_PTR(ut_window_storage, MenuLevelInfo[1].pWindow);

   ut_check_calls = 0;
   UT_ASSERT_EQ_INT(POPUP_ESCAPE,
      utt_od_popup_menu(NULL, NULL, 0, 0, 1, MENU_DESTROY));
   UT_ASSERT_EQ_PTR(NULL, MenuLevelInfo[1].pWindow);
   UT_ASSERT_EQ_PTR(NULL, MenuLevelInfo[1].paMenuItems);
   UT_ASSERT_EQ_UINT(2, ut_free_calls);
   UT_ASSERT_EQ_UINT(1, ut_puttext_calls);
}

static void destroys_kept_menus_for_nonpositive_commands(void)
{
   reset_popup();
   command_on_check(3, POPUP_ESCAPE);
   UT_ASSERT_EQ_INT(POPUP_ESCAPE,
      utt_od_popup_menu(NULL, "One", 1, 1, 0, MENU_KEEP));
   UT_ASSERT_EQ_UINT(2, ut_free_calls);
   UT_ASSERT_EQ_PTR(NULL, MenuLevelInfo[0].pWindow);
}

static void tolerates_a_kept_window_without_menu_storage_on_destroy(void)
{
   reset_popup();
   MenuLevelInfo[0].pWindow = ut_window_storage;
   MenuLevelInfo[0].paMenuItems = NULL;
   MenuLevelInfo[0].btRight = 4;
   MenuLevelInfo[0].btBottom = 3;
   MenuLevelInfo[0].btLeft = 1;
   MenuLevelInfo[0].btTop = 1;
   UT_ASSERT_EQ_INT(POPUP_ESCAPE,
      utt_od_popup_menu(NULL, NULL, 0, 0, 0, MENU_DESTROY));
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
   UT_ASSERT_EQ_PTR(ut_window_storage, ut_freed[0]);
}

static void updates_the_cursor_selection_before_returning(void)
{
   reset_popup();
   ut_actions[3].correct_item = 1;
   command_on_check(6, 2);
   UT_ASSERT_EQ_INT(2,
      utt_od_popup_menu(NULL, "One|Two", 1, 1, 0, 0));
   UT_ASSERT_EQ_UINT(4, ut_display_calls);
   UT_ASSERT(!ut_display_highlight[2]);
   UT_ASSERT(ut_display_position[2]);
   UT_ASSERT(ut_display_highlight[3]);
   UT_ASSERT(ut_display_position[3]);
   UT_ASSERT_EQ_UINT(1, ut_drain_calls);
}

static void cleans_up_when_the_session_ends_at_each_wait_boundary(void)
{
   reset_popup();
   ut_actions[2].initialized = FALSE;
   UT_ASSERT_EQ_INT(POPUP_ESCAPE,
      utt_od_popup_menu(NULL, "One", 1, 1, 0, 0));
   UT_ASSERT_EQ_UINT(2, ut_free_calls);
   UT_ASSERT_EQ_UINT(1, ut_arrow_ends);

   reset_popup();
   ut_actions[2].correct_item = 1;
   ut_end_session_during_drain = TRUE;
   UT_ASSERT_EQ_INT(POPUP_ESCAPE,
      utt_od_popup_menu(NULL, "One|Two", 1, 1, 0, 0));
   UT_ASSERT_EQ_UINT(2, ut_free_calls);

   reset_popup();
   ut_actions[2].correct_item = 1;
   ut_actions[4].initialized = FALSE;
   UT_ASSERT_EQ_INT(POPUP_ESCAPE,
      utt_od_popup_menu(NULL, "One|Two", 1, 1, 0, 0));
   UT_ASSERT_EQ_UINT(2, ut_free_calls);
}

static const UTTestCase ut_cases[] = {
   {"level validation", rejects_both_out_of_range_levels_and_defaults_box_sides},
   {"early parameter failures", rejects_null_text_and_menu_allocation_failure},
   {"menu and graphics validation", rejects_an_empty_menu_and_lack_of_graphics},
   {"window bounds", rejects_each_invalid_window_edge},
   {"window setup failures", reports_window_allocation_and_capture_failures},
   {"parser limits", exercises_parser_limits_and_terminating_delimiters},
   {"title widths", handles_short_and_overlong_titles},
   {"early selection", honors_commands_during_initial_rendering},
   {"normal selection", renders_a_titled_menu_and_returns_a_selection},
   {"kept menu lifecycle", keeps_reuses_and_explicitly_destroys_a_menu},
   {"kept cancellation", destroys_kept_menus_for_nonpositive_commands},
   {"partial kept menu", tolerates_a_kept_window_without_menu_storage_on_destroy},
   {"cursor update", updates_the_cursor_selection_before_returning},
   {"session end cleanup", cleans_up_when_the_session_ends_at_each_wait_boundary},
   {"terminal session", terminal_session_is_rejected}
};
