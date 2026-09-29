/* OpenDoors 32-bit DOS FOSSIL support. */

#ifndef _INC_OD32FOSS
#define _INC_OD32FOSS

#include "OpenDoor.h"

#ifdef ODPLAT_DOS32

typedef struct
{
   WORD wRealSegment;
   WORD wSelector;
   BYTE *pLinear;
   WORD wSize;
} tOD32FossilBuffer;

BOOL OD32FossilDetect(BYTE btPort);
void OD32FossilInitialize(BYTE btPort, BYTE btSettings);
void OD32FossilClose(BYTE btPort);
void OD32FossilSetFlow(BYTE btPort, BYTE btFlow);
WORD OD32FossilStatus(BYTE btPort);
void OD32FossilSetDTR(BYTE btPort, BOOL bHigh);
void OD32FossilClearOutbound(BYTE btPort);
void OD32FossilClearInbound(BYTE btPort);
BYTE OD32FossilGetByte(BYTE btPort);
BOOL OD32FossilSendByte(BYTE btPort, BYTE btValue);

BOOL OD32FossilBufferAllocate(tOD32FossilBuffer *pBuffer, WORD wSize);
void OD32FossilBufferFree(tOD32FossilBuffer *pBuffer);
INT OD32FossilReceiveBlock(BYTE btPort, tOD32FossilBuffer *pBuffer,
   BYTE *pDestination, INT nSize);
INT OD32FossilSendBlock(BYTE btPort, tOD32FossilBuffer *pBuffer,
   const BYTE *pSource, INT nSize);

#endif /* ODPLAT_DOS32 */

#endif /* _INC_OD32FOSS */
