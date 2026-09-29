#ifndef UT_ERRNO_STORAGE
#error UT_ERRNO_STORAGE must name the case-local errno storage
#endif

#define UT_CUSTOM_MOCK___error
#define UT_CUSTOM_MOCK___errno_location
#define __error utm___error
#define __errno_location utm___errno_location

int *utm___error(void) { return(&UT_ERRNO_STORAGE); }
int *utm___errno_location(void) { return(&UT_ERRNO_STORAGE); }
