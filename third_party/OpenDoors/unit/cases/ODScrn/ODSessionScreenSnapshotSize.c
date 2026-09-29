#define UT_CUSTOM_MOCK_ODSessionSnapshotDimensions

static BOOL ut_dimensions_result;
static INT ut_width;
static INT ut_height;

BOOL utm_ODSessionSnapshotDimensions(INT *pnWidth, INT *pnHeight)
{
   *pnWidth = ut_width;
   *pnHeight = ut_height;
   return ut_dimensions_result;
}

static void reports_unavailable_and_overflowed_snapshots(void)
{
   ut_dimensions_result = FALSE;
   ut_width = 80;
   ut_height = 25;
   UT_ASSERT_EQ_UINT(0, utt_ODSessionScreenSnapshotSize());

   ut_dimensions_result = TRUE;
   ut_width = -1;
   ut_height = 1;
   UT_ASSERT_EQ_UINT(0, utt_ODSessionScreenSnapshotSize());
}

static void includes_header_and_interleaved_cells(void)
{
   ut_dimensions_result = TRUE;
   ut_width = 80;
   ut_height = 25;
   UT_ASSERT_EQ_UINT(4048, utt_ODSessionScreenSnapshotSize());
}

static const UTTestCase ut_cases[] = {
   {"unavailable or too large", reports_unavailable_and_overflowed_snapshots},
   {"snapshot bytes", includes_header_and_interleaved_cells}
};
