#define UT_CUSTOM_MOCK_vfprintf
#define UT_CUSTOM_MOCK_ODDropFileRecordWriteFailure

static int ut_print_result;
static unsigned ut_print_calls;
static unsigned ut_record_calls;

int utm_vfprintf(FILE *file, const char *format, va_list arguments)
{
   (void)arguments;
   ++ut_print_calls;
   UT_ASSERT_NOT_NULL(file);
   UT_ASSERT_EQ_INT(0, strcmp("%s %d", format));
   return ut_print_result;
}

void utm_ODDropFileRecordWriteFailure(tODDropFileWriter *writer)
{
   ++ut_record_calls;
   writer->nErrorCode = ERR_GENERALFAILURE;
}

static void suppresses_output_after_failure(void)
{
   tODDropFileWriter writer;
   writer.nErrorCode = ERR_FILEOPEN;
   ut_print_calls = ut_record_calls = 0;
   utt_ODTextDropFileWrite(&writer, "%s %d", "door", 1);
   UT_ASSERT_EQ_UINT(0, ut_print_calls);
   UT_ASSERT_EQ_UINT(0, ut_record_calls);
}

static void records_only_print_failures(void)
{
   tODDropFileWriter writer;
   writer.pFile = (FILE *)&writer;
   writer.nErrorCode = ERR_NONE;
   ut_print_calls = ut_record_calls = 0;
   ut_print_result = 0;
   utt_ODTextDropFileWrite(&writer, "%s %d", "door", 1);
   UT_ASSERT_EQ_UINT(1, ut_print_calls);
   UT_ASSERT_EQ_UINT(0, ut_record_calls);

   ut_print_result = -1;
   utt_ODTextDropFileWrite(&writer, "%s %d", "door", 1);
   UT_ASSERT_EQ_UINT(2, ut_print_calls);
   UT_ASSERT_EQ_UINT(1, ut_record_calls);
}

static const UTTestCase ut_cases[] = {
   {"suppressed text", suppresses_output_after_failure},
   {"formatted result", records_only_print_failures}
};
