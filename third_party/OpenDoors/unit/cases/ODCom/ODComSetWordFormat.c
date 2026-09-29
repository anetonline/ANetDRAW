#define UT_CALL(handle, value) utt_ODComSetWordFormat((handle), (value))
#define UT_VALUE (ODPARITY_EVEN | DATABITS_SEVEN | STOP_TWO)
#define UT_FIELD(port) ((port)->btWordFormat)
#include "simple_setter_fixture.h"
