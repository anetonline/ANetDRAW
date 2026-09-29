#define UT_CUSTOM_MOCK_ODAddANSIParameter
#define UT_CUSTOM_MOCK_ODScrnSetAttribute
#define UT_CUSTOM_MOCK_ODSessionScreenAvailable
#define UT_CUSTOM_MOCK_ODSessionScreenPresent
#define UT_CUSTOM_MOCK_ODSessionScreenSetAttribute
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_od_disp
#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_strcat
#define UT_CUSTOM_MOCK_strlen

static unsigned ut_init_calls;
static unsigned ut_entries;
static unsigned ut_exits;
static int ut_parameters[8];
static unsigned ut_parameter_count;
static BOOL ut_session_available;
static unsigned ut_session_attributes;
static unsigned ut_present_calls;
static unsigned ut_local_attributes;
static BYTE ut_seen_attribute;
static unsigned ut_disp_calls;
static char ut_disp_bytes[8];
static INT ut_disp_size;
static BOOL ut_init_succeeds;

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds) bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void) { ++ut_entries; }
void utm_ODSyncAPIExit(void) { ++ut_exits; }

void utm_ODAddANSIParameter(char *sequence, int parameter)
{
   UT_ASSERT(ut_parameter_count < sizeof(ut_parameters) /
      sizeof(ut_parameters[0]));
   ut_parameters[ut_parameter_count++] = parameter;
   bAnyColorChangeYet = TRUE;
   sequence[0] = 'x';
   sequence[1] = '\0';
}

BOOL utm_ODSessionScreenAvailable(void) { return ut_session_available; }

void utm_ODSessionScreenSetAttribute(BYTE attribute)
{
   ++ut_session_attributes;
   ut_seen_attribute = attribute;
}

void utm_ODSessionScreenPresent(void) { ++ut_present_calls; }

void ODCALL utm_ODScrnSetAttribute(BYTE attribute)
{
   ++ut_local_attributes;
   ut_seen_attribute = attribute;
}

void ODCALL utm_od_disp(const char *buffer, INT size, BOOL local_echo)
{
   INT index;
   UT_ASSERT(size >= 0 && size <= (INT)sizeof(ut_disp_bytes));
   UT_ASSERT_EQ_INT(FALSE, local_echo);
   ++ut_disp_calls;
   ut_disp_size = size;
   for(index = 0; index < size; ++index) ut_disp_bytes[index] = buffer[index];
}

char *utm_strcat(char *destination, const char *source)
{
   char *result = destination;
   while(*destination != '\0') ++destination;
   while((*destination++ = *source++) != '\0') { }
   return result;
}

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

static void reset_attribute(void)
{
   bODInitialized = TRUE;
   od_control.user_avatar = FALSE;
   od_control.user_ansi = FALSE;
   od_control.od_full_color = FALSE;
   od_control.od_cur_attrib = 0;
   od_control.od_error = 0;
   bAnyColorChangeYet = FALSE;
   ut_init_calls = 0;
   ut_entries = 0;
   ut_exits = 0;
   ut_parameter_count = 0;
   ut_session_available = FALSE;
   ut_session_attributes = 0;
   ut_present_calls = 0;
   ut_local_attributes = 0;
   ut_seen_attribute = 0;
   ut_disp_calls = 0;
   ut_init_succeeds = TRUE;
   ut_disp_size = 0;
}

static void terminal_session_is_rejected(void)
{
   reset_attribute(); bODInitialized = FALSE; ut_init_succeeds = FALSE;
   utt_od_set_attrib(7);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entries);
}

static void assert_parameters(const int *expected, unsigned count)
{
   unsigned index;
   UT_ASSERT_EQ_UINT(count, ut_parameter_count);
   for(index = 0; index < count && index < ut_parameter_count; ++index)
      UT_ASSERT_EQ_INT(expected[index], ut_parameters[index]);
}

static void minus_one_returns_after_initialization(void)
{
   reset_attribute();
   bODInitialized = FALSE;
   utt_od_set_attrib(-1);
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(1, ut_entries);
   UT_ASSERT_EQ_UINT(1, ut_exits);
   UT_ASSERT_EQ_UINT(0, ut_disp_calls);
   UT_ASSERT_EQ_UINT(0, ut_local_attributes);
}

static void avatar_mode_suppresses_unchanged_attributes(void)
{
   reset_attribute();
   od_control.user_avatar = TRUE;
   od_control.od_cur_attrib = 0x12;
   utt_od_set_attrib(0x12);
   UT_ASSERT_EQ_UINT(0, ut_disp_calls);
   UT_ASSERT_EQ_UINT(0, ut_local_attributes);
}

static void avatar_mode_updates_each_screen_backend_and_remote(void)
{
   reset_attribute();
   od_control.user_avatar = TRUE;
   od_control.od_cur_attrib = 0;
   ut_session_available = TRUE;
   utt_od_set_attrib(0x1e);
   UT_ASSERT_EQ_UINT(1, ut_session_attributes);
   UT_ASSERT_EQ_UINT(1, ut_present_calls);
   UT_ASSERT_EQ_UINT(0, ut_local_attributes);
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
   UT_ASSERT_EQ_INT(3, ut_disp_size);
   UT_ASSERT_EQ_UINT(22, (unsigned char)ut_disp_bytes[0]);
   UT_ASSERT_EQ_UINT(1, (unsigned char)ut_disp_bytes[1]);
   UT_ASSERT_EQ_UINT(0x1e, (unsigned char)ut_disp_bytes[2]);

   reset_attribute();
   od_control.user_avatar = TRUE;
   od_control.od_cur_attrib = 0x1e;
   od_control.od_full_color = TRUE;
   utt_od_set_attrib(0x1e);
   UT_ASSERT_EQ_UINT(1, ut_local_attributes);
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
}

static void ansi_unknown_state_resets_and_emits_every_requested_modifier(void)
{
   static const int expected[] = {0, 5, 1, 31, 44};
   reset_attribute();
   od_control.user_ansi = TRUE;
   od_control.od_cur_attrib = -1;
   ut_session_available = TRUE;
   utt_od_set_attrib(0x9c);
   assert_parameters(expected, 5);
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
   UT_ASSERT_EQ_UINT(1, ut_session_attributes);
   UT_ASSERT_EQ_UINT(1, ut_present_calls);
   UT_ASSERT_EQ_UINT(0x9c, od_control.od_cur_attrib);
}

static void ansi_full_color_forces_reset_and_both_colors(void)
{
   static const int expected[] = {0, 32, 44};
   reset_attribute();
   od_control.user_ansi = TRUE;
   od_control.od_cur_attrib = 0x12;
   od_control.od_full_color = TRUE;
   utt_od_set_attrib(0x12);
   assert_parameters(expected, 3);
   UT_ASSERT_EQ_UINT(1, ut_local_attributes);
}

static void ansi_disabling_blink_or_brightness_resets_all_attributes(void)
{
   static const int expected[] = {0, 30, 40};
   reset_attribute();
   od_control.user_ansi = TRUE;
   od_control.od_cur_attrib = 0x80;
   utt_od_set_attrib(0);
   assert_parameters(expected, 3);

   reset_attribute();
   od_control.user_ansi = TRUE;
   od_control.od_cur_attrib = 0x08;
   utt_od_set_attrib(0);
   assert_parameters(expected, 3);
}

static void ansi_enables_blink_and_brightness_independently(void)
{
   static const int blink[] = {5};
   static const int bright[] = {1};
   reset_attribute();
   od_control.user_ansi = TRUE;
   utt_od_set_attrib(0x80);
   assert_parameters(blink, 1);

   reset_attribute();
   od_control.user_ansi = TRUE;
   utt_od_set_attrib(0x08);
   assert_parameters(bright, 1);
}

static void ansi_retains_enabled_blink_and_brightness_without_a_reset(void)
{
   reset_attribute();
   od_control.user_ansi = TRUE;
   od_control.od_cur_attrib = 0x88;
   utt_od_set_attrib(0x88);
   assert_parameters(NULL, 0);
   UT_ASSERT_EQ_UINT(0, ut_disp_calls);

   reset_attribute();
   od_control.user_ansi = TRUE;
   od_control.od_cur_attrib = 0x08;
   utt_od_set_attrib(0x08);
   assert_parameters(NULL, 0);
   UT_ASSERT_EQ_UINT(0, ut_disp_calls);
}

static void ansi_unknown_state_forces_matching_color_components(void)
{
   static const int expected[] = {0, 37, 47};
   reset_attribute();
   od_control.user_ansi = TRUE;
   od_control.od_cur_attrib = -1;
   utt_od_set_attrib(0x77);
   assert_parameters(expected, 3);
}

static void ansi_changes_each_color_and_suppresses_an_exact_match(void)
{
   static const int foreground[] = {34};
   static const int background[] = {44};
   reset_attribute();
   od_control.user_ansi = TRUE;
   utt_od_set_attrib(0x01);
   assert_parameters(foreground, 1);

   reset_attribute();
   od_control.user_ansi = TRUE;
   utt_od_set_attrib(0x10);
   assert_parameters(background, 1);

   reset_attribute();
   od_control.user_ansi = TRUE;
   utt_od_set_attrib(0);
   assert_parameters(NULL, 0);
   UT_ASSERT_EQ_UINT(0, ut_disp_calls);
   UT_ASSERT_EQ_UINT(1, ut_local_attributes);
}

static void plain_text_mode_reports_no_graphics(void)
{
   reset_attribute();
   utt_od_set_attrib(7);
   UT_ASSERT_EQ_INT(ERR_NOGRAPHICS, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_disp_calls);
   UT_ASSERT_EQ_UINT(0, ut_local_attributes);
   UT_ASSERT_EQ_UINT(1, ut_exits);
}

static const UTTestCase ut_cases[] = {
   {"no attribute change", minus_one_returns_after_initialization},
   {"unchanged AVATAR", avatar_mode_suppresses_unchanged_attributes},
   {"AVATAR output", avatar_mode_updates_each_screen_backend_and_remote},
   {"unknown ANSI", ansi_unknown_state_resets_and_emits_every_requested_modifier},
   {"full ANSI", ansi_full_color_forces_reset_and_both_colors},
   {"ANSI reset", ansi_disabling_blink_or_brightness_resets_all_attributes},
   {"ANSI modifiers", ansi_enables_blink_and_brightness_independently},
   {"retained ANSI modifiers", ansi_retains_enabled_blink_and_brightness_without_a_reset},
   {"matching unknown ANSI", ansi_unknown_state_forces_matching_color_components},
   {"ANSI colors", ansi_changes_each_color_and_suppresses_an_exact_match},
   {"plain text", plain_text_mode_reports_no_graphics},
   {"terminal session", terminal_session_is_rejected}
};
