static WORD ut_screen[2000];

static void scrolls_the_boundary_and_clears_its_bottom_row(void)
{
   unsigned index;
   for(index = 0; index < DIM(ut_screen); ++index)
      ut_screen[index] = 0x5555;
   pScrnBuffer = ut_screen;
   btLeftBoundary = 1; btTopBoundary = 1;
   btRightBoundary = 3; btBottomBoundary = 3;
   btCurrentAttribute = 0x2e;
   ut_screen[81] = 0x1101; ut_screen[82] = 0x1102; ut_screen[83] = 0x1103;
   ut_screen[161] = 0x2201; ut_screen[162] = 0x2202; ut_screen[163] = 0x2203;
   ut_screen[241] = 0x3301; ut_screen[242] = 0x3302; ut_screen[243] = 0x3303;

   utt_ODScrnScrollUpOneLine();

   UT_ASSERT_EQ_UINT(0x2201, ut_screen[81]);
   UT_ASSERT_EQ_UINT(0x2202, ut_screen[82]);
   UT_ASSERT_EQ_UINT(0x2203, ut_screen[83]);
   UT_ASSERT_EQ_UINT(0x3301, ut_screen[161]);
   UT_ASSERT_EQ_UINT(0x3302, ut_screen[162]);
   UT_ASSERT_EQ_UINT(0x3303, ut_screen[163]);
   UT_ASSERT_EQ_UINT(0x2e20, ut_screen[241]);
   UT_ASSERT_EQ_UINT(0x2e20, ut_screen[242]);
   UT_ASSERT_EQ_UINT(0x2e20, ut_screen[243]);
   UT_ASSERT_EQ_UINT(0x5555, ut_screen[80]);
   UT_ASSERT_EQ_UINT(0x5555, ut_screen[84]);
}

static void clears_a_one_row_boundary_without_copying(void)
{
   unsigned index;
   for(index = 0; index < DIM(ut_screen); ++index)
      ut_screen[index] = 0x5555;
   pScrnBuffer = ut_screen;
   btLeftBoundary = 1; btTopBoundary = 2;
   btRightBoundary = 3; btBottomBoundary = 2;
   btCurrentAttribute = 0x2e;

   utt_ODScrnScrollUpOneLine();

   UT_ASSERT_EQ_UINT(0x2e20, ut_screen[161]);
   UT_ASSERT_EQ_UINT(0x2e20, ut_screen[162]);
   UT_ASSERT_EQ_UINT(0x2e20, ut_screen[163]);
   UT_ASSERT_EQ_UINT(0x5555, ut_screen[81]);
   UT_ASSERT_EQ_UINT(0x5555, ut_screen[164]);
   UT_ASSERT_EQ_UINT(0x5555, ut_screen[241]);
}

static const UTTestCase ut_cases[] = {
   {"scroll boundary", scrolls_the_boundary_and_clears_its_bottom_row},
   {"one-row boundary", clears_a_one_row_boundary_without_copying}
};
