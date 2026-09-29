#define UT_CUSTOM_MOCK_ODSessionScreenIsEmulating
#define UT_CUSTOM_MOCK_ODSessionScreenSetCursorPos
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos

static BOOL ut_emulating;
static unsigned ut_virtual_calls;
static unsigned ut_local_calls;
static INT ut_column;
static INT ut_row;

BOOL utm_ODSessionScreenIsEmulating(void)
{
   return ut_emulating;
}

void utm_ODSessionScreenSetCursorPos(INT column, INT row)
{
   ++ut_virtual_calls;
   ut_column = column;
   ut_row = row;
}

void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{
   ++ut_local_calls;
   ut_column = column;
   ut_row = row;
}

static void dispatches_to_each_screen_backend(void)
{
   ut_virtual_calls = ut_local_calls = 0;
   ut_emulating = TRUE;
   utt_ODEmulateSetCursorPos(300, 60);
   UT_ASSERT_EQ_UINT(1, ut_virtual_calls);
   UT_ASSERT_EQ_UINT(0, ut_local_calls);
   UT_ASSERT_EQ_INT(300, ut_column);
   UT_ASSERT_EQ_INT(60, ut_row);

   ut_emulating = FALSE;
   utt_ODEmulateSetCursorPos(7, 8);
   UT_ASSERT_EQ_UINT(1, ut_virtual_calls);
   UT_ASSERT_EQ_UINT(1, ut_local_calls);
   UT_ASSERT_EQ_INT(7, ut_column);
   UT_ASSERT_EQ_INT(8, ut_row);
}

static const UTTestCase ut_cases[] = {
   {"backends", dispatches_to_each_screen_backend}
};
