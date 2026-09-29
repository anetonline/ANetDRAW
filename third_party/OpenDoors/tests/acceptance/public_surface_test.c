#include <stddef.h>
#include <stdlib.h>

/* Turbo C 2.01's <stddef.h> does not define the ANSI offsetof macro. */
#ifndef offsetof
#define offsetof(type, member) ((size_t)&(((type *)0)->member))
#endif

#define OD_ACCEPTANCE_NO_LOCAL_CONFIG
#ifdef __TURBOC__
#include "ATSUP.H"
#else
#include "test_support.h"
#endif

#if (defined(ODPLAT_WIN32) + defined(ODPLAT_NIX) + defined(ODPLAT_DOS) + \
     defined(ODPLAT_DOS32)) != 1
#error OpenDoor.h must select exactly one public platform
#endif

typedef struct
{
   const char *name;
   size_t offset;
   size_t size;
} tPublicField;

#define FIELD(name) { #name, offsetof(tODControl, name), \
   sizeof(((tODControl *)0)->name) },
static const tPublicField fields[] = {
#ifdef __TURBOC__
#include "ATFLDS.I"
#else
#include "control-fields.inc"
#endif
};
#undef FIELD

static int IsZero(const void *memory, size_t size)
{
   const unsigned char *bytes = (const unsigned char *)memory;
   size_t index;
   for(index = 0; index < size; ++index)
      if(bytes[index] != 0)
         return(0);
   return(1);
}

int main(void)
{
   static const tODControl zero_control;
   size_t index;
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   FILE *marker;
#endif

   OD_TEST_CHECK(sizeof(BYTE) == 1);
   OD_TEST_CHECK(sizeof(INT8) == 1);
   OD_TEST_CHECK(sizeof(WORD) == 2);
   OD_TEST_CHECK(sizeof(INT16) == 2);
   OD_TEST_CHECK(sizeof(DWORD) == 4);
   OD_TEST_CHECK(sizeof(INT32) == 4);
   OD_TEST_CHECK(sizeof(tODMilliSec) == sizeof(DWORD));
   OD_TEST_CHECK(OD_RESERVE_ERROR == -1);
   OD_TEST_CHECK(OD_RESERVE_PENDING == 0);
   OD_TEST_CHECK(OD_RESERVE_ACQUIRED == 1);
   OD_TEST_CHECK(OD_VERSION == 0x630);
   OD_TEST_CHECK(DORINFO1 == 0 && DOOR32SYS == 13 && NO_DOOR_FILE == 100);
   OD_TEST_CHECK(ERR_NONE == 0 && ERR_UNSUPPORTED == 11);
   OD_TEST_CHECK(D_BLACK == 0 && L_WHITE == 15);
   OD_TEST_CHECK(B_YELLOW == L_YELLOW && B_WHITE == L_WHITE);
   OD_TEST_CHECK(od_control_get() == &od_control);
   OD_TEST_CHECK(memcmp(&od_control, &zero_control, sizeof(od_control)) == 0);
#ifdef ODPLAT_WIN32
   OD_TEST_CHECK(sizeof(fields) / sizeof(fields[0]) == 256);
#else
   OD_TEST_CHECK(sizeof(fields) / sizeof(fields[0]) == 254);
#endif

   for(index = 0; index < sizeof(fields) / sizeof(fields[0]); ++index)
   {
      OD_TEST_CHECK(fields[index].name[0] != '\0');
      OD_TEST_CHECK(fields[index].size != 0);
      OD_TEST_CHECK(fields[index].offset <= sizeof(tODControl));
      OD_TEST_CHECK(fields[index].size <= sizeof(tODControl) - fields[index].offset);
      OD_TEST_CHECK(IsZero((const unsigned char *)&od_control +
         fields[index].offset, fields[index].size));
   }
   OD_TEST_CHECK(od_set_user_8bit(TRUE));
   OD_TEST_CHECK(od_get_user_8bit() == TRUE);
   OD_TEST_CHECK(od_set_user_8bit(FALSE));
   OD_TEST_CHECK(od_get_user_8bit() == FALSE);
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   marker = fopen("APIPASS.OK", "w");
   OD_TEST_CHECK(marker != NULL);
   OD_TEST_CHECK(fputs("OpenDoors public surface acceptance passed\n", marker) >= 0);
   OD_TEST_CHECK(fclose(marker) == 0);
#endif
   return(0);
}
