#include "test_support.h"

#define LOG_FILE "ODPUBLIC.LOG"
#define MISSING_LOG_FILE "OD-MISSING-DIR/ODPUBLIC.LOG"

int main(void)
{
   FILE *file;
   char contents[1024];
   size_t length;

   remove(LOG_FILE);
   ODTestConfigureLocal();
   od_init();

   od_control.od_logfile_disable = TRUE;
   strcpy(od_control.od_logfile_name, LOG_FILE);
   OD_TEST_CHECK(od_log_open());
   OD_TEST_CHECK(od_log_write("disabled public log record"));
   file = fopen(LOG_FILE, "r");
   OD_TEST_CHECK(file == NULL);

   od_control.od_logfile_disable = FALSE;
   strcpy(od_control.od_logfile_name, MISSING_LOG_FILE);
   OD_TEST_CHECK(!od_log_open());
   OD_TEST_CHECK(!od_log_write("unwritable public log record"));

   strcpy(od_control.od_logfile_name, LOG_FILE);
   OD_TEST_CHECK(od_log_write("lazy public log record"));
   OD_TEST_CHECK(od_log_open());
   OD_TEST_CHECK(od_log_write("second public log record"));
   od_exit(23, FALSE);

   file = fopen(LOG_FILE, "r");
   OD_TEST_CHECK(file != NULL);
   length = fread(contents, 1, sizeof(contents) - 1, file);
   OD_TEST_CHECK(!ferror(file));
   contents[length] = '\0';
   OD_TEST_CHECK(fclose(file) == 0);
   OD_TEST_CHECK(strstr(contents, "lazy public log record") != NULL);
   OD_TEST_CHECK(strstr(contents, "second public log record") != NULL);
   remove(LOG_FILE);
   return(0);
}
