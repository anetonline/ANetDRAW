#define UT_CALL(handle, value) utt_ODComSetPortAddress((handle), (value))
#define UT_VALUE 0x3e8
#define UT_FIELD(port) ((port)->nPortAddress)
#include "simple_setter_fixture.h"
