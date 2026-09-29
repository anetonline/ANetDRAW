#define UT_CUSTOM_MOCK_ODSessionScreenIsEmulating
#define UT_CUSTOM_MOCK_ODSessionScreenCopyText
#define UT_CUSTOM_MOCK_ODScrnCopyText

static BOOL ut_emulating;
static unsigned ut_virtual_calls;
static unsigned ut_local_calls;
static INT ut_values[6];

BOOL utm_ODSessionScreenIsEmulating(void)
{
   return ut_emulating;
}

static void capture(INT left, INT top, INT right, INT bottom,
   INT destination_column, INT destination_row)
{
   ut_values[0] = left;
   ut_values[1] = top;
   ut_values[2] = right;
   ut_values[3] = bottom;
   ut_values[4] = destination_column;
   ut_values[5] = destination_row;
}

BOOL utm_ODSessionScreenCopyText(INT left, INT top, INT right, INT bottom,
   INT destination_column, INT destination_row)
{
   ++ut_virtual_calls;
   capture(left, top, right, bottom, destination_column, destination_row);
   return TRUE;
}

BOOL utm_ODScrnCopyText(BYTE left, BYTE top, BYTE right, BYTE bottom,
   BYTE destination_column, BYTE destination_row)
{
   ++ut_local_calls;
   capture(left, top, right, bottom, destination_column, destination_row);
   return TRUE;
}

static void assert_values(INT first)
{
   unsigned index;
   for(index = 0; index < DIM(ut_values); ++index)
      UT_ASSERT_EQ_INT(first + (INT)index, ut_values[index]);
}

static void dispatches_to_each_screen_backend(void)
{
   ut_virtual_calls = ut_local_calls = 0;
   ut_emulating = TRUE;
   utt_ODEmulateCopyText(300, 301, 302, 303, 304, 305);
   UT_ASSERT_EQ_UINT(1, ut_virtual_calls);
   UT_ASSERT_EQ_UINT(0, ut_local_calls);
   assert_values(300);

   ut_emulating = FALSE;
   utt_ODEmulateCopyText(1, 2, 3, 4, 5, 6);
   UT_ASSERT_EQ_UINT(1, ut_virtual_calls);
   UT_ASSERT_EQ_UINT(1, ut_local_calls);
   assert_values(1);
}

static const UTTestCase ut_cases[] = {
   {"backends", dispatches_to_each_screen_backend}
};
