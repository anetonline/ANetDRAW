static void maps_row_and_column_to_interleaved_cell(void)
{
   static BYTE cells[40];
   SessionScreen.pCells = cells;
   SessionScreen.nWidth = 5;
   UT_ASSERT_EQ_PTR(cells, utt_ODSessionScreenCell(0, 0));
   UT_ASSERT_EQ_PTR(cells + 26, utt_ODSessionScreenCell(3, 2));
}

static const UTTestCase ut_cases[] = {
   {"cell offset", maps_row_and_column_to_interleaved_cell}
};
