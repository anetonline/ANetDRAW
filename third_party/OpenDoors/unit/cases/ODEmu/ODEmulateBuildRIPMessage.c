#define UT_CUSTOM_MOCK_ODStringCopy
#define UT_CUSTOM_MOCK_strlen

static unsigned ut_copy_calls;
static char *ut_copy_destinations[2];
static INT ut_copy_sizes[2];

size_t utm_strlen(const char *text)
{
   const char *end = text;
   while(*end != '\0')
      ++end;
   return (size_t)(end - text);
}

void utm_ODStringCopy(char *destination, CONST char *source, INT size)
{
   INT index = 0;
   if(ut_copy_calls < DIM(ut_copy_destinations)) {
      ut_copy_destinations[ut_copy_calls] = destination;
      ut_copy_sizes[ut_copy_calls] = size;
   }
   ++ut_copy_calls;
   if(size <= 0)
      return;
   while(index + 1 < size && source[index] != '\0') {
      destination[index] = source[index];
      ++index;
   }
   destination[index] = '\0';
}

static void appends_the_filename_within_the_destination(void)
{
   char message[12];
   od_control.od_sending_rip = "Send ";
   ut_copy_calls = 0;
   memset(message, 'x', sizeof(message));

   utt_ODEmulateBuildRIPMessage(message, sizeof(message), "FILE.RIP");

   UT_ASSERT_EQ_INT(0, strcmp("Send FILE.R", message));
   UT_ASSERT_EQ_UINT(2, ut_copy_calls);
   UT_ASSERT_EQ_PTR(message, ut_copy_destinations[0]);
   UT_ASSERT_EQ_INT((INT)sizeof(message), ut_copy_sizes[0]);
   UT_ASSERT_EQ_PTR(message + 5, ut_copy_destinations[1]);
   UT_ASSERT_EQ_INT((INT)sizeof(message) - 5, ut_copy_sizes[1]);
}

static const UTTestCase ut_cases[] = {
   {"bounded message", appends_the_filename_within_the_destination}
};
