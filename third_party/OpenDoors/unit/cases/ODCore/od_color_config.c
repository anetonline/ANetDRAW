#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_od_strupr
#define UT_CUSTOM_MOCK_strcmp
#define UT_CUSTOM_MOCK_strncpy
#define UT_CUSTOM_MOCK_strupr

static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static BOOL ut_init_succeeds;

static void ut_copy_name(char *destination, const char *source)
{
   while((*destination++ = *source++) != '\0') { }
}

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

char *utm_strncpy(char *destination, const char *source, size_t count)
{
   size_t index;
   for(index = 0; index < count && source[index] != '\0'; ++index)
      destination[index] = source[index];
   while(index < count) destination[index++] = '\0';
   return destination;
}

static char *ut_upper_string(char *text)
{
   char *position;
   for(position = text; *position != '\0'; ++position)
      if(*position >= 'a' && *position <= 'z')
         *position = (char)(*position - 'a' + 'A');
   return text;
}

char *utm_od_strupr(char *text) { return ut_upper_string(text); }
char *utm_strupr(char *text) { return ut_upper_string(text); }

int utm_strcmp(const char *left, const char *right)
{
   while(*left != '\0' && *left == *right)
   {
      ++left;
      ++right;
   }
   return (unsigned char)*left - (unsigned char)*right;
}

static void reset_colors(void)
{
   static const char *names[12] = {
      "BLACK", "BLUE", "GREEN", "CYAN", "RED", "MAGENTA",
      "YELLOW", "WHITE", "BROWN", "GREY", "BRIGHT", "FLASHING"
   };
   unsigned index;
   bODInitialized = TRUE;
   chColorCheck = 0;
   pchColorEndPos = NULL;
   for(index = 0; index < 12; ++index)
      ut_copy_name(od_control.od_color_names[index], names[index]);
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   ut_init_succeeds = TRUE;
}

static void terminal_session_is_rejected(void)
{
   char description[] = "red";
   reset_colors(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   UT_ASSERT_EQ_UINT(0, utt_od_color_config(description));
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void empty_description_returns_the_default_after_initialization(void)
{
   char description[] = "";
   reset_colors();
   bODInitialized = FALSE;
   UT_ASSERT_EQ_UINT(0x07, utt_od_color_config(description));
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_PTR(description, pchColorEndPos);
}

static void parses_foreground_background_brightness_and_blink(void)
{
   char description[] = " \tred blue bright flashing unknown";
   reset_colors();
   UT_ASSERT_EQ_UINT(0x9c, utt_od_color_config(description));
   UT_ASSERT(*pchColorEndPos == '\0');
}

static void maps_brown_and_grey_to_pc_colour_indexes(void)
{
   char description[] = "brown\tgrey";
   reset_colors();
   UT_ASSERT_EQ_UINT(0x76, utt_od_color_config(description));
}

static void stops_at_the_configured_colour_marker(void)
{
   char description[] = "green|red";
   reset_colors();
   chColorCheck = '|';
   UT_ASSERT_EQ_UINT(0x02, utt_od_color_config(description));
   UT_ASSERT(*pchColorEndPos == '|');
}

static void truncates_an_overlong_unknown_token_and_continues(void)
{
   char description[64];
   unsigned index;
   reset_colors();
   for(index = 0; index < 45; ++index) description[index] = 'a';
   description[45] = ' ';
   description[46] = 'r';
   description[47] = 'e';
   description[48] = 'd';
   description[49] = '\0';
   UT_ASSERT_EQ_UINT(0x04, utt_od_color_config(description));
   UT_ASSERT(*pchColorEndPos == '\0');
}

static const UTTestCase ut_cases[] = {
   {"default colour", empty_description_returns_the_default_after_initialization},
   {"colour modifiers", parses_foreground_background_brightness_and_blink},
   {"brown and grey", maps_brown_and_grey_to_pc_colour_indexes},
   {"colour marker", stops_at_the_configured_colour_marker},
   {"overlong token", truncates_an_overlong_unknown_token_and_continues},
   {"terminal session", terminal_session_is_rejected}
};
