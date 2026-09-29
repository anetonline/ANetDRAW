#define UT_CUSTOM_MOCK_ODScrnGetTextInfo
#define UT_CUSTOM_MOCK_ODSessionScreenCell
#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_ODScrnPutText
#define UT_CUSTOM_MOCK_ODScrnSetAttribute
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos
#define UT_CUSTOM_MOCK_ODScrnEnableCaret

static tODScrnTextInfo ut_info;
static BYTE ut_source_row[OD_SCREEN_WIDTH * 2];
static unsigned ut_info_calls;
static unsigned ut_cell_calls;
static unsigned ut_put_calls;
static unsigned ut_attribute_calls;
static unsigned ut_cursor_calls;
static unsigned ut_caret_calls;
static BYTE ut_first_put_left;
static BYTE ut_first_put_top;
static BYTE ut_last_put_right;
static BYTE ut_last_put_bottom;
static BOOL ut_last_caret;

void utm_ODScrnGetTextInfo(tODScrnTextInfo *pInfo)
{
   ++ut_info_calls;
   *pInfo = ut_info;
}

BYTE ODFAR *utm_ODSessionScreenCell(INT nColumn, INT nRow)
{
   ++ut_cell_calls;
   UT_ASSERT(nColumn >= 0);
   UT_ASSERT(nRow >= 0);
   return ut_source_row;
}

void *utm_memcpy(void *destination, const void *source, size_t count)
{
   BYTE *dest;
   const BYTE *src;
   size_t index;
   UT_ASSERT(count <= sizeof(ut_source_row));
   dest = (BYTE *)destination;
   src = (const BYTE *)source;
   for(index = 0; index < count; ++index)
      dest[index] = src[index];
   return destination;
}

BOOL ODCALL utm_ODScrnPutText(BYTE btLeft, BYTE btTop, BYTE btRight,
   BYTE btBottom, void *pBuffer)
{
   UT_ASSERT(pBuffer != NULL);
   if(ut_put_calls == 0)
   {
      ut_first_put_left = btLeft;
      ut_first_put_top = btTop;
   }
   ++ut_put_calls;
   ut_last_put_right = btRight;
   ut_last_put_bottom = btBottom;
   return TRUE;
}

void ODCALL utm_ODScrnSetAttribute(BYTE btAttribute)
{
   ++ut_attribute_calls;
   UT_ASSERT_EQ_UINT(SessionScreen.btAttribute, btAttribute);
}

void ODCALL utm_ODScrnSetCursorPos(BYTE btColumn, BYTE btRow)
{
   ++ut_cursor_calls;
   UT_ASSERT_EQ_UINT((unsigned)SessionScreen.nCursorColumn + 1U, btColumn);
   UT_ASSERT_EQ_UINT((unsigned)SessionScreen.nCursorRow + 1U, btRow);
}

void utm_ODScrnEnableCaret(BOOL bEnable)
{
   ++ut_caret_calls;
   ut_last_caret = bEnable;
   UT_ASSERT_EQ_INT(TRUE, bCaretPresentationChange);
}

static void reset_present(INT nLocalWidth, INT nLocalHeight,
   INT nSessionWidth, INT nSessionHeight)
{
   unsigned index;
   memset(&ut_info, 0, sizeof(ut_info));
   ut_info.winleft = 1;
   ut_info.wintop = 1;
   ut_info.winright = (BYTE)nLocalWidth;
   ut_info.winbottom = (BYTE)nLocalHeight;
   SessionScreen.nWidth = nSessionWidth;
   SessionScreen.nHeight = nSessionHeight;
   SessionScreen.btAttribute = 0x1e;
   SessionScreen.nCursorColumn = 0;
   SessionScreen.nCursorRow = 0;
   SessionScreen.bDirty = FALSE;
   bSessionScreenAvailable = TRUE;
   bSessionScreenEmulating = FALSE;
   bRequestedCaretOn = TRUE;
   bCaretPresentationChange = FALSE;
   for(index = 0; index < sizeof(ut_source_row); ++index)
      ut_source_row[index] = (BYTE)index;
   ut_info_calls = ut_cell_calls = ut_put_calls = 0;
   ut_attribute_calls = ut_cursor_calls = ut_caret_calls = 0;
   ut_last_caret = FALSE;
}

static void returns_when_unavailable_or_emulating(void)
{
   reset_present(10, 5, 10, 5);
   bSessionScreenAvailable = FALSE;
   utt_ODSessionScreenPresent();
   UT_ASSERT_EQ_UINT(0, ut_info_calls);

   bSessionScreenAvailable = TRUE;
   bSessionScreenEmulating = TRUE;
   utt_ODSessionScreenPresent();
   UT_ASSERT_EQ_UINT(0, ut_info_calls);
}

static void clips_a_dirty_region_to_the_presentable_screen(void)
{
   reset_present(100, 70, 100, 60);
   SessionScreen.bDirty = TRUE;
   SessionScreen.nDirtyLeft = -2;
   SessionScreen.nDirtyTop = -3;
   SessionScreen.nDirtyRight = 99;
   SessionScreen.nDirtyBottom = 70;
   SessionScreen.nCursorColumn = 50;
   SessionScreen.nCursorRow = 50;
   utt_ODSessionScreenPresent();
   UT_ASSERT_EQ_UINT(60, ut_cell_calls);
   UT_ASSERT_EQ_UINT(60, ut_put_calls);
   UT_ASSERT_EQ_UINT(1, ut_first_put_left);
   UT_ASSERT_EQ_UINT(1, ut_first_put_top);
   UT_ASSERT_EQ_UINT(80, ut_last_put_right);
   UT_ASSERT_EQ_UINT(60, ut_last_put_bottom);
   UT_ASSERT_EQ_INT(FALSE, SessionScreen.bDirty);
   UT_ASSERT_EQ_UINT(1, ut_attribute_calls);
   UT_ASSERT_EQ_UINT(1, ut_cursor_calls);
   UT_ASSERT_EQ_UINT(1, ut_caret_calls);
   UT_ASSERT_EQ_INT(TRUE, ut_last_caret);
   UT_ASSERT_EQ_INT(FALSE, bCaretPresentationChange);
}

static void suppresses_empty_dirty_regions_and_an_offscreen_cursor(void)
{
   reset_present(12, 5, 10, 10);
   SessionScreen.bDirty = FALSE;
   SessionScreen.nDirtyLeft = 0;
   SessionScreen.nDirtyTop = 0;
   SessionScreen.nDirtyRight = 1;
   SessionScreen.nDirtyBottom = 1;
   SessionScreen.nCursorColumn = 10;
   utt_ODSessionScreenPresent();
   UT_ASSERT_EQ_UINT(0, ut_put_calls);
   UT_ASSERT_EQ_UINT(0, ut_cursor_calls);
   UT_ASSERT_EQ_INT(FALSE, ut_last_caret);

   reset_present(5, 5, 10, 10);
   SessionScreen.bDirty = TRUE;
   SessionScreen.nDirtyLeft = 4;
   SessionScreen.nDirtyRight = 3;
   SessionScreen.nDirtyTop = 0;
   SessionScreen.nDirtyBottom = 0;
   SessionScreen.nCursorColumn = 4;
   SessionScreen.nCursorRow = 5;
   utt_ODSessionScreenPresent();
   UT_ASSERT_EQ_UINT(0, ut_put_calls);
   UT_ASSERT_EQ_UINT(0, ut_cursor_calls);
   UT_ASSERT_EQ_INT(FALSE, ut_last_caret);

   reset_present(5, 5, 10, 10);
   SessionScreen.bDirty = TRUE;
   SessionScreen.nDirtyLeft = 0;
   SessionScreen.nDirtyRight = 0;
   SessionScreen.nDirtyTop = 3;
   SessionScreen.nDirtyBottom = 2;
   utt_ODSessionScreenPresent();
   UT_ASSERT_EQ_UINT(0, ut_put_calls);
}

static void presents_an_unclipped_single_row(void)
{
   reset_present(5, 5, 10, 10);
   SessionScreen.bDirty = TRUE;
   SessionScreen.nDirtyLeft = 1;
   SessionScreen.nDirtyRight = 3;
   SessionScreen.nDirtyTop = 2;
   SessionScreen.nDirtyBottom = 2;
   utt_ODSessionScreenPresent();
   UT_ASSERT_EQ_UINT(1, ut_put_calls);
   UT_ASSERT_EQ_UINT(2, ut_first_put_left);
   UT_ASSERT_EQ_UINT(3, ut_first_put_top);
   UT_ASSERT_EQ_UINT(4, ut_last_put_right);
   UT_ASSERT_EQ_UINT(3, ut_last_put_bottom);
}

static const UTTestCase ut_cases[] = {
   {"presentation gates", returns_when_unavailable_or_emulating},
   {"clipped dirty region", clips_a_dirty_region_to_the_presentable_screen},
   {"empty regions and cursor", suppresses_empty_dirty_regions_and_an_offscreen_cursor},
   {"single dirty row", presents_an_unclipped_single_row}
};
