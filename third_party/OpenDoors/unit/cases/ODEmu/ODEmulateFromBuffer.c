#include <stdarg.h>
#ifndef UT_TURBO_SHARD
#define UT_TURBO_SHARD 0
#endif
#define UT_CUSTOM_MOCK_ODSessionScreenAvailable
#define UT_CUSTOM_MOCK_ODSessionScreenBeginEmulation
#define UT_CUSTOM_MOCK_ODSessionScreenEndEmulation
#define UT_CUSTOM_MOCK_ODDWordDivide
#define UT_CUSTOM_MOCK_ODDWordMultiply
#define UT_CUSTOM_MOCK_ODTimerStart
#define UT_CUSTOM_MOCK_ODTimerWaitForElapse
#define UT_CUSTOM_MOCK_od_get_answer
#define UT_CUSTOM_MOCK_ODEmulateGetTextInfo
#define UT_CUSTOM_MOCK_ODEmulateCopyText
#define UT_CUSTOM_MOCK_ODEmulateSetCursorPos
#define UT_CUSTOM_MOCK_ODEmulateFillArea
#define UT_CUSTOM_MOCK_ODScrnDisplayChar
#define UT_CUSTOM_MOCK_ODScrnDisplayString
#define UT_CUSTOM_MOCK_ODScrnSetAttribute
#define UT_CUSTOM_MOCK_ODScrnClear
#define UT_CUSTOM_MOCK_ODScrnClearToEndOfLine
#define UT_CUSTOM_MOCK_ODScrnEnableScrolling
#define UT_CUSTOM_MOCK_od_disp_str
#define UT_CUSTOM_MOCK_od_printf
#define UT_CUSTOM_MOCK_od_exit
#define UT_CUSTOM_MOCK_strcmp
#define UT_CUSTOM_MOCK_atoi
#define UT_CUSTOM_MOCK_abs
#define UT_CUSTOM_MOCK_ODCoreSendRemoteByte

#define UT_EMU_MAX_CALLS 512
static BOOL ut_session_available;
static unsigned ut_session_begin_calls, ut_session_end_calls;
static unsigned ut_divide_calls, ut_multiply_calls;
static unsigned ut_timer_start_calls, ut_timer_wait_calls;
static unsigned ut_answer_calls;
static tODVScreenInfo ut_info;
static unsigned ut_info_calls;
static unsigned ut_copy_calls, ut_cursor_calls, ut_fill_calls;
static INT ut_cursor_columns[UT_EMU_MAX_CALLS];
static INT ut_cursor_rows[UT_EMU_MAX_CALLS];
static unsigned ut_char_calls;
static BYTE ut_chars[UT_EMU_MAX_CALLS];
static unsigned ut_string_calls, ut_attribute_calls;
static unsigned ut_clear_calls, ut_clear_eol_calls, ut_scrolling_calls;
static unsigned ut_disp_calls, ut_printf_calls, ut_exit_calls;
static unsigned ut_remote_calls;
static BYTE ut_remote[UT_EMU_MAX_CALLS];
static const char ut_ra_user_parameters[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:;";
static const char ut_ra_system_parameters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ012";
static const char ut_ansi_commands[] = "su@PLMrEFl";
int utm_strcmp(const char *left, const char *right);

BOOL utm_ODSessionScreenAvailable(void) { return(ut_session_available); }
void utm_ODSessionScreenBeginEmulation(void) { ++ut_session_begin_calls; }
void utm_ODSessionScreenEndEmulation(void) { ++ut_session_end_calls; }
BOOL utm_ODDWordDivide(DWORD *quotient, DWORD *remainder,
   DWORD dividend, DWORD divisor)
{
   UT_ASSERT(quotient != NULL);
   UT_ASSERT(divisor != 0);
   *quotient = dividend / divisor;
   if(remainder != NULL) *remainder = dividend % divisor;
   ++ut_divide_calls;
   return(TRUE);
}
DWORD utm_ODDWordMultiply(DWORD left, DWORD right)
{
   ++ut_multiply_calls;
   return(left * right);
}
void utm_ODTimerStart(tODTimer *timer, tODMilliSec duration)
{
   UT_ASSERT(timer != NULL);
   UT_ASSERT_EQ_UINT(MODEM_SIMULATOR_TICK, duration);
   ++ut_timer_start_calls;
}
void utm_ODTimerWaitForElapse(tODTimer *timer)
{
   UT_ASSERT(timer != NULL);
   ++ut_timer_wait_calls;
}
char ODCALL utm_od_get_answer(const char *answers)
{
   UT_ASSERT(utm_strcmp("\n\r", answers) == 0);
   ++ut_answer_calls;
   return('\r');
}
static void utm_ODEmulateGetTextInfo(tODVScreenInfo *info)
{
   UT_ASSERT(info != NULL);
   *info = ut_info;
   ++ut_info_calls;
}
static void utm_ODEmulateCopyText(INT left, INT top, INT right, INT bottom,
   INT destination_column, INT destination_row)
{
   (void)left; (void)top; (void)right; (void)bottom;
   (void)destination_column; (void)destination_row;
   ++ut_copy_calls;
}
static void utm_ODEmulateSetCursorPos(INT column, INT row)
{
   UT_ASSERT(ut_cursor_calls < UT_EMU_MAX_CALLS);
   ut_cursor_columns[ut_cursor_calls] = column;
   ut_cursor_rows[ut_cursor_calls] = row;
   ++ut_cursor_calls;
}
static void utm_ODEmulateFillArea(BYTE left, BYTE top, BYTE right,
   BYTE bottom, char value)
{
   (void)left; (void)top; (void)right; (void)bottom; (void)value;
   ++ut_fill_calls;
}
void ODCALL utm_ODScrnDisplayChar(unsigned char value)
{
   UT_ASSERT(ut_char_calls < UT_EMU_MAX_CALLS);
   ut_chars[ut_char_calls++] = value;
}
void ODCALL utm_ODScrnDisplayString(const char *text)
{
   UT_ASSERT(text != NULL);
   ++ut_string_calls;
}
void ODCALL utm_ODScrnSetAttribute(BYTE attribute)
{
   od_control.od_cur_attrib = attribute;
   ++ut_attribute_calls;
}
void utm_ODScrnClear(void) { ++ut_clear_calls; }
void utm_ODScrnClearToEndOfLine(void) { ++ut_clear_eol_calls; }
void utm_ODScrnEnableScrolling(BOOL enable)
{
   (void)enable;
   ++ut_scrolling_calls;
}
void ODCALL utm_od_disp_str(const char *text)
{
   UT_ASSERT(text != NULL);
   ++ut_disp_calls;
}
void ODVCALL utm_od_printf(const char *format, ...)
{
   va_list arguments;
   UT_ASSERT(format != NULL);
   va_start(arguments, format);
   va_end(arguments);
   ++ut_printf_calls;
}
void ODCALL utm_od_exit(INT error_level, BOOL hangup)
{
   UT_ASSERT_EQ_INT(2, error_level);
   UT_ASSERT(hangup);
   ++ut_exit_calls;
}
int utm_strcmp(const char *left, const char *right)
{
   while(*left != '\0' && *left == *right) { ++left; ++right; }
   return((unsigned char)*left - (unsigned char)*right);
}
int utm_atoi(const char *text)
{
   int value = 0;
   while(*text >= '0' && *text <= '9') value = value * 10 + (*text++ - '0');
   return(value);
}
int utm_abs(int value) { return(value < 0 ? -value : value); }
tODResult utm_ODCoreSendRemoteByte(BYTE value)
{
   UT_ASSERT(ut_remote_calls < UT_EMU_MAX_CALLS);
   ut_remote[ut_remote_calls++] = value;
   return(kODRCSuccess);
}

static void reset_emulator(void)
{
   ut_session_available = FALSE;
   ut_session_begin_calls = ut_session_end_calls = 0;
   ut_divide_calls = ut_multiply_calls = 0;
   ut_timer_start_calls = ut_timer_wait_calls = ut_answer_calls = 0;
   ut_info_calls = ut_copy_calls = ut_cursor_calls = ut_fill_calls = 0;
   ut_char_calls = ut_string_calls = ut_attribute_calls = 0;
   ut_clear_calls = ut_clear_eol_calls = ut_scrolling_calls = 0;
   ut_disp_calls = ut_printf_calls = ut_exit_calls = ut_remote_calls = 0;
   ut_info.winleft = 1; ut_info.wintop = 1;
   ut_info.winright = 80; ut_info.winbottom = 25;
   ut_info.attribute = 7; ut_info.curx = 10; ut_info.cury = 10;
   ut_info.scrolling = TRUE;
   btANSISeqLevel = 0;
   btAvatarSeqLevel = 0;
   btCurrentParamLength = 0;
   btNumParams = 0;
   btDefaultAttrib = 7;
   bAvatarInsertMode = FALSE;
   btRepeatCount = 0;
   btScrollLines = 0;
   od_control.od_emu_simulate_modem = FALSE;
   od_control.od_no_ra_codes = FALSE;
   od_control.baud = 0;
   od_control.od_connect_speed = 9600L;
   od_control.od_cur_attrib = 7;
   od_control.user_ansi = FALSE;
   od_control.user_attribute = 0;
   od_control.user_attrib2 = 0;
}

static void covers_session_screen_and_remote_echo_policy(void)
{
   reset_emulator();
   utt_ODEmulateFromBuffer("", FALSE, FALSE);
   UT_ASSERT_EQ_UINT(0, ut_session_begin_calls);

   reset_emulator();
   utt_ODEmulateFromBuffer("x", FALSE, TRUE);
   UT_ASSERT_EQ_UINT(0, ut_session_begin_calls);
   UT_ASSERT_EQ_UINT(1, ut_char_calls);
   UT_ASSERT_EQ_UINT(0, ut_remote_calls);

   reset_emulator();
   ut_session_available = TRUE;
   utt_ODEmulateFromBuffer("x", FALSE, TRUE);
   UT_ASSERT_EQ_UINT(1, ut_session_begin_calls);
   UT_ASSERT_EQ_UINT(1, ut_session_end_calls);

   reset_emulator();
   od_control.baud = 9600L;
   utt_ODEmulateFromBuffer("x", TRUE, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_remote_calls);

   reset_emulator();
   od_control.baud = 9600L;
   utt_ODEmulateFromBuffer("x", FALSE, FALSE);
   UT_ASSERT_EQ_UINT(0, ut_remote_calls);

   reset_emulator();
   utt_ODEmulateFromBuffer("x", TRUE, FALSE);
   UT_ASSERT_EQ_UINT(0, ut_remote_calls);
}

static void simulates_modem_rate_for_local_or_connected_sessions(void)
{
   reset_emulator();
   od_control.od_emu_simulate_modem = TRUE;
   od_control.baud = 0;
   utt_ODEmulateFromBuffer("xy", FALSE, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_multiply_calls);
   UT_ASSERT_EQ_UINT(1, ut_divide_calls);
   UT_ASSERT_EQ_UINT(1, ut_timer_start_calls);
   UT_ASSERT_EQ_UINT(0, ut_timer_wait_calls);

   reset_emulator();
   od_control.od_emu_simulate_modem = TRUE;
   od_control.baud = 9600L;
   od_control.od_connect_speed = 1L;
   utt_ODEmulateFromBuffer("xy", FALSE, FALSE);
   UT_ASSERT_EQ_UINT(2, ut_divide_calls);
   UT_ASSERT(ut_timer_wait_calls >= 1);
   UT_ASSERT(ut_timer_start_calls >= 2);
}

static void handles_basic_avatar_controls_and_insert_mode(void)
{
   char controls[7];
   reset_emulator();
   controls[0] = 1; controls[1] = 6; controls[2] = 11;
   controls[3] = 12; controls[4] = 25; controls[5] = 0; controls[6] = 0;
   utt_ODEmulateFromBuffer(controls, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_answer_calls);
   UT_ASSERT_EQ_UINT(1, ut_clear_calls);
   UT_ASSERT_EQ_UINT(1, btAvatarSeqLevel);

   reset_emulator();
   controls[0] = 11; controls[1] = 0;
   utt_ODEmulateFromBuffer(controls, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(22, btAvatarSeqLevel);

   reset_emulator();
   controls[0] = 22; controls[1] = 0;
   utt_ODEmulateFromBuffer(controls, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(3, btAvatarSeqLevel);

   reset_emulator();
   od_control.od_no_ra_codes = TRUE;
   controls[0] = 1; controls[1] = 0;
   utt_ODEmulateFromBuffer(controls, FALSE, FALSE);
   reset_emulator(); od_control.od_no_ra_codes = TRUE;
   controls[0] = 6; controls[1] = 0;
   utt_ODEmulateFromBuffer(controls, FALSE, FALSE);
   reset_emulator(); od_control.od_no_ra_codes = TRUE;
   controls[0] = 11; controls[1] = 0;
   utt_ODEmulateFromBuffer(controls, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_char_calls);

   reset_emulator();
   bAvatarInsertMode = TRUE;
   ut_info.curx = 10; ut_info.winright = 80;
   utt_ODEmulateFromBuffer("x", FALSE, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_copy_calls);

   reset_emulator();
   bAvatarInsertMode = TRUE;
   ut_info.curx = ut_info.winright;
   utt_ODEmulateFromBuffer("x", FALSE, FALSE);
   UT_ASSERT_EQ_UINT(0, ut_copy_calls);
}

static void covers_avatar_repeat_and_primary_command_dispatch(void)
{
   char sequence[4];
   int command;
   reset_emulator();
   sequence[0] = 25; sequence[1] = 'x'; sequence[2] = 2; sequence[3] = 0;
   utt_ODEmulateFromBuffer(sequence, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(2, ut_char_calls);

   for(command = 1; command <= 14; ++command) {
      reset_emulator();
      btAvatarSeqLevel = 3;
      sequence[0] = (char)command; sequence[1] = 0;
      utt_ODEmulateFromBuffer(sequence, FALSE, FALSE);
      if(command == 7) UT_ASSERT_EQ_UINT(1, ut_clear_eol_calls);
   }
   reset_emulator(); btAvatarSeqLevel = 3; sequence[0] = 25; sequence[1] = 0;
   utt_ODEmulateFromBuffer(sequence, FALSE, FALSE);
   reset_emulator(); btAvatarSeqLevel = 3; sequence[0] = 127; sequence[1] = 0;
   utt_ODEmulateFromBuffer(sequence, FALSE, FALSE);
}

static void covers_avatar_boundaries_and_extended_states(void)
{
   char value[2];
   int command;
   value[1] = 0;
   reset_emulator(); btAvatarSeqLevel = 13; value[0] = 'x';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   for(command = 3; command <= 6; ++command) {
      reset_emulator();
      btAvatarSeqLevel = 3;
      if(command == 3) ut_info.cury = 1;
      if(command == 4) ut_info.cury = ut_info.winbottom;
      if(command == 5) ut_info.curx = 1;
      if(command == 6) ut_info.curx = ut_info.winright;
      value[0] = (char)command;
      utt_ODEmulateFromBuffer(value, FALSE, FALSE);
      UT_ASSERT_EQ_UINT(0, ut_cursor_calls);
   }
   reset_emulator();
   btAvatarSeqLevel = 3; ut_info.curx = ut_info.winright;
   value[0] = 14;
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(0, ut_copy_calls);

   reset_emulator(); btAvatarSeqLevel = 1; value[0] = 27;
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(2, btAvatarSeqLevel);

   reset_emulator(); btAvatarSeqLevel = 4; value[0] = 3;
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_INT(3, od_control.od_cur_attrib);

   reset_emulator(); btAvatarSeqLevel = 5; value[0] = 8;
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(6, btAvatarSeqLevel);
   value[0] = 9;
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_cursor_calls);

   reset_emulator(); btAvatarSeqLevel = 7; btScrollLines = -1; value[0] = 2;
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_INT(2, btScrollLines);
   reset_emulator(); btAvatarSeqLevel = 7; btScrollLines = 1; value[0] = 2;
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_INT(-2, btScrollLines);

   for(command = 8; command <= 11; ++command) {
      reset_emulator(); btAvatarSeqLevel = (BYTE)command; value[0] = 2;
      utt_ODEmulateFromBuffer(value, FALSE, FALSE);
      UT_ASSERT_EQ_UINT((unsigned)(command + 1), btAvatarSeqLevel);
   }

   reset_emulator(); btAvatarSeqLevel = 12; btScrollLines = 0;
   btScrollTop = 1; btScrollBottom = 5; value[0] = 'x';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_fill_calls);
   reset_emulator(); btAvatarSeqLevel = 12; btScrollLines = 7;
   btScrollTop = 1; btScrollBottom = 5; value[0] = 'x';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_fill_calls);
   reset_emulator(); btAvatarSeqLevel = 12; btScrollLines = -2;
   btScrollTop = 1; btScrollBottom = 5; value[0] = 'x';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_copy_calls);
   UT_ASSERT_EQ_UINT(1, ut_fill_calls);
   reset_emulator(); btAvatarSeqLevel = 12; btScrollLines = 2;
   btScrollTop = 1; btScrollBottom = 5; value[0] = 'x';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_copy_calls);

   reset_emulator(); btAvatarSeqLevel = 14; value[0] = 3;
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(17, btAvatarSeqLevel);
   reset_emulator(); btAvatarSeqLevel = 15; value[0] = 3;
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(16, btAvatarSeqLevel);
   value[0] = 'x'; utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(17, btAvatarSeqLevel);
   value[0] = 2; utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(18, btAvatarSeqLevel);
   value[0] = 4; utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_fill_calls);

   reset_emulator(); btAvatarSeqLevel = 19; value[0] = 2;
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(20, btAvatarSeqLevel);
   value[0] = 'a'; utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   value[0] = 'b'; utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   value[0] = 2; utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(2, ut_string_calls);
   reset_emulator(); btAvatarSeqLevel = 20; btScrollLines = 0; value[0] = 1;
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
}

static void covers_remote_access_user_parameter_codes(void)
{
   char value[2];
   unsigned index;
   value[1] = 0;
   for(index = 0; ut_ra_user_parameters[index] != '\0'; ++index) {
      reset_emulator();
      btAvatarSeqLevel = 21;
      value[0] = ut_ra_user_parameters[index];
      utt_ODEmulateFromBuffer(value, FALSE, FALSE);
      UT_ASSERT_EQ_UINT(0, btAvatarSeqLevel);
   }
   reset_emulator(); btAvatarSeqLevel = 21; value[0] = '?';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);

   reset_emulator(); od_control.user_flags[0] = 0x55;
   btAvatarSeqLevel = 21; value[0] = 'H';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);

   reset_emulator();
   od_control.user_name[0] = 'F'; od_control.user_name[1] = ' '; 
   od_control.user_name[2] = 'L'; od_control.user_name[3] = '\0';
   btAvatarSeqLevel = 21; value[0] = 'W';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   reset_emulator();
   od_control.user_name[0] = 'F'; od_control.user_name[1] = '\0';
   btAvatarSeqLevel = 21; value[0] = 'W';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);

   reset_emulator(); od_control.user_ansi = TRUE;
   btAvatarSeqLevel = 21; value[0] = 'X';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   reset_emulator(); od_control.user_attribute = 0x04;
   btAvatarSeqLevel = 21; value[0] = 'Y';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   reset_emulator(); od_control.user_attribute = 0x02;
   btAvatarSeqLevel = 21; value[0] = 'Z';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   reset_emulator(); od_control.user_attribute = 0x40;
   btAvatarSeqLevel = 21; value[0] = '0';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   reset_emulator(); od_control.user_attribute = 0x80;
   btAvatarSeqLevel = 21; value[0] = '1';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   reset_emulator(); od_control.user_attrib2 = 0x01;
   btAvatarSeqLevel = 21; value[0] = '2';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   reset_emulator(); od_control.user_attrib2 = 0x02;
   btAvatarSeqLevel = 21; value[0] = '8';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   reset_emulator(); od_control.user_attrib2 = 0x04;
   btAvatarSeqLevel = 21; value[0] = ';';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
}

static void covers_remote_access_system_parameter_codes(void)
{
   char value[2];
   unsigned index;
   value[1] = 0;
   for(index = 0; ut_ra_system_parameters[index] != '\0'; ++index) {
      reset_emulator();
      btAvatarSeqLevel = 22;
      value[0] = ut_ra_system_parameters[index];
      utt_ODEmulateFromBuffer(value, FALSE, FALSE);
      UT_ASSERT_EQ_UINT(0, btAvatarSeqLevel);
      if(value[0] == 'X') UT_ASSERT_EQ_UINT(1, ut_exit_calls);
   }
   reset_emulator(); btAvatarSeqLevel = 22; value[0] = '?';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
}

static void covers_ansi_sequence_recognition_and_parameter_parsing(void)
{
   char value[2];
   value[1] = 0;
   reset_emulator();
   utt_ODEmulateFromBuffer("\033", FALSE, FALSE);
   UT_ASSERT_EQ_UINT(1, btANSISeqLevel);

   reset_emulator();
   utt_ODEmulateFromBuffer("\033x", FALSE, FALSE);
   UT_ASSERT_EQ_UINT(2, ut_char_calls);

   reset_emulator();
   utt_ODEmulateFromBuffer("\033[", FALSE, FALSE);
   UT_ASSERT_EQ_UINT(2, btANSISeqLevel);

   reset_emulator();
   utt_ODEmulateFromBuffer("\033[123456", FALSE, FALSE);
   UT_ASSERT_EQ_UINT(0, btANSISeqLevel);

   reset_emulator();
   utt_ODEmulateFromBuffer("\033[1;A", FALSE, FALSE);
   reset_emulator();
   utt_ODEmulateFromBuffer("\033[?9;A", FALSE, FALSE);
   reset_emulator();
   utt_ODEmulateFromBuffer("\033[;A", FALSE, FALSE);

   reset_emulator(); btANSISeqLevel = 2; btNumParams = 10;
   value[0] = ';'; utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   UT_ASSERT_EQ_UINT(0, btANSISeqLevel);

   reset_emulator();
   utt_ODEmulateFromBuffer("\033[1A", FALSE, FALSE);
   reset_emulator();
   utt_ODEmulateFromBuffer("\033[?9A", FALSE, FALSE);

   reset_emulator(); btANSISeqLevel = 2; btNumParams = 10;
   btCurrentParamLength = 1; szCurrentParam[0] = '1'; szCurrentParam[1] = 0;
   value[0] = 'x'; utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   reset_emulator(); btANSISeqLevel = 2; btNumParams = 0;
   btCurrentParamLength = 0; value[0] = 'x';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);

   reset_emulator(); btANSISeqLevel = 2; value[0] = '/';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   reset_emulator(); btANSISeqLevel = 2; value[0] = ':';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
   reset_emulator(); btANSISeqLevel = 2; value[0] = '?';
   utt_ODEmulateFromBuffer(value, FALSE, FALSE);
}

static void covers_ansi_cursor_movement_and_positioning(void)
{
   reset_emulator(); utt_ODEmulateFromBuffer("\033[A", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[1A", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[99A", FALSE, FALSE);
   reset_emulator(); ut_info.cury = ut_info.winbottom;
   utt_ODEmulateFromBuffer("\033[?9A", FALSE, FALSE);

   reset_emulator(); utt_ODEmulateFromBuffer("\033[B", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[99B", FALSE, FALSE);
   reset_emulator(); ut_info.cury = 1;
   utt_ODEmulateFromBuffer("\033[?9B", FALSE, FALSE);

   reset_emulator(); utt_ODEmulateFromBuffer("\033[C", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[99C", FALSE, FALSE);
   reset_emulator(); ut_info.curx = 1;
   utt_ODEmulateFromBuffer("\033[?9C", FALSE, FALSE);

   reset_emulator(); utt_ODEmulateFromBuffer("\033[D", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[99D", FALSE, FALSE);
   reset_emulator(); ut_info.curx = ut_info.winright;
   utt_ODEmulateFromBuffer("\033[?9D", FALSE, FALSE);

   reset_emulator(); utt_ODEmulateFromBuffer("\033[H", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[3H", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[0H", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[;5H", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[2;5H", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[f", FALSE, FALSE);
}

static void covers_ansi_erasure_attributes_and_miscellaneous_commands(void)
{
   unsigned index;
   reset_emulator(); utt_ODEmulateFromBuffer("\033[J", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[0J", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[1J", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[2J", FALSE, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_clear_calls);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[3J", FALSE, FALSE);

   reset_emulator(); utt_ODEmulateFromBuffer("\033[K", FALSE, FALSE);
   UT_ASSERT_EQ_UINT(1, ut_clear_eol_calls);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[0K", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[1K", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[2K", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[3K", FALSE, FALSE);

   reset_emulator();
   utt_ODEmulateFromBuffer("\033[0;1;2;4;5;7;8;30;37;29m", FALSE, FALSE);
   UT_ASSERT(ut_attribute_calls >= 9);
   reset_emulator();
   utt_ODEmulateFromBuffer("\033[40;47;39;48m", FALSE, FALSE);
   UT_ASSERT_EQ_UINT(2, ut_attribute_calls);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[m", FALSE, FALSE);

   for(index = 0; ut_ansi_commands[index] != '\0'; ++index) {
      char sequence[4];
      reset_emulator();
      sequence[0] = 27; sequence[1] = '[';
      sequence[2] = ut_ansi_commands[index]; sequence[3] = 0;
      utt_ODEmulateFromBuffer(sequence, FALSE, FALSE);
   }

   reset_emulator(); utt_ODEmulateFromBuffer("\033[4h", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[?9h", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[3h", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[h", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[4l", FALSE, FALSE);
   reset_emulator(); utt_ODEmulateFromBuffer("\033[3l", FALSE, FALSE);
}

static const UTTestCase ut_cases[] = {
   {"echo and session", covers_session_screen_and_remote_echo_policy},
   {"modem simulation", simulates_modem_rate_for_local_or_connected_sessions},
   {"basic AVATAR", handles_basic_avatar_controls_and_insert_mode},
   {"AVATAR commands", covers_avatar_repeat_and_primary_command_dispatch},
   {"AVATAR states", covers_avatar_boundaries_and_extended_states},
   {"RA user codes", covers_remote_access_user_parameter_codes},
   {"RA system codes", covers_remote_access_system_parameter_codes},
   {"ANSI parser", covers_ansi_sequence_recognition_and_parameter_parsing},
   {"ANSI cursor", covers_ansi_cursor_movement_and_positioning},
   {"ANSI commands", covers_ansi_erasure_attributes_and_miscellaneous_commands}
};
