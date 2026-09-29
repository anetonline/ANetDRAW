#include "test_support.h"

int main(void)
{
   const char *registry = "ODRESACC.SYN";
   DWORD seconds;
   WORD milliseconds;

   remove(registry);
   ODTestConfigureLocal();

   OD_TEST_CHECK(!od_reserve_configure(NULL));
   OD_TEST_CHECK(od_control.od_error == ERR_PARAMETER);
   OD_TEST_CHECK(od_reserve_configure(registry));
   od_init();

   OD_TEST_CHECK(!od_reserve_request(""));
   OD_TEST_CHECK(od_control.od_error == ERR_PARAMETER);
   OD_TEST_CHECK(od_reserve_request("AcceptanceOne"));
   OD_TEST_CHECK(!od_reserve_request("AcceptanceTwo"));
   OD_TEST_CHECK(od_reserve_wait(0) == OD_RESERVE_ACQUIRED);
   OD_TEST_CHECK(od_reserve_end());
   OD_TEST_CHECK(!od_reserve_end());

   OD_TEST_CHECK(od_reserve_request("AcceptanceTwo"));
   od_get_time(&seconds, &milliseconds);
   ++seconds;
   OD_TEST_CHECK(od_reserve_wait_until(seconds, milliseconds)
      == OD_RESERVE_ACQUIRED);
   OD_TEST_CHECK(od_reserve_end());

   od_exit(0, FALSE);
   OD_TEST_CHECK(remove(registry) == 0);
   return(0);
}
