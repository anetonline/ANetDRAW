#include "OpenDoor.h"
#include "ODStat.h"

int personality_header_smoke()
{
    return sizeof(OD_PERSONALITY_PROC *) == sizeof(void *) ? 0 : 1;
}
