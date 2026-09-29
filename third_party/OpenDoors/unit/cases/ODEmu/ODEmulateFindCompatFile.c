#define UT_CUSTOM_MOCK_fopen
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_strcpy

static char ut_file_token;
static const char *ut_available_extension;
static unsigned ut_open_calls;
static char ut_opened[4][16];

size_t utm_strlen(const char *text)
{
   const char *end = text;
   while(*end != '\0')
      ++end;
   return (size_t)(end - text);
}

void *utm_memcpy(void *destination, const void *source, size_t count)
{
   BYTE *to = (BYTE *)destination;
   const BYTE *from = (const BYTE *)source;
   size_t index;
   for(index = 0; index < count; ++index)
      to[index] = from[index];
   return destination;
}

char *utm_strcpy(char *destination, const char *source)
{
   char *result = destination;
   while((*destination++ = *source++) != '\0')
      ;
   return result;
}

FILE *utm_fopen(const char *path, const char *mode)
{
   size_t index = 0;
   UT_ASSERT_EQ_INT(0, strcmp("rb", mode));
   if(ut_open_calls < DIM(ut_opened)) {
      while(index + 1 < sizeof(ut_opened[0]) && path[index] != '\0') {
         ut_opened[ut_open_calls][index] = path[index];
         ++index;
      }
      ut_opened[ut_open_calls][index] = '\0';
   }
   ++ut_open_calls;
   if(ut_available_extension != NULL) {
      size_t path_length = utm_strlen(path);
      size_t extension_length = utm_strlen(ut_available_extension);
      if(path_length >= extension_length
         && strcmp(path + path_length - extension_length,
            ut_available_extension) == 0)
         return (FILE *)&ut_file_token;
   }
   return NULL;
}

static void reset_search(const char *available)
{
   memset(ut_opened, 0, sizeof(ut_opened));
   ut_open_calls = 0;
   ut_available_extension = available;
}

static void skips_disabled_formats_and_finds_ascii(void)
{
   INT level = LEVEL_RIP;
   od_control.user_rip = FALSE;
   od_control.user_avatar = FALSE;
   od_control.user_ansi = FALSE;
   reset_search(".asc");

   UT_ASSERT_EQ_PTR(&ut_file_token,
      utt_ODEmulateFindCompatFile("MENU", &level));
   UT_ASSERT_EQ_INT(LEVEL_ASCII, level);
   UT_ASSERT_EQ_UINT(1, ut_open_calls);
   UT_ASSERT_EQ_INT(0, strcmp("MENU.asc", ut_opened[0]));
}

static void tries_enabled_formats_in_descending_order(void)
{
   INT level = LEVEL_RIP;
   od_control.user_rip = TRUE;
   od_control.user_avatar = TRUE;
   od_control.user_ansi = TRUE;
   reset_search(".ans");

   UT_ASSERT_EQ_PTR(&ut_file_token,
      utt_ODEmulateFindCompatFile("SCREEN", &level));
   UT_ASSERT_EQ_INT(LEVEL_ANSI, level);
   UT_ASSERT_EQ_UINT(3, ut_open_calls);
   UT_ASSERT_EQ_INT(0, strcmp("SCREEN.rip", ut_opened[0]));
   UT_ASSERT_EQ_INT(0, strcmp("SCREEN.avt", ut_opened[1]));
   UT_ASSERT_EQ_INT(0, strcmp("SCREEN.ans", ut_opened[2]));
}

static void reports_no_compatible_file(void)
{
   INT level = LEVEL_RIP;
   od_control.user_rip = TRUE;
   od_control.user_avatar = TRUE;
   od_control.user_ansi = TRUE;
   reset_search(NULL);

   UT_ASSERT_NULL(utt_ODEmulateFindCompatFile("NONE", &level));
   UT_ASSERT_EQ_INT(LEVEL_NONE, level);
   UT_ASSERT_EQ_UINT(4, ut_open_calls);

   level = LEVEL_NONE;
   reset_search(NULL);
   UT_ASSERT_NULL(utt_ODEmulateFindCompatFile("NONE", &level));
   UT_ASSERT_EQ_UINT(0, ut_open_calls);
}

static void rejects_an_unknown_level(void)
{
   INT level = LEVEL_RIP + 1;
   reset_search(NULL);
   UT_ASSERT_NULL(utt_ODEmulateFindCompatFile("NONE", &level));
   UT_ASSERT_EQ_UINT(0, ut_open_calls);
}

static const UTTestCase ut_cases[] = {
   {"disabled formats", skips_disabled_formats_and_finds_ascii},
   {"descending formats", tries_enabled_formats_in_descending_order},
   {"no file", reports_no_compatible_file},
   {"unknown level", rejects_an_unknown_level}
};
