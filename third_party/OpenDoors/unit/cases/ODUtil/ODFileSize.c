#define UT_CUSTOM_MOCK_fseek
#define UT_CUSTOM_MOCK_ftell

static long ut_positions[2];
static unsigned ut_tell_index;
static unsigned ut_seek_index;

long utm_ftell(FILE *file)
{
   UT_ASSERT_NOT_NULL(file);
   UT_ASSERT(ut_tell_index < 2);
   return ut_positions[ut_tell_index++];
}

int utm_fseek(FILE *file, long offset, int origin)
{
   UT_ASSERT_NOT_NULL(file);
   if(ut_seek_index == 0)
   {
      UT_ASSERT_EQ_INT(0, offset);
      UT_ASSERT_EQ_INT(SEEK_END, origin);
   }
   else
   {
      UT_ASSERT_EQ_INT(17, offset);
      UT_ASSERT_EQ_INT(SEEK_SET, origin);
   }
   ++ut_seek_index;
   return 0;
}

static void measures_and_restores_file_position(void)
{
   FILE *file = (FILE *)1;
   ut_positions[0] = 17;
   ut_positions[1] = 1234;
   ut_tell_index = 0;
   ut_seek_index = 0;
   UT_ASSERT_EQ_UINT(1234, utt_ODFileSize(file));
   UT_ASSERT_EQ_UINT(2, ut_tell_index);
   UT_ASSERT_EQ_UINT(2, ut_seek_index);
}

static const UTTestCase ut_cases[] = {
   {"file size", measures_and_restores_file_position}
};
