#define UT_CUSTOM_MOCK_ODSizeAdd
#define UT_CUSTOM_MOCK_strcat
#define UT_CUSTOM_MOCK_strcpy
#define UT_CUSTOM_MOCK_strlen

static unsigned ut_add_call;
static unsigned ut_fail_add_call;

int utm_ODSizeAdd(size_t left, size_t right, size_t *result)
{
   ++ut_add_call;
   if(ut_add_call == ut_fail_add_call || right > (size_t)-1 - left)
      return 0;
   *result = left + right;
   return 1;
}

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

char *utm_strcpy(char *destination, const char *source)
{
   char *result = destination;
   while((*destination++ = *source++) != '\0') { }
   return result;
}

char *utm_strcat(char *destination, const char *source)
{
   char *result = destination;
   destination += utm_strlen(destination);
   while((*destination++ = *source++) != '\0') { }
   return result;
}

static tODResult make(char *output, const char *path, const char *name,
   INT size, unsigned fail_call)
{
   ut_add_call = 0;
   ut_fail_add_call = fail_call;
   return utt_ODMakeFilename(output, path, name, size);
}

static void constructs_empty_separated_and_in_place_paths(void)
{
   char output[64];
   char terminated[64];
   char in_place[64] = "path";
   terminated[0] = 'p';
   terminated[1] = 'a';
   terminated[2] = 't';
   terminated[3] = 'h';
   terminated[4] = DIRSEP;
   terminated[5] = '\0';
   UT_ASSERT_EQ_INT(kODRCSuccess, make(output, "", "file", sizeof(output), 0));
   UT_ASSERT_EQ_INT(0, strcmp("file", output));
   UT_ASSERT_EQ_INT(kODRCSuccess,
      make(output, terminated, "file", sizeof(output), 0));
   UT_ASSERT_EQ_INT(0, strcmp("file", output + 5));
   UT_ASSERT_EQ_INT(kODRCSuccess,
      make(in_place, in_place, "file", sizeof(in_place), 0));
   UT_ASSERT_EQ_INT(DIRSEP, in_place[4]);
   UT_ASSERT_EQ_INT(0, strcmp("file", in_place + 5));
}

static void rejects_each_size_failure_and_small_destination(void)
{
   char output[16] = "unchanged";
   UT_ASSERT_EQ_INT(kODRCFilenameTooLong,
      make(output, "path", "file", sizeof(output), 1));
   UT_ASSERT_EQ_INT(kODRCFilenameTooLong,
      make(output, "path", "file", sizeof(output), 2));
   UT_ASSERT_EQ_INT(kODRCFilenameTooLong,
      make(output, "path", "file", sizeof(output), 3));
   UT_ASSERT_EQ_INT(kODRCFilenameTooLong,
      make(output, "path", "file", 9, 0));
   UT_ASSERT_EQ_INT(0, strcmp("unchanged", output));
}

static const UTTestCase ut_cases[] = {
   {"construct filename", constructs_empty_separated_and_in_place_paths},
   {"filename too long", rejects_each_size_failure_and_small_destination}
};
