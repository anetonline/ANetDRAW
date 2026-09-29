#include "test_support.h"

static int before_exit_calls;
static int kernel_exit_calls;
static int sleep_kernel_calls;

static void BeforeExit(void)
{
   ++before_exit_calls;
   od_control.od_noexit = TRUE;
}

static void KernelExit(void)
{
   ++kernel_exit_calls;
   od_control.od_ker_exec = NULL;
   od_exit(23, FALSE);
}

static void CountSleepKernel(void)
{
   ++sleep_kernel_calls;
}

int main(void)
{
   tODInputEvent event;

   ODTestConfigureLocal();
   strcpy(od_control.od_prog_name, "Acceptance Door");
   strcpy(od_control.od_prog_version, "1.0");
   od_control.od_before_exit = BeforeExit;
   od_control.user_screenwidth = 100;
   od_control.user_screen_length = 30;

   OD_TEST_CHECK(od_set_port(0));
   OD_TEST_CHECK(od_control.port == 0);
   od_init();

   OD_TEST_CHECK(od_control.od_info_type == NO_DOOR_FILE);
   OD_TEST_CHECK(od_control.od_force_local == TRUE);
#ifdef ODPLAT_WIN32
   /* ODTestConfigureLocal() explicitly requests silent mode. Windows
    * preserves that request and must not attach to or allocate a console. */
   OD_TEST_CHECK(od_control.od_silent_mode == TRUE);
#else
   OD_TEST_CHECK(od_control.od_silent_mode == FALSE);
#endif
   OD_TEST_CHECK(strcmp(od_control.od_prog_name, "Acceptance Door") == 0);
   OD_TEST_CHECK(od_control.user_screenwidth == 100);
   OD_TEST_CHECK(od_control.user_screen_length == 30);
   OD_TEST_CHECK(od_control.od_page_len != 0);
   OD_TEST_CHECK(od_control.od_color_char == '\0');
   OD_TEST_CHECK(od_control.od_color_delimiter == '`');
   OD_TEST_CHECK(od_control.od_error == ERR_NONE);
   OD_TEST_CHECK(od_control_get() == &od_control);

   od_set_statusline(STATUS_NONE);
   od_set_statusline(STATUS_NORMAL);
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_WIN32)
   OD_TEST_CHECK(od_control.od_error == ERR_NONE);
#else
   OD_TEST_CHECK(od_control.od_error == ERR_UNSUPPORTED);
#endif
   od_control.od_error = ERR_NONE;
   OD_TEST_CHECK(od_color_config("Bright White on Blue") ==
      (BYTE)(L_WHITE | (D_BLUE << 4)));
   OD_TEST_CHECK(!od_carrier());
   OD_TEST_CHECK(od_control.od_error == ERR_NOREMOTE);
   od_set_dtr(TRUE);

   od_control.od_ker_exec = CountSleepKernel;
   od_sleep(1);
   OD_TEST_CHECK(sleep_kernel_calls == 0);
   od_control.od_ker_exec = NULL;

   od_kernel();

   od_control.od_noexit = FALSE;
   od_control.od_ker_exec = KernelExit;
   OD_TEST_CHECK(!od_get_input(&event, OD_NO_TIMEOUT, GETIN_RAW));
   OD_TEST_CHECK(kernel_exit_calls == 1);
   OD_TEST_CHECK(before_exit_calls == 1);
   OD_TEST_CHECK(od_control_get() == NULL);
   OD_TEST_CHECK(od_control.od_error == ERR_GENERALFAILURE);
   return(0);
}
