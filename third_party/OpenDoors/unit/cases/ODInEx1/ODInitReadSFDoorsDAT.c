#define UT_CUSTOM_MOCK_ODStringToName
#define UT_CUSTOM_MOCK_atoi
#define UT_CUSTOM_MOCK_atol
#define UT_CUSTOM_MOCK_fclose
#define UT_CUSTOM_MOCK_fgets
#define UT_CUSTOM_MOCK_fopen
#define UT_CUSTOM_MOCK_sprintf
#define UT_CUSTOM_MOCK_strcpy
#define UT_CUSTOM_MOCK_strupr
#define UT_CUSTOM_MOCK_od_strupr

#include "text_drop_file_fixture.h"

static char ut_drop_storage[25][81];

static void reset_sfdoors_fixture(void)
{
   unsigned index;
   ut_text_reset();
   memset(&od_control, 0, sizeof(od_control));
   memset(ut_drop_storage, 0, sizeof(ut_drop_storage));
   for(index = 0; index < 25; ++index)
      apszDropFileInfo[index] = ut_drop_storage[index];
   utm_strcpy(szDropFilePath, "node/sfdoors.dat");
   ut_text_expected_path = szDropFilePath;
   ut_text_expected_mode = "r";

   ut_text_set_line(0, "42\n");
   ut_text_set_line(1, "Sample User\n");
   ut_text_set_line(2, "secret\n");
   ut_text_set_line(3, "unused-4\n");
   ut_text_set_line(4, "57600\n");
   ut_text_set_line(5, "3\n");
   ut_text_set_line(6, "45\n");
   ut_text_set_line(7, "unused-8\n");
   ut_text_set_line(8, "unused-9\n");
   ut_text_set_line(9, "true\n");
   ut_text_set_line(10, "80\n");
   ut_text_set_line(11, "11\n");
   ut_text_set_line(12, "22\n");
   ut_text_set_line(13, "unused-14\n");
   ut_text_set_line(14, "125\n");
   ut_text_set_line(15, "unused-16\n");
   ut_text_set_line(16, "true\n");
   ut_text_set_line(17, "unused-18\n");
   ut_text_set_line(18, "unused-19\n");
   ut_text_set_line(19, "unused-20\n");
   ut_text_set_line(20, "true\n");
   ut_text_set_line(21, "7\n");
   ut_text_set_line(22, "8\n");
   ut_text_set_line(23, "9\n");
   ut_text_set_line(24, "unused-25\n");
   ut_text_set_line(25, "unused-26\n");
   ut_text_set_line(26, "unused-27\n");
   ut_text_set_line(27, "123\n");
   ut_text_set_line(28, "456\n");
   ut_text_set_line(29, "789\n");
   ut_text_set_line(30, "555-0100\n");
   ut_text_set_line(31, "Somewhere\n");
   ut_text_set_line(32, "unused-33\n");
   ut_text_set_line(33, "true\n");
   ut_text_set_line(34, "true\n");
   ut_text_set_line(35, "unused-36\n");
   ut_text_set_line(36, "5\n");
   ut_text_set_line(37, "1016\n");
   ut_text_set_line(38, "unused-39\n");
}

static void rejects_a_missing_file(void)
{
   reset_sfdoors_fixture();
   ut_text_open_fails = TRUE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODInitReadSFDoorsDAT());
   UT_ASSERT_EQ_UINT(1, ut_text_open_count);
   UT_ASSERT_EQ_UINT(0, ut_text_read_count);
   UT_ASSERT_EQ_UINT(0, ut_text_close_count);
}

static void rejects_each_truncated_mandatory_record(void)
{
   unsigned available;
   for(available = 0; available < 32; ++available)
   {
      reset_sfdoors_fixture();
      ut_text_line_count = available;
      UT_ASSERT_EQ_INT(FALSE, utt_ODInitReadSFDoorsDAT());
      UT_ASSERT_EQ_UINT(available + 1, ut_text_read_count);
      UT_ASSERT_EQ_UINT(1, ut_text_close_count);
   }
}

static void imports_a_complete_record(void)
{
   reset_sfdoors_fixture();
   UT_ASSERT_EQ_INT(TRUE, utt_ODInitReadSFDoorsDAT());
   UT_ASSERT_EQ_UINT(39, ut_text_read_count);
   UT_ASSERT_EQ_UINT(1, ut_text_close_count);
   UT_ASSERT_EQ_INT(42, od_control.user_num);
   UT_ASSERT(strcmp("Sample User", od_control.user_name) == 0);
   UT_ASSERT(strcmp("secret", od_control.user_password) == 0);
   UT_ASSERT_EQ_INT(57600, od_control.baud);
   UT_ASSERT_EQ_INT(2, od_control.port);
   UT_ASSERT_EQ_INT(45, od_control.user_timelimit);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_ansi);
   UT_ASSERT_EQ_INT(80, od_control.user_security);
   UT_ASSERT_EQ_INT(11, od_control.user_uploads);
   UT_ASSERT_EQ_INT(22, od_control.user_downloads);
   UT_ASSERT(strcmp("02:05", od_control.user_logintime) == 0);
   UT_ASSERT_EQ_INT(TRUE, od_control.sysop_next);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_error_free);
   UT_ASSERT_EQ_INT(7, od_control.user_msg_area);
   UT_ASSERT_EQ_INT(8, od_control.user_file_area);
   UT_ASSERT_EQ_INT(9, od_control.od_node);
   UT_ASSERT_EQ_INT(123, od_control.user_todayk);
   UT_ASSERT_EQ_INT(456, od_control.user_upk);
   UT_ASSERT_EQ_INT(789, od_control.user_downk);
   UT_ASSERT(strcmp("555-0100", od_control.user_homephone) == 0);
   UT_ASSERT(strcmp("Somewhere", od_control.user_location) == 0);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_rip);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_wantchat);
   UT_ASSERT_EQ_INT(5, od_control.od_com_irq);
   UT_ASSERT_EQ_INT(1016, od_control.od_com_address);
   UT_ASSERT(strcmp("unused-4\n", apszDropFileInfo[0]) == 0);
   UT_ASSERT(strcmp("unused-39\n", apszDropFileInfo[18]) == 0);
}

static void accepts_a_record_without_optional_extension_lines(void)
{
   reset_sfdoors_fixture();
   ut_text_line_count = 32;
   od_control.user_rip = TRUE;
   od_control.user_wantchat = TRUE;
   od_control.od_com_irq = 7;
   od_control.od_com_address = 888;
   UT_ASSERT_EQ_INT(TRUE, utt_ODInitReadSFDoorsDAT());
   UT_ASSERT_EQ_UINT(39, ut_text_read_count);
   UT_ASSERT_EQ_UINT(1, ut_text_close_count);
   UT_ASSERT_EQ_INT(0, apszDropFileInfo[15][0]);
   UT_ASSERT_EQ_INT(0, apszDropFileInfo[17][0]);
   UT_ASSERT_EQ_INT(0, apszDropFileInfo[18][0]);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_rip);
   UT_ASSERT_EQ_INT(TRUE, od_control.user_wantchat);
   UT_ASSERT_EQ_INT(7, od_control.od_com_irq);
   UT_ASSERT_EQ_INT(888, od_control.od_com_address);
}

static void clamps_login_minutes_to_the_supported_range(void)
{
   reset_sfdoors_fixture();
   ut_text_set_line(14, "-1\n");
   UT_ASSERT_EQ_INT(TRUE, utt_ODInitReadSFDoorsDAT());
   UT_ASSERT(strcmp("00:00", od_control.user_logintime) == 0);

   reset_sfdoors_fixture();
   ut_text_set_line(14, "6000\n");
   UT_ASSERT_EQ_INT(TRUE, utt_ODInitReadSFDoorsDAT());
   UT_ASSERT(strcmp("99:59", od_control.user_logintime) == 0);
}

static void imports_false_boolean_flags(void)
{
   reset_sfdoors_fixture();
   ut_text_set_line(9, "false\n");
   ut_text_set_line(16, "false\n");
   ut_text_set_line(20, "false\n");
   ut_text_set_line(33, "false\n");
   ut_text_set_line(34, "false\n");
   UT_ASSERT_EQ_INT(TRUE, utt_ODInitReadSFDoorsDAT());
   UT_ASSERT_EQ_INT(FALSE, od_control.user_ansi);
   UT_ASSERT_EQ_INT(FALSE, od_control.sysop_next);
   UT_ASSERT_EQ_INT(FALSE, od_control.user_error_free);
   UT_ASSERT_EQ_INT(FALSE, od_control.user_rip);
   UT_ASSERT_EQ_INT(FALSE, od_control.user_wantchat);
}

static const UTTestCase ut_cases[] = {
   {"missing file", rejects_a_missing_file},
   {"truncated mandatory record", rejects_each_truncated_mandatory_record},
   {"complete record", imports_a_complete_record},
   {"base record", accepts_a_record_without_optional_extension_lines},
   {"login-minute bounds", clamps_login_minutes_to_the_supported_range},
   {"false flags", imports_false_boolean_flags}
};
