#define UT_CUSTOM_MOCK_ODSessionScreenSnapshotSize
#define UT_CUSTOM_MOCK_ODSessionSnapshotPutDWORD
#define UT_CUSTOM_MOCK_ODScrnGetTextInfo
#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_ODScrnGetText

static DWORD ut_required;
static DWORD ut_dwords[10];
static unsigned ut_dword_calls;
static tODScrnTextInfo ut_local_info;
static unsigned ut_info_calls;
static unsigned ut_copy_calls;
static unsigned ut_get_calls;
static BOOL ut_get_result;

DWORD utm_ODSessionScreenSnapshotSize(void)
{
   return ut_required;
}

void utm_ODSessionSnapshotPutDWORD(BYTE *pDest, DWORD dwValue)
{
   if(ut_dword_calls < DIM(ut_dwords))
      ut_dwords[ut_dword_calls] = dwValue;
   ++ut_dword_calls;
   pDest[0] = (BYTE)(dwValue & 0xffUL);
   pDest[1] = (BYTE)((dwValue >> 8) & 0xffUL);
   pDest[2] = (BYTE)((dwValue >> 16) & 0xffUL);
   pDest[3] = (BYTE)((dwValue >> 24) & 0xffUL);
}

void utm_ODScrnGetTextInfo(tODScrnTextInfo *pInfo)
{
   ++ut_info_calls;
   *pInfo = ut_local_info;
}

void *utm_memcpy(void *destination, const void *source, size_t count)
{
   BYTE *dest;
   const BYTE *src;
   size_t index;
   ++ut_copy_calls;
   dest = (BYTE *)destination;
   src = (const BYTE *)source;
   for(index = 0; index < count; ++index)
      dest[index] = src[index];
   return destination;
}

BOOL ODCALL utm_ODScrnGetText(BYTE btLeft, BYTE btTop, BYTE btRight,
   BYTE btBottom, void *pBuffer)
{
   BYTE *bytes;
   unsigned index;
   ++ut_get_calls;
   UT_ASSERT_EQ_UINT(1, btLeft);
   UT_ASSERT_EQ_UINT(1, btTop);
   UT_ASSERT_EQ_UINT(3, btRight);
   UT_ASSERT_EQ_UINT(2, btBottom);
   bytes = (BYTE *)pBuffer;
   for(index = 0; index < 12; ++index)
      bytes[index] = (BYTE)(0x80 + index);
   return ut_get_result;
}

static void reset_save(void)
{
   memset(&ut_local_info, 0, sizeof(ut_local_info));
   ut_required = 56;
   ut_dword_calls = ut_info_calls = ut_copy_calls = ut_get_calls = 0;
   ut_get_result = TRUE;
}

static void rejects_each_invalid_destination_condition(void)
{
   BYTE buffer[56];
   reset_save();
   ut_required = 0;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenSave(buffer, sizeof(buffer)));
   ut_required = 56;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenSave(NULL, sizeof(buffer)));
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenSave(buffer, 55));
   UT_ASSERT_EQ_UINT(0, ut_dword_calls);
}

static void saves_session_metadata_and_cells(void)
{
   BYTE buffer[56];
   BYTE cells[8];
   unsigned index;
   reset_save();
   for(index = 0; index < sizeof(cells); ++index)
      cells[index] = (BYTE)(index + 1);
   memset(buffer, 0, sizeof(buffer));
   bSessionScreenAvailable = TRUE;
   SessionScreen.pCells = cells;
   SessionScreen.nWidth = 2;
   SessionScreen.nHeight = 2;
   SessionScreen.nLeft = 0;
   SessionScreen.nTop = 0;
   SessionScreen.nRight = 1;
   SessionScreen.nBottom = 1;
   SessionScreen.nCursorColumn = 1;
   SessionScreen.nCursorRow = 0;
   SessionScreen.btAttribute = 0x2e;
   SessionScreen.bScrolling = TRUE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionScreenSave(buffer, sizeof(buffer)));
   UT_ASSERT_EQ_UINT('O', buffer[0]);
   UT_ASSERT_EQ_UINT('D', buffer[1]);
   UT_ASSERT_EQ_UINT('S', buffer[2]);
   UT_ASSERT_EQ_UINT('1', buffer[3]);
   UT_ASSERT_EQ_UINT(10, ut_dword_calls);
   UT_ASSERT_EQ_UINT(1, ut_dwords[0]);
   UT_ASSERT_EQ_UINT(56, ut_dwords[1]);
   UT_ASSERT_EQ_UINT(2, ut_dwords[2]);
   UT_ASSERT_EQ_UINT(2, ut_dwords[3]);
   UT_ASSERT_EQ_UINT(1, ut_dwords[4]);
   UT_ASSERT_EQ_UINT(1, ut_dwords[5]);
   UT_ASSERT_EQ_UINT(2, ut_dwords[6]);
   UT_ASSERT_EQ_UINT(2, ut_dwords[7]);
   UT_ASSERT_EQ_UINT(2, ut_dwords[8]);
   UT_ASSERT_EQ_UINT(1, ut_dwords[9]);
   UT_ASSERT_EQ_UINT(0x2e, buffer[44]);
   UT_ASSERT_EQ_UINT(1, buffer[45]);
   UT_ASSERT_EQ_UINT(0, buffer[46]);
   UT_ASSERT_EQ_UINT(0, buffer[47]);
   UT_ASSERT_EQ_UINT(1, ut_copy_calls);
   for(index = 0; index < sizeof(cells); ++index)
      UT_ASSERT_EQ_UINT(cells[index], buffer[48 + index]);

   reset_save();
   bSessionScreenAvailable = TRUE;
   SessionScreen.pCells = cells;
   SessionScreen.nWidth = SessionScreen.nHeight = 2;
   SessionScreen.nLeft = SessionScreen.nTop = 0;
   SessionScreen.nRight = SessionScreen.nBottom = 1;
   SessionScreen.nCursorColumn = SessionScreen.nCursorRow = 0;
   SessionScreen.bScrolling = FALSE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionScreenSave(buffer, sizeof(buffer)));
   UT_ASSERT_EQ_UINT(0, buffer[45]);
}

static void saves_the_local_screen_and_returns_its_result(void)
{
   BYTE buffer[60];
   reset_save();
   ut_required = 60;
   bSessionScreenAvailable = FALSE;
   ut_local_info.winleft = 4;
   ut_local_info.winright = 6;
   ut_local_info.wintop = 2;
   ut_local_info.winbottom = 3;
   ut_local_info.curx = 2;
   ut_local_info.cury = 1;
   ut_local_info.attribute = 0x17;
   ut_get_result = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenSave(buffer, sizeof(buffer)));
   UT_ASSERT_EQ_UINT(1, ut_info_calls);
   UT_ASSERT_EQ_UINT(1, ut_get_calls);
   UT_ASSERT_EQ_UINT(3, ut_dwords[2]);
   UT_ASSERT_EQ_UINT(2, ut_dwords[3]);
   UT_ASSERT_EQ_UINT(3, ut_dwords[6]);
   UT_ASSERT_EQ_UINT(2, ut_dwords[7]);
   UT_ASSERT_EQ_UINT(2, ut_dwords[8]);
   UT_ASSERT_EQ_UINT(1, ut_dwords[9]);
   UT_ASSERT_EQ_UINT(0x17, buffer[44]);
   UT_ASSERT_EQ_UINT(1, buffer[45]);

   reset_save();
   ut_required = 60;
   bSessionScreenAvailable = FALSE;
   ut_local_info.winleft = 4;
   ut_local_info.winright = 6;
   ut_local_info.wintop = 2;
   ut_local_info.winbottom = 3;
   ut_get_result = TRUE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionScreenSave(buffer, sizeof(buffer)));
   UT_ASSERT_EQ_UINT(0x80, buffer[48]);
   UT_ASSERT_EQ_UINT(0x8b, buffer[59]);
}

static const UTTestCase ut_cases[] = {
   {"invalid destination", rejects_each_invalid_destination_condition},
   {"session snapshot", saves_session_metadata_and_cells},
   {"local snapshot", saves_the_local_screen_and_returns_its_result}
};
