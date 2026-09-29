#define UT_CALL(handle, value) utt_ODComSetFIFO((handle), (value))
#define UT_VALUE (FIFO_ENABLE | FIFO_TRIGGER_14)
#define UT_FIELD(port) ((port)->btFIFOSetting)
#include "simple_setter_fixture.h"
