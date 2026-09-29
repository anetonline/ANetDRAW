#define UT_CUSTOM_MOCK_fwrite
#define UT_CUSTOM_MOCK_ODDropFileRecordWriteFailure

static size_t ut_write_result;
static unsigned ut_write_calls;
static unsigned ut_record_calls;

size_t utm_fwrite(const void *data, size_t size, size_t count, FILE *file)
{
   ++ut_write_calls;
   UT_ASSERT_NOT_NULL(data);
   UT_ASSERT_EQ_UINT(1, size);
   UT_ASSERT_EQ_UINT(7, count);
   UT_ASSERT_NOT_NULL(file);
   return ut_write_result;
}

void utm_ODDropFileRecordWriteFailure(tODDropFileWriter *writer)
{
   ++ut_record_calls;
   writer->nErrorCode = ERR_GENERALFAILURE;
}

static void suppresses_output_after_failure(void)
{
   tODDropFileWriter writer;
   char data[7] = {0};
   writer.pFile = (FILE *)data;
   writer.nErrorCode = ERR_FILEOPEN;
   ut_write_calls = ut_record_calls = 0;
   UT_ASSERT_EQ_INT(FALSE, utt_ODDropFileWrite(&writer, data, sizeof(data)));
   UT_ASSERT_EQ_UINT(0, ut_write_calls);
   UT_ASSERT_EQ_UINT(0, ut_record_calls);
}

static void reports_short_writes(void)
{
   tODDropFileWriter writer;
   char data[7] = {0};
   writer.pFile = (FILE *)data;
   writer.nErrorCode = ERR_NONE;
   ut_write_calls = ut_record_calls = 0;
   ut_write_result = 6;
   UT_ASSERT_EQ_INT(FALSE, utt_ODDropFileWrite(&writer, data, sizeof(data)));
   UT_ASSERT_EQ_UINT(1, ut_write_calls);
   UT_ASSERT_EQ_UINT(1, ut_record_calls);

   writer.nErrorCode = ERR_NONE;
   ut_write_result = 7;
   UT_ASSERT_EQ_INT(TRUE, utt_ODDropFileWrite(&writer, data, sizeof(data)));
   UT_ASSERT_EQ_UINT(2, ut_write_calls);
   UT_ASSERT_EQ_UINT(1, ut_record_calls);
}

static const UTTestCase ut_cases[] = {
   {"suppressed output", suppresses_output_after_failure},
   {"write result", reports_short_writes}
};
