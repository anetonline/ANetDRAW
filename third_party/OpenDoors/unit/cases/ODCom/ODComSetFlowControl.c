#define UT_CALL(handle, value) utt_ODComSetFlowControl((handle), (value))
#define UT_VALUE (FLOW_NONE | FLOW_RTSCTS)
#define UT_FIELD(port) ((port)->btFlowControlSetting)
#include "simple_setter_fixture.h"
