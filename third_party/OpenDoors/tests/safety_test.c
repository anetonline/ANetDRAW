#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "OpenDoor.h"
#include "ODCore.h"
#include "ODFormat.h"
#include "ODInEx.h"
#include "ODInQue.h"
#include "ODSafe.h"
#include "ODUtil.h"

#define CHECK(condition) do { if(!(condition)) return(__LINE__); } while(0)

BOOL ODComCP437ToUnicodeLen(const BYTE *buf, int size, size_t *length);
BYTE *ODComCP437ToUnicode(BYTE *buf, int *size);
void ODCALL ODStatAddKey(WORD wKeyCode);
void ODCALL ODStatRemoveKey(WORD wKeyCode);

static void HotKeyOne(void)
{
}

static void HotKeyTwo(void)
{
}

static void HotKeyThree(void)
{
}

static int Format(char *buffer, size_t size, const char *format, ...)
{
   va_list args;
   int result;

   va_start(args, format);
   result = ODVsnprintf(buffer, size, format, args);
   va_end(args);
   return(result);
}

#if defined(ODPLAT_WIN32) || defined(ODPLAT_NIX)
static void ParseHandleOption(char *option, char *value)
{
#ifdef ODPLAT_WIN32
   char program[] = "door";
   char *arguments[3];

   arguments[0] = program;
   arguments[1] = option;
   arguments[2] = value;
   od_parse_cmd_line_cons(3, arguments);
#else
   char program[] = "door";
   char *arguments[3];

   arguments[0] = program;
   arguments[1] = option;
   arguments[2] = value;
   od_parse_cmd_line(3, arguments);
#endif
}
#endif

int main(void)
{
   size_t result;
   size_t maximum = (size_t)-1;
   BYTE input[3];
   BYTE *converted;
   int converted_size;
   char buffer[8];
   char filename[32];
   tODInQueueHandle queue;
#ifdef ODPLAT_WIN32
   char argument_zero[] = "door";
   char argument_port[] = "-P";
   char argument_value[] = "0";
   char argument_com1[] = "COM1";
   char *port_arguments[3];
#else
   char argument_zero[] = "door";
   char argument_port[] = "-P";
   char argument_value[] = "0";
   char argument_com1[] = "COM1";
   char *port_arguments[3];
#endif
#ifdef ODPLAT_NIX
   struct passwd user_info;
#endif

   CHECK(ODSizeAdd(10, 20, &result) && result == 30);
   CHECK(!ODSizeAdd(maximum, 1, &result));
   CHECK(ODSizeMultiply(7, 9, &result) && result == 63);
   CHECK(!ODSizeMultiply(maximum, 2, &result));

   input[0] = 'A';
   input[1] = 0x80;
   input[2] = 0xdb;
   CHECK(ODComCP437ToUnicodeLen(input, 3, &result));
   CHECK(result == 6);
   converted_size = 3;
   converted = ODComCP437ToUnicode(input, &converted_size);
   CHECK(converted != NULL);
   CHECK(converted_size == 6);
   CHECK(converted[0] == 'A');
   CHECK(converted[1] == 0xc3 && converted[2] == 0x87);
   CHECK(converted[3] == 0xe2 && converted[4] == 0x96
      && converted[5] == 0x88);
   free(converted);

   CHECK(Format(buffer, sizeof(buffer), "%s-%u", "abc", 42) == 6);
   CHECK(strcmp(buffer, "abc-42") == 0);
   CHECK(Format(buffer, 4, "%s", "abcdef") == 6);
   CHECK(strcmp(buffer, "abc") == 0);
   CHECK(Format(buffer, 0, "%08lu", 1234UL) == 8);

   CHECK(ODMakeFilename(filename, "", "FILE.TXT", sizeof("FILE.TXT"))
      == kODRCSuccess);
   CHECK(strcmp(filename, "FILE.TXT") == 0);
   CHECK(ODMakeFilename(filename, "AREA" DIRSEP_STR, "FILES.BBS",
      sizeof("AREA" DIRSEP_STR "FILES.BBS")) == kODRCSuccess);
   CHECK(strcmp(filename, "AREA" DIRSEP_STR "FILES.BBS") == 0);
   CHECK(ODMakeFilename(filename, "AREA", "FILES.BBS",
      sizeof("AREA" DIRSEP_STR "FILES.BBS")) == kODRCSuccess);
   CHECK(strcmp(filename, "AREA" DIRSEP_STR "FILES.BBS") == 0);
   strcpy(filename, "AREA");
   CHECK(ODMakeFilename(filename, filename, "FILES.BBS",
      sizeof("AREA" DIRSEP_STR "FILES.BBS")) == kODRCSuccess);
   CHECK(strcmp(filename, "AREA" DIRSEP_STR "FILES.BBS") == 0);
   strcpy(filename, "UNCHANGED");
   CHECK(ODMakeFilename(filename, "AREA", "FILES.BBS",
      sizeof("AREA" DIRSEP_STR "FILES.BBS") - 1)
      == kODRCFilenameTooLong);
   CHECK(strcmp(filename, "UNCHANGED") == 0);

   CHECK(ODInQueueAlloc(&queue, 0) == kODRCInvalidCall);
   CHECK(ODInQueueAlloc(&queue, 1) == kODRCInvalidCall);
   CHECK(ODInQueueAlloc(&queue, 2) == kODRCSuccess);
   ODInQueueFree(queue);

   od_control.od_error = ERR_NONE;
   CHECK(ODCoreCarrierResult(kODRCSuccess, TRUE));
   CHECK(od_control.od_error == ERR_NONE);
   CHECK(!ODCoreCarrierResult(kODRCSuccess, FALSE));
   CHECK(od_control.od_error == ERR_NONE);
   CHECK(!ODCoreCarrierResult(kODRCGeneralFailure, TRUE));
   CHECK(od_control.od_error == ERR_GENERALFAILURE);

   od_control.od_error = ERR_NONE;
   ODCoreSetDTRResult(kODRCSuccess);
   CHECK(od_control.od_error == ERR_NONE);
   ODCoreSetDTRResult(kODRCUnsupported);
   CHECK(od_control.od_error == ERR_GENERALFAILURE);

   bODInitialized = FALSE;
   od_control.od_error = ERR_NONE;
   CHECK(od_set_port(0));
   CHECK(od_control.port == 0);
   CHECK(nForcedPort == 0);

   CHECK(od_set_port(255));
   CHECK(od_control.port == 255);
   CHECK(nForcedPort == 255);
   od_control.od_error = ERR_NONE;
   CHECK(!od_set_port(-1));
   CHECK(od_control.od_error == ERR_PARAMETER);
   CHECK(od_control.port == 255);
   od_control.od_error = ERR_NONE;
   CHECK(!od_set_port(256));
   CHECK(od_control.od_error == ERR_PARAMETER);
   CHECK(od_control.port == 255);

   bODInitialized = TRUE;
   od_control.od_error = ERR_NONE;
   CHECK(!od_set_port(1));
   CHECK(od_control.od_error == ERR_PARAMETER);
   CHECK(od_control.port == 255);
   bODInitialized = FALSE;

   wPreSetInfo = 0;
#ifdef ODPLAT_WIN32
   port_arguments[0] = argument_zero;
   port_arguments[1] = argument_port;
   port_arguments[2] = argument_value;
   od_parse_cmd_line_cons(3, port_arguments);
#else
   port_arguments[0] = argument_zero;
   port_arguments[1] = argument_port;
   port_arguments[2] = argument_value;
   od_parse_cmd_line(3, port_arguments);
#endif
   CHECK(od_control.port == 0);
   CHECK((wPreSetInfo & PRESET_PORT) != 0);
   CHECK(nForcedPort == 0);

   CHECK(od_set_port(2));
#ifdef ODPLAT_WIN32
   port_arguments[2] = argument_com1;
   od_parse_cmd_line_cons(3, port_arguments);
#else
   port_arguments[2] = argument_com1;
   od_parse_cmd_line(3, port_arguments);
#endif
   CHECK(od_control.port == 0);
   CHECK(nForcedPort == 0);

#ifdef ODPLAT_NIX
   CHECK(ODInitSelectTerminalBaud(B9600, B19200) == 9600L);
   CHECK(ODInitSelectTerminalBaud(B0, B38400) == 38400L);
   CHECK(ODInitSelectTerminalBaud(B0, B0) == 19200L);
   CHECK(ODInitSelectTerminalBaud((speed_t)-1, B9600) == 9600L);
#ifdef B115200
   CHECK(ODInitSelectTerminalBaud(B115200, B9600) == 115200L);
#endif

   strcpy(od_control.user_handle, "Existing handle");
   strcpy(od_control.user_name, "Existing name");
   ODInitApplyUserInfo(NULL);
   CHECK(strcmp(od_control.user_handle, "Existing handle") == 0);
   CHECK(strcmp(od_control.user_name, "Existing name") == 0);

   memset(&user_info, 0, sizeof(user_info));
   user_info.pw_name = "login-name";
   user_info.pw_gecos = "Display Name";
   ODInitApplyUserInfo(&user_info);
   CHECK(strcmp(od_control.user_handle, "login-name") == 0);
   CHECK(strcmp(od_control.user_name, "Display Name") == 0);

   user_info.pw_name = NULL;
   user_info.pw_gecos = "Replacement Name";
   ODInitApplyUserInfo(&user_info);
   CHECK(strcmp(od_control.user_handle, "login-name") == 0);
   CHECK(strcmp(od_control.user_name, "Replacement Name") == 0);

   user_info.pw_name = "replacement-login";
   user_info.pw_gecos = NULL;
   ODInitApplyUserInfo(&user_info);
   CHECK(strcmp(od_control.user_handle, "replacement-login") == 0);
   CHECK(strcmp(od_control.user_name, "Replacement Name") == 0);
#endif

#if defined(ODPLAT_WIN32) || defined(ODPLAT_NIX)
   od_control.od_open_handle = 17;
   od_control.od_use_socket = FALSE;
   od_control.od_error = ERR_NONE;
   ParseHandleOption("-HANDLE", "4294967295");
   CHECK(od_control.od_open_handle == (DWORD_PTR)4294967295UL);
   CHECK(!od_control.od_use_socket);
   CHECK(od_control.od_error == ERR_NONE);

   od_control.od_open_handle = 18;
   od_control.od_error = ERR_NONE;
   if(sizeof(DWORD_PTR) > 4)
   {
      ParseHandleOption("-HANDLE", "18446744073709551615");
      CHECK(od_control.od_open_handle == (DWORD_PTR)-1);
      CHECK(od_control.od_error == ERR_NONE);

      od_control.od_open_handle = 18;
      ParseHandleOption("-HANDLE", "18446744073709551616");
   }
   else
   {
      ParseHandleOption("-HANDLE", "4294967296");
   }
   CHECK(od_control.od_open_handle == 18);
   CHECK(od_control.od_error == ERR_PARAMETER);

   od_control.od_open_handle = 19;
   od_control.od_error = ERR_NONE;
   ParseHandleOption("-HANDLE", "123x");
   CHECK(od_control.od_open_handle == 19);
   CHECK(od_control.od_error == ERR_PARAMETER);

   od_control.od_open_handle = 20;
   od_control.od_use_socket = FALSE;
   od_control.od_error = ERR_NONE;
   ParseHandleOption("-SOCKET", "-1");
   CHECK(od_control.od_open_handle == 20);
   CHECK(!od_control.od_use_socket);
   CHECK(od_control.od_error == ERR_PARAMETER);

   od_control.od_error = ERR_NONE;
   ParseHandleOption("-SOCKET", "+21");
   CHECK(od_control.od_open_handle == 21);
   CHECK(od_control.od_use_socket);
   CHECK(od_control.od_error == ERR_NONE);
#endif

   od_control.od_num_keys = 0;
   ODStatAddKey(0x1111);
   od_control.od_hot_function[0] = HotKeyOne;
   ODStatAddKey(0x2222);
   od_control.od_hot_function[1] = HotKeyTwo;
   ODStatAddKey(0x3333);
   od_control.od_hot_function[2] = HotKeyThree;

   ODStatRemoveKey(0x2222);
   CHECK(od_control.od_num_keys == 2);
   CHECK(od_control.od_hot_key[0] == 0x1111);
   CHECK(od_control.od_hot_function[0] == HotKeyOne);
   CHECK(od_control.od_hot_key[1] == 0x3333);
   CHECK(od_control.od_hot_function[1] == HotKeyThree);
   CHECK(od_control.od_hot_key[2] == 0);
   CHECK(od_control.od_hot_function[2] == NULL);

   ODStatRemoveKey(0x3333);
   CHECK(od_control.od_num_keys == 1);
   CHECK(od_control.od_hot_key[0] == 0x1111);
   CHECK(od_control.od_hot_function[0] == HotKeyOne);
   CHECK(od_control.od_hot_key[1] == 0);
   CHECK(od_control.od_hot_function[1] == NULL);

   ODStatRemoveKey(0x9999);
   CHECK(od_control.od_num_keys == 1);
   CHECK(od_control.od_hot_key[0] == 0x1111);
   CHECK(od_control.od_hot_function[0] == HotKeyOne);

   return(0);
}
