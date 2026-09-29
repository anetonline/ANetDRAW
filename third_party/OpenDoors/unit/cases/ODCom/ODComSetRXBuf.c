#define UT_CALL(handle, value) utt_ODComSetRXBuf((handle), (value))
#define UT_VALUE 2048
#define UT_FIELD(port) ((port)->nReceiveBufferSize)
#include "simple_setter_fixture.h"
