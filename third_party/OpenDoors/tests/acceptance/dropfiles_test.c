#include <stdlib.h>

#include "test_support.h"

static int WriteLines(const char *name, const char *const *lines, size_t count)
{
   FILE *file;
   size_t index;
   file = fopen(name, "w");
   if(file == NULL)
      return(0);
   for(index = 0; index < count; ++index)
      if(fprintf(file, "%s\n", lines[index]) < 0)
      {
         fclose(file);
         return(0);
      }
   return(fclose(file) == 0);
}

static int RunDropFile(const char *name, BYTE type, const char *user,
   BOOL eight_bit)
{
   memset(&od_control, 0, sizeof(od_control));
   strncpy(od_control.info_path, name, sizeof(od_control.info_path) - 1);
   od_control.od_disable = DIS_NAME_PROMPT | DIS_CARRIERDETECT |
      DIS_TIMEOUT | DIS_BPS_SETTING | DIS_LOCAL_INPUT;
   od_control.od_silent_mode = TRUE;
   od_control.od_nocopyright = TRUE;
   od_control.od_noexit = TRUE;
   od_control.od_errorlevel[ERRORLEVEL_CRITICAL] = 97;
   fprintf(stderr, "%s: initializing\n", name);
   od_init();
   fprintf(stderr, "%s: initialized\n", name);
   if(od_control.od_info_type != type ||
      od_get_user_8bit() != eight_bit ||
      (user != NULL && strcmp(od_control.user_name, user) != 0))
   {
      fprintf(stderr,
         "%s: expected type %u/user %s/8-bit %u, got %u/%s/%u\n", name,
         (unsigned)type, user == NULL ? "(not checked)" : user,
         (unsigned)eight_bit,
         (unsigned)od_control.od_info_type,
         od_control.user_name, (unsigned)od_get_user_8bit());
      od_exit(0, FALSE);
      return(0);
   }
   fprintf(stderr, "%s: shutting down\n", name);
   od_exit(0, FALSE);
   fprintf(stderr, "%s: shut down\n", name);
   return(1);
}

static int TestDorinfo(void)
{
   static const char *const lines[] = {
      "Acceptance BBS", "Test", "Sysop", "COM0", "38400", "8N1",
      "Fixture", "Caller", "Test Lab", "1", "42", "60"
   };
   return(WriteLines("dorinfo1.def", lines, sizeof(lines) / sizeof(lines[0])) &&
      RunDropFile("dorinfo1.def", DORINFO1, "Fixture Caller", TRUE));
}

static int TestChain(void)
{
   static const char *const lines[] = {
      "17", "Fixture Alias", "Fixture Caller", "N0CALL", "unused", "M",
      "unused", "01-02-24", "100", "30", "42", "0", "0", "1", "0",
      "3600", "unused", "unused", "unused", "KB", "0", "unused",
      "secret", "unused", "unused", "unused", "unused", "unused", "unused",
      "N,8,1"
   };
   return(WriteLines("chain.txt", lines, sizeof(lines) / sizeof(lines[0])) &&
      RunDropFile("chain.txt", CHAINTXT, "Fixture Caller", TRUE));
}

static int TestSfdoors(void)
{
   static const char *const lines[] = {
      "17", "Fixture Caller", "secret", "unused", "0", "0", "60",
      "unused", "unused", "TRUE", "42", "3", "4", "unused", "754",
      "unused", "TRUE", "unused", "unused", "unused", "TRUE", "11",
      "12", "3", "unused", "unused", "unused", "13", "14", "15",
      "555-1212", "Test Lab", "unused", "TRUE", "FALSE", "unused",
      "4", "1016", "unused"
   };
   return(WriteLines("sfdoors.dat", lines, sizeof(lines) / sizeof(lines[0])) &&
      RunDropFile("sfdoors.dat", SFDOORSDAT, "Fixture Caller", FALSE));
}

static int TestDoorway(void)
{
   static const char *const lines[] = {
      "Fixture Caller", "0", "0", "60", "G"
   };
   return(WriteLines("door.sys", lines, sizeof(lines) / sizeof(lines[0])) &&
      RunDropFile("door.sys", DOORSYS_DRWY, "Fixture Caller", FALSE));
}

static void FillGap(const char **lines, size_t count)
{
   size_t index;
   for(index = 0; index < count; ++index)
      lines[index] = "0";
   lines[0] = "COM0:STDIO";
   lines[1] = "0";
   lines[2] = "8N1";
   lines[3] = "3";
   lines[4] = "N";
   lines[9] = "Fixture Caller";
   lines[10] = "Test Lab";
   lines[11] = "555-1212";
   lines[12] = "555-1213";
   lines[13] = "secret";
   lines[14] = "42";
   lines[18] = "60";
   lines[19] = "GR";
   lines[20] = "30";
   lines[23] = "plain protocol";
   lines[24] = "01-02-24";
   lines[25] = "17";
}

static int TestGap(void)
{
   const char *lines[31];
   FillGap(lines, sizeof(lines) / sizeof(lines[0]));
   return(WriteLines("door.sys", lines, sizeof(lines) / sizeof(lines[0])) &&
      RunDropFile("door.sys", DOORSYS_GAP, "Fixture Caller", TRUE));
}

static int TestWildcat(void)
{
   const char *lines[55];
   FillGap(lines, sizeof(lines) / sizeof(lines[0]));
   lines[35] = "extra";
   lines[36] = "01-02-90";
   lines[37] = "Test Sysop";
   lines[38] = "Fixture Alias";
   lines[39] = "23:30";
   lines[40] = "Y";
   lines[46] = "12:34";
   lines[47] = "11:22";
   lines[50] = "100";
   lines[51] = "200";
   lines[52] = "Fixture comment";
   lines[54] = "9";
   return(WriteLines("door.sys", lines, sizeof(lines) / sizeof(lines[0])) &&
      RunDropFile("door.sys", DOORSYS_WILDCAT, "Fixture Caller", TRUE));
}

static int TestCallinfo(void)
{
   const char *lines[31];
   size_t index;
   for(index = 0; index < sizeof(lines) / sizeof(lines[0]); ++index)
      lines[index] = "unused";
   lines[0] = "Fixture Caller";
   lines[2] = "Test Lab";
   lines[3] = "42";
   lines[4] = "60";
   lines[5] = "G";
   lines[6] = "secret";
   lines[16] = "555-1212";
   lines[22] = "30";
   lines[28] = "COM0";
   lines[30] = "0";
   return(WriteLines("callinfo.bbs", lines, sizeof(lines) / sizeof(lines[0])) &&
      RunDropFile("callinfo.bbs", CALLINFO, "Fixture Caller", FALSE));
}

static int TestTribbs(void)
{
   static const char *const lines[] = {
      "17", "Fixture Caller", "secret", "42", "Y", "Y", "60",
      "555-1212", "Test Lab", "01/02/90", "3", "0", "0", "0", "N",
      "Y", "Acceptance BBS", "Test Sysop", "Fixture Alias", "Y"
   };
   return(WriteLines("tribbs.sys", lines, sizeof(lines) / sizeof(lines[0])) &&
      RunDropFile("tribbs.sys", TRIBBSSYS, "Fixture Caller", FALSE));
}

static int TestDoor32(void)
{
   static const char *const lines[] = {
      "0", "0", "0", "Acceptance 1.0", "17", "Fixture Caller",
      "Fixture Alias", "42", "60", "1", "3"
   };
   return(WriteLines("door32.sys", lines, sizeof(lines) / sizeof(lines[0])) &&
      RunDropFile("door32.sys", DOOR32SYS, "Fixture Caller", FALSE));
}

static int TestCustom(void)
{
   static const char *const drop[] = { "ignored", "Fixture Caller" };
   static const char *const config[] = {
      "CustomFileName custom.dat", "CustomFileLine Ignore",
      "CustomFileLine UserName"
   };
   memset(&od_control, 0, sizeof(od_control));
   if(!WriteLines("custom.dat", drop, sizeof(drop) / sizeof(drop[0])) ||
      !WriteLines("custom.cfg", config, sizeof(config) / sizeof(config[0])))
      return(0);
   od_control.od_config_file = INCLUDE_CONFIG_FILE;
   od_control.od_config_filename = "custom.cfg";
   od_control.od_disable = DIS_NAME_PROMPT | DIS_CARRIERDETECT |
      DIS_TIMEOUT | DIS_BPS_SETTING | DIS_LOCAL_INPUT;
   od_control.od_silent_mode = TRUE;
   od_control.od_noexit = TRUE;
   fprintf(stderr, "custom.dat: initializing\n");
   od_init();
   fprintf(stderr, "custom.dat: initialized\n");
   if(od_control.od_info_type != CUSTOM ||
      strcmp(od_control.user_name, "Fixture Caller") != 0)
   {
      od_exit(0, FALSE);
      return(0);
   }
   fprintf(stderr, "custom.dat: shutting down\n");
   od_exit(0, FALSE);
   fprintf(stderr, "custom.dat: shut down\n");
   return(1);
}

static int WriteCanonicalExitInfo(size_t size)
{
   unsigned char *bytes;
   FILE *file;
   int result = 0;
   bytes = (unsigned char *)calloc(size, 1);
   if(bytes == NULL)
      return(0);
   /* The historical records are little-endian and byte packed.  num_calls
    * begins at byte two in every EXITINFO variant. */
   if(size >= 6)
   {
      bytes[2] = 0x78;
      bytes[3] = 0x56;
      bytes[4] = 0x34;
      bytes[5] = 0x12;
   }
   file = fopen("exitinfo.bbs", "wb");
   if(file != NULL)
   {
      if(fwrite(bytes, 1, size, file) == size)
         result = fclose(file) == 0;
      else
         fclose(file);
   }
   free(bytes);
   return(result);
}

static int TestExitInfoSize(size_t size, BYTE expected_type)
{
   static const char *const dorinfo[] = {
      "Acceptance BBS", "Test", "Sysop", "COM0", "0", "0",
      "Fixture", "Caller", "Test Lab", "1", "42", "60"
   };
   int result;
   FILE *file;
   unsigned char calls[4];
   if(!WriteCanonicalExitInfo(size) ||
      !WriteLines("dorinfo1.def", dorinfo,
         sizeof(dorinfo) / sizeof(dorinfo[0])))
      return(0);
   result = RunDropFile("exitinfo.bbs", expected_type, "Fixture Caller",
      FALSE);
   if(!result || od_control.system_calls != 0x12345678UL)
      return(0);
   file = fopen("exitinfo.bbs", "rb");
   if(file == NULL || fseek(file, 2, SEEK_SET) != 0 ||
      fread(calls, 1, sizeof(calls), file) != sizeof(calls))
   {
      if(file != NULL) fclose(file);
      return(0);
   }
   result = fclose(file) == 0 && calls[0] == 0x78 && calls[1] == 0x56 &&
      calls[2] == 0x34 && calls[3] == 0x12;
   return(result);
}

static int TestExitInfo(void)
{
   return(TestExitInfoSize(452, EXITINFO));
}

static int TestQbbs(void)
{
   return(TestExitInfoSize(902, QBBS275EXITINFO));
}

static int TestRa1(void)
{
   return(TestExitInfoSize(1493, RA1EXITINFO));
}

static int TestRa2(void)
{
   return(TestExitInfoSize(2363, RA2EXITINFO));
}

typedef int (*tDropFileTest)(void);

typedef struct
{
   const char *name;
   tDropFileTest function;
} tDropFileCase;

static void Cleanup(void)
{
   remove("dorinfo1.def");
   remove("chain.txt");
   remove("sfdoors.dat");
   remove("door.sys");
   remove("callinfo.bbs");
   remove("tribbs.sys");
   remove("door32.sys");
   remove("custom.dat");
   remove("custom.cfg");
   remove("exitinfo.bbs");
}

int main(int argc, char **argv)
{
   static const tDropFileCase cases[] = {
      { "dorinfo", TestDorinfo },
      { "chain", TestChain },
      { "sfdoors", TestSfdoors },
      { "doorway", TestDoorway },
      { "gap", TestGap },
      { "wildcat", TestWildcat },
      { "callinfo", TestCallinfo },
      { "tribbs", TestTribbs },
      { "door32", TestDoor32 },
      { "custom", TestCustom },
      { "exitinfo", TestExitInfo },
      { "qbbs275", TestQbbs },
      { "ra1", TestRa1 },
      { "ra2", TestRa2 }
   };
   size_t index;
   int result;

   if(argc != 2)
      return(2);
   Cleanup();
   for(index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index)
   {
      if(strcmp(argv[1], cases[index].name) == 0)
      {
         result = cases[index].function() ? 0 : 1;
         Cleanup();
         return(result);
      }
   }
   Cleanup();
   return(3);
}
