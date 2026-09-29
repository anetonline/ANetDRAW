#define UT_CALL(handle, value) utt_ODComSetSpeed((handle), (value))
#define UT_VALUE 57600L
#define UT_FIELD(port) ((port)->lSpeed)
#include "simple_setter_fixture.h"
