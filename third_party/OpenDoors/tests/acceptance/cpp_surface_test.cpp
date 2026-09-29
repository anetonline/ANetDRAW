#include <OpenDoor.h>
#include <ODStat.h>

extern "C" tODControl *acceptance_cpp_control(void)
{
    tODInputEvent event = {};
    tODEditOptions options = {};
    (void)event;
    (void)options;
    return od_control_get();
}
