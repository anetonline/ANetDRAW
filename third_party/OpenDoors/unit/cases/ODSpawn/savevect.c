#ifdef ODPLAT_DOS
#define UT_CUSTOM_MOCK__getvect

static unsigned ut_getvect_calls;

void ODSWAPCALL utm__getvect(int number, unsigned int *segment,
   unsigned int *offset)
{
   ++ut_getvect_calls;
   *segment = (unsigned int)(0x1000 + number);
   *offset = (unsigned int)(0x2000 + number);
}

static void copies_active_vectors_and_preserves_free_records(void)
{
   unsigned index;
   ut_getvect_calls = 0;
   utt_savevect();
   UT_ASSERT_EQ_UINT(6, ut_getvect_calls);
   for(index = 0; index < 6; ++index)
   {
      UT_ASSERT_EQ_INT(vectab1[index].number, vectab2[index].number);
      UT_ASSERT_EQ_INT(CURRENT, vectab2[index].flag);
      UT_ASSERT_EQ_UINT((unsigned)(0x1000 + vectab1[index].number),
         vectab2[index].vseg);
      UT_ASSERT_EQ_UINT((unsigned)(0x2000 + vectab1[index].number),
         vectab2[index].voff);
   }
   for(index = 6; index < 20; ++index)
      UT_ASSERT_EQ_INT(2, vectab2[index].flag);
   UT_ASSERT_EQ_INT(3, vectab2[20].flag);
}

static const UTTestCase ut_cases[] = {
   {"copy vector table", copies_active_vectors_and_preserves_free_records}
};
#endif
