#define UT_CUSTOM_MOCK_od_putch
#define UT_CUSTOM_MOCK_od_repeat
#define UT_CUSTOM_MOCK_od_set_attrib
#define UT_CUSTOM_MOCK_od_set_cursor

enum {
   UT_CALL_CURSOR = 1,
   UT_CALL_ATTRIB,
   UT_CALL_PUTCH,
   UT_CALL_REPEAT
};

typedef struct {
   BYTE kind;
   INT first;
   INT second;
} UTDisplayCall;

static UTDisplayCall ut_calls[32];
static unsigned ut_call_count;

static void record_call(BYTE kind, INT first, INT second)
{
   UT_ASSERT(ut_call_count < sizeof(ut_calls) / sizeof(ut_calls[0]));
   ut_calls[ut_call_count].kind = kind;
   ut_calls[ut_call_count].first = first;
   ut_calls[ut_call_count].second = second;
   ++ut_call_count;
}

void ODCALL utm_od_set_cursor(INT row, INT column)
{
   record_call(UT_CALL_CURSOR, row, column);
}

void ODCALL utm_od_set_attrib(INT attribute)
{
   record_call(UT_CALL_ATTRIB, attribute, 0);
}

void ODCALL utm_od_putch(char character)
{
   record_call(UT_CALL_PUTCH, (unsigned char)character, 0);
}

void ODCALL utm_od_repeat(char character, BYTE count)
{
   record_call(UT_CALL_REPEAT, (unsigned char)character, count);
}

static void reset_display(void)
{
   memset(&od_control, 0, sizeof(od_control));
   od_control.od_menu_text_col = 1;
   od_control.od_menu_key_col = 2;
   od_control.od_menu_highlight_col = 3;
   od_control.od_menu_highkey_col = 4;
   ut_call_count = 0;
}

static void highlights_key_pads_text_and_restores_cursor(void)
{
   tMenuItem items[1];
   reset_display();
   strcpy(items[0].szItemText, "ABC");
   items[0].btKeyIndex = 1;

   utt_ODPopupDisplayMenuItem(5, 7, items, 0, TRUE, 5, TRUE);

   UT_ASSERT_EQ_UINT(10, ut_call_count);
   UT_ASSERT_EQ_INT(UT_CALL_CURSOR, ut_calls[0].kind);
   UT_ASSERT_EQ_INT(8, ut_calls[0].first);
   UT_ASSERT_EQ_INT(6, ut_calls[0].second);
   UT_ASSERT_EQ_INT(3, ut_calls[1].first);
   UT_ASSERT_EQ_INT(' ', ut_calls[2].first);
   UT_ASSERT_EQ_INT('A', ut_calls[3].first);
   UT_ASSERT_EQ_INT(4, ut_calls[4].first);
   UT_ASSERT_EQ_INT('B', ut_calls[5].first);
   UT_ASSERT_EQ_INT(3, ut_calls[6].first);
   UT_ASSERT_EQ_INT('C', ut_calls[7].first);
   UT_ASSERT_EQ_INT(UT_CALL_REPEAT, ut_calls[8].kind);
   UT_ASSERT_EQ_INT(' ', ut_calls[8].first);
   UT_ASSERT_EQ_INT(3, ut_calls[8].second);
   UT_ASSERT_EQ_INT(UT_CALL_CURSOR, ut_calls[9].kind);
}

static void clips_unhighlighted_text_without_positioning_cursor(void)
{
   tMenuItem items[1];
   reset_display();
   strcpy(items[0].szItemText, "LONG");
   items[0].btKeyIndex = 3;

   utt_ODPopupDisplayMenuItem(1, 1, items, 0, FALSE, 2, FALSE);

   UT_ASSERT_EQ_UINT(5, ut_call_count);
   UT_ASSERT_EQ_INT(UT_CALL_ATTRIB, ut_calls[0].kind);
   UT_ASSERT_EQ_INT(1, ut_calls[0].first);
   UT_ASSERT_EQ_INT(' ', ut_calls[1].first);
   UT_ASSERT_EQ_INT('L', ut_calls[2].first);
   UT_ASSERT_EQ_INT('O', ut_calls[3].first);
   UT_ASSERT_EQ_INT(UT_CALL_REPEAT, ut_calls[4].kind);
   UT_ASSERT_EQ_INT(1, ut_calls[4].second);
}

static const UTTestCase ut_cases[] = {
   {"highlighted hotkey", highlights_key_pads_text_and_restores_cursor},
   {"clipped plain item", clips_unhighlighted_text_without_positioning_cursor}
};
