/*
 * ANSI C89 fallback for platforms whose runtime lacks vsnprintf.
 * Trio is embedded in this translation unit in its snprintf-only mode.
 */
#ifndef OPENDOORS_HAVE_VSNPRINTF

#ifndef NDEBUG
#define NDEBUG
#endif

#define TRIO_SNPRINTF_ONLY
#define TRIO_MINIMAL
#ifdef __TURBOC__
#define TRIO_C99 0
#define TRIO_BSD 0
#define TRIO_GNU 0
#define TRIO_MISC 0
#define TRIO_UNIX98 0
#define TRIO_MICROSOFT 0
#define TRIO_EXTENSION 0
#define TRIO_DEPRECATED 0
#define TRIO_IEEE_754
#include <dos.h>
#endif
#ifdef __TURBOC__
#include "trio/trio.c"
#else
#include "third_party/trio/trio.c"
#endif

int ODFallbackVsnprintf(char *buffer, size_t size, const char *format,
   va_list args)
{
   return(trio_vsnprintf(buffer, size, format, args));
}

#endif /* !OPENDOORS_HAVE_VSNPRINTF */
