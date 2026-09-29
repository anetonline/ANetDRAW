#include "test_support.h"

static int init_exit_calls;
static int before_exit_calls;

static void BeforeExit(void)
{
   ++before_exit_calls;
}

static void ExitDuringInitialization(void)
{
   ++init_exit_calls;
   od_control.od_force_local = TRUE;
   od_exit(31, FALSE);
   od_exit(99, TRUE);
}

int main(void)
{
   tODInputEvent event;

   memset(&od_control, 0, sizeof(od_control));
   od_control.od_disable = DIS_NAME_PROMPT | DIS_CARRIERDETECT |
      DIS_TIMEOUT | DIS_LOCAL_INPUT;
   od_control.od_silent_mode = TRUE;
   od_control.od_nocopyright = TRUE;
   od_control.od_noexit = TRUE;
   od_control.od_no_file_func = ExitDuringInitialization;
   od_control.od_before_exit = BeforeExit;

   OD_TEST_CHECK(!od_get_input(&event, 0, GETIN_RAW));
   OD_TEST_CHECK(init_exit_calls == 1);
   OD_TEST_CHECK(before_exit_calls == 1);
   OD_TEST_CHECK(od_control_get() == NULL);
   OD_TEST_CHECK(od_control.od_error == ERR_GENERALFAILURE);
   return(0);
}
