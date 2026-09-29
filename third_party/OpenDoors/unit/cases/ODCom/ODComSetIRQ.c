#define UT_CALL(handle, value) utt_ODComSetIRQ((handle), (value))
#define UT_VALUE 5
#define UT_FIELD(port) ((port)->btIRQLevel)
#include "simple_setter_fixture.h"
