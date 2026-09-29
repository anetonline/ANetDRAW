#define UT_CUSTOM_MOCK_ODComCP437ToUnicodeLen
#define UT_CUSTOM_MOCK_malloc

static BYTE ut_output[16];
static BOOL ut_length_succeeds;
static BOOL ut_malloc_succeeds;
static size_t ut_forced_need;
static size_t ut_malloc_size;

BOOL utm_ODComCP437ToUnicodeLen(const BYTE *buffer, int size, size_t *length)
{
   (void)buffer;
   (void)size;
   if(!ut_length_succeeds)
      return FALSE;
   *length = ut_forced_need;
   return TRUE;
}

void *utm_malloc(size_t size)
{
   ut_malloc_size = size;
   return ut_malloc_succeeds ? ut_output : NULL;
}

static void reset_conversion(void)
{
   memset(ut_output, 0, sizeof(ut_output));
   memset(&od_control, 0, sizeof(od_control));
   ut_length_succeeds = TRUE;
   ut_malloc_succeeds = TRUE;
   ut_forced_need = 0;
   ut_malloc_size = 0;
}

static void rejects_each_invalid_input_and_excessive_result(void)
{
   BYTE byte = 'A';
   int size = 1;
   reset_conversion();
   UT_ASSERT_NULL(utt_ODComCP437ToUnicode(NULL, &size));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);

   reset_conversion();
   UT_ASSERT_NULL(utt_ODComCP437ToUnicode(&byte, NULL));

   reset_conversion();
   size = -1;
   UT_ASSERT_NULL(utt_ODComCP437ToUnicode(&byte, &size));

   reset_conversion();
   size = 1;
   ut_length_succeeds = FALSE;
   UT_ASSERT_NULL(utt_ODComCP437ToUnicode(&byte, &size));

   reset_conversion();
   size = 1;
   ut_forced_need = (size_t)INT_MAX + 1U;
   UT_ASSERT_NULL(utt_ODComCP437ToUnicode(&byte, &size));
   UT_ASSERT_EQ_INT(ERR_LIMIT, od_control.od_error);
}

static void reports_allocation_failure_for_a_valid_size(void)
{
   BYTE byte = 'A';
   int size = 1;
   reset_conversion();
   ut_forced_need = 1;
   ut_malloc_succeeds = FALSE;
   UT_ASSERT_NULL(utt_ODComCP437ToUnicode(&byte, &size));
   UT_ASSERT_EQ_UINT(1, ut_malloc_size);
   UT_ASSERT_EQ_INT(ERR_MEMORY, od_control.od_error);
}

static void allocates_one_byte_for_an_empty_conversion(void)
{
   BYTE byte = 0;
   int size = 0;
   reset_conversion();
   ut_forced_need = 0;
   UT_ASSERT_EQ_PTR(ut_output, utt_ODComCP437ToUnicode(&byte, &size));
   UT_ASSERT_EQ_UINT(1, ut_malloc_size);
   UT_ASSERT_EQ_INT(0, size);
}

static void emits_ascii_two_byte_and_three_byte_utf8_sequences(void)
{
   static BYTE input[] = {'A', 0x80, 0xb0};
   int size = sizeof(input);
   reset_conversion();
   ut_forced_need = 6;
   UT_ASSERT_EQ_PTR(ut_output, utt_ODComCP437ToUnicode(input, &size));
   UT_ASSERT_EQ_UINT(6, ut_malloc_size);
   UT_ASSERT_EQ_INT(6, size);
   UT_ASSERT_EQ_INT(0x41, ut_output[0]);
   UT_ASSERT_EQ_INT(0xc3, ut_output[1]);
   UT_ASSERT_EQ_INT(0x87, ut_output[2]);
   UT_ASSERT_EQ_INT(0xe2, ut_output[3]);
   UT_ASSERT_EQ_INT(0x96, ut_output[4]);
   UT_ASSERT_EQ_INT(0x91, ut_output[5]);
}

static const UTTestCase ut_cases[] = {
   {"invalid inputs", rejects_each_invalid_input_and_excessive_result},
   {"allocation failure", reports_allocation_failure_for_a_valid_size},
   {"empty", allocates_one_byte_for_an_empty_conversion},
   {"UTF-8", emits_ascii_two_byte_and_three_byte_utf8_sequences}
};
