#define UT_CALL(handle, value) utt_ODComSetTXBuf((handle), (value))
#define UT_VALUE 3072
#define UT_FIELD(port) ((port)->nTransmitBufferSize)
#include "simple_setter_fixture.h"
