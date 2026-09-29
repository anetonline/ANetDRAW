#ifndef OPENDOORS_UNIT_TURBO_I86_H
#define OPENDOORS_UNIT_TURBO_I86_H

/* Let Open Watcom supply the DOS declarations used to model Turbo C, then
 * replace its segment-operator spelling, which Clang cannot parse. */
#include_next <i86.h>

#undef MK_FP
#define MK_FP(s, o) \
   ((void *)((((unsigned long)(s)) << 16) + (unsigned)(o)))

#endif
