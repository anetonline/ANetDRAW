#ifndef OPENDOORS_UNIT_TURBO_AST_H
#define OPENDOORS_UNIT_TURBO_AST_H

/* Open Watcom's headers are used only to let Clang model Turbo C sources.
 * Supply Borland-only DOS declarations that Watcom intentionally lacks. */
#ifdef __TURBOC__
struct date
{
   int da_year;
   char da_day;
   char da_mon;
};

struct time
{
   unsigned char ti_min;
   unsigned char ti_hour;
   unsigned char ti_hund;
   unsigned char ti_sec;
};

long dostounix(struct date *date_value, struct time *time_value);
#endif

#endif
