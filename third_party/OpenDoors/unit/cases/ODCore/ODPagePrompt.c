#define UT_CUSTOM_MOCK_ODScrnGetTextInfo
#define UT_CUSTOM_MOCK_od_disp_str
#define UT_CUSTOM_MOCK_od_get_key
#define UT_CUSTOM_MOCK_od_set_attrib
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_tolower
#define UT_CUSTOM_MOCK_toupper

static char ut_keys[3];
static char ut_prompt_text[6];
static unsigned ut_key_count;
static unsigned ut_key_index;
static unsigned ut_shutdown_key;
static unsigned ut_text_info_calls;
static unsigned ut_attrib_calls;
static INT ut_attrib_values[2];
static unsigned ut_display_calls;

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

int utm_tolower(int value)
{
   UT_ASSERT_EQ_INT((int)(unsigned char)value, value);
   if(value >= 'A' && value <= 'Z') return value - 'A' + 'a';
   return value;
}

int utm_toupper(int value)
{
   UT_ASSERT_EQ_INT((int)(unsigned char)value, value);
   if(value >= 'a' && value <= 'z') return value - 'a' + 'A';
   return value;
}

void utm_ODScrnGetTextInfo(tODScrnTextInfo *info)
{
   UT_ASSERT(info != NULL);
   info->attribute = 0x1e;
   ++ut_text_info_calls;
}

void ODCALL utm_od_set_attrib(INT attribute)
{
   UT_ASSERT(ut_attrib_calls < 2);
   ut_attrib_values[ut_attrib_calls++] = attribute;
}

void ODCALL utm_od_disp_str(const char *text)
{
   if(ut_display_calls == 0)
      UT_ASSERT_EQ_PTR(od_control.od_continue, text);
   else
      UT_ASSERT_EQ_PTR(szBackspaceWithDelete, text);
   ++ut_display_calls;
}

char ODCALL utm_od_get_key(BOOL wait)
{
   UT_ASSERT_EQ_INT(TRUE, wait);
   if(ut_key_index >= ut_key_count)
   {
      UT_ASSERT(ut_key_index < ut_key_count);
      return od_control.od_continue_yes;
   }
   ++ut_key_index;
   if(ut_key_index == ut_shutdown_key) bODInitialized = FALSE;
   return ut_keys[ut_key_index - 1];
}

static void reset_prompt(void)
{
   char *destination = ut_prompt_text;
   const char *source = "More?";
   while((*destination++ = *source++) != '\0') { }
   od_control.od_continue = ut_prompt_text;
   od_control.od_continue_col = 0x2f;
   od_control.od_continue_yes = 'y';
   od_control.od_continue_nonstop = 'n';
   od_control.od_continue_no = 'q';
   od_control.baud = 0;
   hSerialPort = (tPortHandle)1;
   bODInitialized = TRUE;
   ut_key_count = 0;
   ut_key_index = 0;
   ut_shutdown_key = 0;
   ut_text_info_calls = 0;
   ut_attrib_calls = 0;
   ut_display_calls = 0;
}

static BOOL run_prompt_key(char key, BOOL *pausing)
{
   ut_keys[0] = key;
   ut_key_count = 1;
   return utt_ODPagePrompt(pausing);
}

static void assert_normal_prompt_cleanup(void)
{
   UT_ASSERT_EQ_UINT(1, ut_text_info_calls);
   UT_ASSERT_EQ_UINT(2, ut_attrib_calls);
   UT_ASSERT_EQ_INT(0x2f, ut_attrib_values[0]);
   UT_ASSERT_EQ_INT(0x1e, ut_attrib_values[1]);
   UT_ASSERT_EQ_UINT(6, ut_display_calls);
}

static void disabled_pausing_returns_without_display(void)
{
   BOOL pausing = FALSE;
   reset_prompt();
   UT_ASSERT_EQ_INT(FALSE, utt_ODPagePrompt(&pausing));
   UT_ASSERT_EQ_UINT(0, ut_text_info_calls);
   UT_ASSERT_EQ_UINT(0, ut_key_index);
   UT_ASSERT_EQ_UINT(0, ut_display_calls);
}

static void every_continue_key_preserves_pausing(void)
{
   char keys[] = {'y', 'Y', 13, ' '};
   unsigned index;
   for(index = 0; index < sizeof(keys); ++index)
   {
      BOOL pausing = TRUE;
      reset_prompt();
      UT_ASSERT_EQ_INT(FALSE, run_prompt_key(keys[index], &pausing));
      UT_ASSERT_EQ_INT(TRUE, pausing);
      assert_normal_prompt_cleanup();
   }
}

static void every_nonstop_key_disables_pausing(void)
{
   char keys[] = {'n', 'N'};
   unsigned index;
   for(index = 0; index < sizeof(keys); ++index)
   {
      BOOL pausing = TRUE;
      reset_prompt();
      UT_ASSERT_EQ_INT(FALSE, run_prompt_key(keys[index], &pausing));
      UT_ASSERT_EQ_INT(FALSE, pausing);
      assert_normal_prompt_cleanup();
   }
}

static void every_abort_key_stops_display(void)
{
   char keys[] = {'q', 'Q', 's', 'S', 3, 11, 0x18};
   unsigned index;
   for(index = 0; index < sizeof(keys); ++index)
   {
      BOOL pausing = TRUE;
      reset_prompt();
      od_control.baud = index == 0 ? 9600 : 0;
      UT_ASSERT_EQ_INT(TRUE, run_prompt_key(keys[index], &pausing));
      UT_ASSERT_EQ_INT(TRUE, pausing);
      assert_normal_prompt_cleanup();
   }
}

static void invalid_keys_are_ignored_until_a_valid_choice(void)
{
   BOOL pausing = TRUE;
   reset_prompt();
   ut_keys[0] = 'x';
   ut_keys[1] = 'y';
   ut_key_count = 2;
   UT_ASSERT_EQ_INT(FALSE, utt_ODPagePrompt(&pausing));
   UT_ASSERT_EQ_UINT(2, ut_key_index);
   assert_normal_prompt_cleanup();
}

static void accepts_a_high_bit_configured_response(void)
{
   BOOL pausing = TRUE;
   reset_prompt();
   od_control.od_continue_yes = (char)0x80;
   UT_ASSERT_EQ_INT(FALSE, run_prompt_key((char)0x80, &pausing));
   UT_ASSERT_EQ_INT(TRUE, pausing);
   assert_normal_prompt_cleanup();
}

static void shutdown_aborts_without_erasing_the_prompt(void)
{
   BOOL pausing = TRUE;
   reset_prompt();
   ut_keys[0] = 'y';
   ut_key_count = 1;
   ut_shutdown_key = 1;
   UT_ASSERT_EQ_INT(TRUE, utt_ODPagePrompt(&pausing));
   UT_ASSERT_EQ_UINT(1, ut_display_calls);
   UT_ASSERT_EQ_UINT(2, ut_attrib_calls);
}

static const UTTestCase ut_cases[] = {
   {"pausing disabled", disabled_pausing_returns_without_display},
   {"continue keys", every_continue_key_preserves_pausing},
   {"nonstop keys", every_nonstop_key_disables_pausing},
   {"abort keys", every_abort_key_stops_display},
   {"invalid key", invalid_keys_are_ignored_until_a_valid_choice},
   {"high-bit response", accepts_a_high_bit_configured_response},
   {"session shutdown", shutdown_aborts_without_erasing_the_prompt}
};
