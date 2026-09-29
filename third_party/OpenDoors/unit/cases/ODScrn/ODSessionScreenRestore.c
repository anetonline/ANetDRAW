#define UT_CUSTOM_MOCK_ODSessionSnapshotGetDWORD
#define UT_CUSTOM_MOCK_ODSessionSnapshotDimensions
#define UT_CUSTOM_MOCK_od_puttext
#define UT_CUSTOM_MOCK_od_clr_scr
#define UT_CUSTOM_MOCK_od_putch
#define UT_CUSTOM_MOCK_od_disp_str
#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_ODSessionScreenMarkDirty
#define UT_CUSTOM_MOCK_ODSessionScreenPresent
#define UT_CUSTOM_MOCK_od_set_cursor
#define UT_CUSTOM_MOCK_od_set_attrib
#define UT_CUSTOM_MOCK_ODScrnPutText
#define UT_CUSTOM_MOCK_ODScrnSetBoundary
#define UT_CUSTOM_MOCK_ODScrnSetCursorPos
#define UT_CUSTOM_MOCK_ODScrnSetAttribute
#define UT_CUSTOM_MOCK_ODScrnEnableScrolling

static BYTE ut_snapshot[80];
static BYTE ut_session_cells[32];
static BOOL ut_dimensions_result;
static INT ut_current_width;
static INT ut_current_height;
static BOOL ut_remote_put_result;
static BOOL ut_local_put_result;
static unsigned ut_remote_put_calls;
static unsigned ut_clear_calls;
static unsigned ut_putch_calls;
static char ut_putchars[16];
static unsigned ut_disp_calls;
static unsigned ut_copy_calls;
static unsigned ut_dirty_calls;
static unsigned ut_present_calls;
static unsigned ut_remote_cursor_calls;
static unsigned ut_remote_attribute_calls;
static unsigned ut_local_put_calls;
static unsigned ut_boundary_calls;
static unsigned ut_local_cursor_calls;
static unsigned ut_local_attribute_calls;
static unsigned ut_scrolling_calls;

static void put_dword(BYTE *destination, DWORD value)
{
   destination[0] = (BYTE)(value & 0xffUL);
   destination[1] = (BYTE)((value >> 8) & 0xffUL);
   destination[2] = (BYTE)((value >> 16) & 0xffUL);
   destination[3] = (BYTE)((value >> 24) & 0xffUL);
}

DWORD utm_ODSessionSnapshotGetDWORD(const BYTE *source)
{
   return((DWORD)source[0] | ((DWORD)source[1] << 8)
      | ((DWORD)source[2] << 16) | ((DWORD)source[3] << 24));
}

BOOL utm_ODSessionSnapshotDimensions(INT *pnWidth, INT *pnHeight)
{
   *pnWidth = ut_current_width;
   *pnHeight = ut_current_height;
   return ut_dimensions_result;
}

BOOL ODCALL utm_od_puttext(INT left, INT top, INT right, INT bottom,
   void *buffer)
{
   ++ut_remote_put_calls;
   UT_ASSERT_EQ_INT(1, left);
   UT_ASSERT_EQ_INT(1, top);
   UT_ASSERT_EQ_INT(ut_current_width, right);
   UT_ASSERT_EQ_INT(ut_current_height, bottom);
   UT_ASSERT_EQ_PTR(ut_snapshot + 48, buffer);
   return ut_remote_put_result;
}

void ODCALL utm_od_clr_scr(void)
{
   ++ut_clear_calls;
}

void ODCALL utm_od_putch(char value)
{
   if(ut_putch_calls < DIM(ut_putchars))
      ut_putchars[ut_putch_calls] = value;
   ++ut_putch_calls;
}

void ODCALL utm_od_disp_str(const char *text)
{
   ++ut_disp_calls;
   UT_ASSERT_EQ_INT('\r', text[0]);
   UT_ASSERT_EQ_INT('\n', text[1]);
   UT_ASSERT_EQ_INT('\0', text[2]);
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

void utm_ODSessionScreenMarkDirty(INT left, INT top, INT right, INT bottom)
{
   ++ut_dirty_calls;
   UT_ASSERT_EQ_INT(0, left);
   UT_ASSERT_EQ_INT(0, top);
   UT_ASSERT_EQ_INT(SessionScreen.nWidth - 1, right);
   UT_ASSERT_EQ_INT(SessionScreen.nHeight - 1, bottom);
}

void utm_ODSessionScreenPresent(void)
{
   ++ut_present_calls;
}

void ODCALL utm_od_set_cursor(INT row, INT column)
{
   ++ut_remote_cursor_calls;
   UT_ASSERT_EQ_INT(1, row);
   UT_ASSERT_EQ_INT(1, column);
}

void ODCALL utm_od_set_attrib(INT attribute)
{
   ++ut_remote_attribute_calls;
   UT_ASSERT_EQ_INT(0x2e, attribute);
}

BOOL ODCALL utm_ODScrnPutText(BYTE left, BYTE top, BYTE right, BYTE bottom,
   void *buffer)
{
   ++ut_local_put_calls;
   UT_ASSERT_EQ_UINT(1, left);
   UT_ASSERT_EQ_UINT(1, top);
   UT_ASSERT_EQ_UINT(2, right);
   UT_ASSERT_EQ_UINT(2, bottom);
   UT_ASSERT_EQ_PTR(ut_snapshot + 48, buffer);
   return ut_local_put_result;
}

void utm_ODScrnSetBoundary(BYTE left, BYTE top, BYTE right, BYTE bottom)
{
   ++ut_boundary_calls;
   UT_ASSERT_EQ_UINT(1, left);
   UT_ASSERT_EQ_UINT(1, top);
   UT_ASSERT_EQ_UINT(2, right);
   UT_ASSERT_EQ_UINT(2, bottom);
}

void ODCALL utm_ODScrnSetCursorPos(BYTE column, BYTE row)
{
   ++ut_local_cursor_calls;
   UT_ASSERT_EQ_UINT(1, column);
   UT_ASSERT_EQ_UINT(1, row);
}

void ODCALL utm_ODScrnSetAttribute(BYTE attribute)
{
   ++ut_local_attribute_calls;
   UT_ASSERT_EQ_UINT(0x2e, attribute);
}

void utm_ODScrnEnableScrolling(BOOL enable)
{
   ++ut_scrolling_calls;
   UT_ASSERT_EQ_INT(TRUE, enable);
}

static void make_snapshot(INT width, INT height)
{
   DWORD total;
   unsigned index;
   total = 48UL + (DWORD)width * (DWORD)height * 2UL;
   for(index = 0; index < sizeof(ut_snapshot); ++index)
      ut_snapshot[index] = 0;
   ut_snapshot[0] = 'O';
   ut_snapshot[1] = 'D';
   ut_snapshot[2] = 'S';
   ut_snapshot[3] = '1';
   put_dword(ut_snapshot + 4, 1);
   put_dword(ut_snapshot + 8, total);
   put_dword(ut_snapshot + 12, (DWORD)width);
   put_dword(ut_snapshot + 16, (DWORD)height);
   put_dword(ut_snapshot + 20, 1);
   put_dword(ut_snapshot + 24, 1);
   put_dword(ut_snapshot + 28, (DWORD)width);
   put_dword(ut_snapshot + 32, (DWORD)height);
   put_dword(ut_snapshot + 36, 1);
   put_dword(ut_snapshot + 40, 1);
   ut_snapshot[44] = 0x2e;
   ut_snapshot[45] = 1;
   for(index = 48; index < (unsigned)total; ++index)
      ut_snapshot[index] = (BYTE)('A' + ((index - 48) / 2));
}

static void reset_restore(void)
{
   make_snapshot(2, 2);
   memset(ut_session_cells, 0, sizeof(ut_session_cells));
   ut_dimensions_result = TRUE;
   ut_current_width = 2;
   ut_current_height = 2;
   ut_remote_put_result = TRUE;
   ut_local_put_result = TRUE;
   bSessionScreenAvailable = TRUE;
   SessionScreen.pCells = ut_session_cells;
   SessionScreen.nWidth = 2;
   SessionScreen.nHeight = 2;
   od_control.user_ansi = FALSE;
   od_control.user_avatar = FALSE;
   ut_remote_put_calls = ut_clear_calls = ut_putch_calls = ut_disp_calls = 0;
   ut_copy_calls = ut_dirty_calls = ut_present_calls = 0;
   ut_remote_cursor_calls = ut_remote_attribute_calls = 0;
   ut_local_put_calls = ut_boundary_calls = ut_local_cursor_calls = 0;
   ut_local_attribute_calls = ut_scrolling_calls = 0;
}

static void expect_rejected(DWORD size)
{
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenRestore(ut_snapshot, size));
}

static void rejects_missing_short_or_bad_signature_buffers(void)
{
   unsigned index;
   reset_restore();
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenRestore(NULL, 56));
   expect_rejected(47);
   for(index = 0; index < 4; ++index)
   {
      reset_restore();
      ut_snapshot[index] ^= 1;
      expect_rejected(56);
   }
   reset_restore();
   put_dword(ut_snapshot + 4, 2);
   expect_rejected(56);
}

static void rejects_impossible_payload_dimensions(void)
{
   reset_restore();
   put_dword(ut_snapshot + 12, 0);
   expect_rejected(56);
   reset_restore();
   put_dword(ut_snapshot + 16, 0);
   expect_rejected(56);
   reset_restore();
   put_dword(ut_snapshot + 12, 0xffffffffUL);
   put_dword(ut_snapshot + 16, 2);
   expect_rejected(56);
   reset_restore();
   put_dword(ut_snapshot + 12, 0x7fffffffUL);
   put_dword(ut_snapshot + 16, 2);
   expect_rejected(56);
}

static void rejects_each_inconsistent_metadata_field(void)
{
   reset_restore(); put_dword(ut_snapshot + 8, 55); expect_rejected(56);
   reset_restore(); expect_rejected(55);
   reset_restore(); ut_dimensions_result = FALSE; expect_rejected(56);
   reset_restore(); ut_current_width = 3; expect_rejected(56);
   reset_restore(); ut_current_height = 3; expect_rejected(56);
   reset_restore(); put_dword(ut_snapshot + 20, 0); expect_rejected(56);
   reset_restore(); put_dword(ut_snapshot + 24, 0); expect_rejected(56);
   reset_restore(); put_dword(ut_snapshot + 20, 2); put_dword(ut_snapshot + 28, 1); expect_rejected(56);
   reset_restore(); put_dword(ut_snapshot + 24, 2); put_dword(ut_snapshot + 32, 1); expect_rejected(56);
   reset_restore(); put_dword(ut_snapshot + 28, 3); expect_rejected(56);
   reset_restore(); put_dword(ut_snapshot + 32, 3); expect_rejected(56);
   reset_restore(); put_dword(ut_snapshot + 36, 0); expect_rejected(56);
   reset_restore(); put_dword(ut_snapshot + 36, 3); expect_rejected(56);
   reset_restore(); put_dword(ut_snapshot + 40, 0); expect_rejected(56);
   reset_restore(); put_dword(ut_snapshot + 40, 3); expect_rejected(56);
   reset_restore(); ut_snapshot[45] = 2; expect_rejected(56);
   reset_restore(); ut_snapshot[46] = 1; expect_rejected(56);
   reset_restore(); ut_snapshot[47] = 1; expect_rejected(56);
}

static void restores_ansi_and_avatar_session_screens(void)
{
   reset_restore();
   od_control.user_ansi = TRUE;
   ut_remote_put_result = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenRestore(ut_snapshot, 56));
   UT_ASSERT_EQ_UINT(1, ut_remote_put_calls);
   UT_ASSERT_EQ_UINT(0, ut_copy_calls);

   reset_restore();
   od_control.user_ansi = TRUE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionScreenRestore(ut_snapshot, 56));
   UT_ASSERT_EQ_UINT(1, ut_copy_calls);
   UT_ASSERT_EQ_UINT(1, ut_dirty_calls);
   UT_ASSERT_EQ_UINT(1, ut_present_calls);
   UT_ASSERT_EQ_UINT(1, ut_remote_cursor_calls);
   UT_ASSERT_EQ_UINT(1, ut_remote_attribute_calls);
   UT_ASSERT_EQ_INT(0, SessionScreen.nLeft);
   UT_ASSERT_EQ_INT(1, SessionScreen.nRight);
   UT_ASSERT_EQ_UINT(0x2e, SessionScreen.btAttribute);
   UT_ASSERT_EQ_INT(TRUE, SessionScreen.bScrolling);

   reset_restore();
   od_control.user_avatar = TRUE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionScreenRestore(ut_snapshot, 56));
   UT_ASSERT_EQ_UINT(1, ut_remote_put_calls);
   UT_ASSERT_EQ_UINT(1, ut_remote_cursor_calls);
}

static void restores_plain_text_with_trimming_and_cursor_protection(void)
{
   reset_restore();
   ut_snapshot[48] = 'A'; ut_snapshot[50] = ' ';
   ut_snapshot[52] = 'B'; ut_snapshot[54] = 'C';
   put_dword(ut_snapshot + 40, 2);
   put_dword(ut_snapshot + 36, 2);
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionScreenRestore(ut_snapshot, 56));
   UT_ASSERT_EQ_UINT(1, ut_clear_calls);
   UT_ASSERT_EQ_UINT(2, ut_putch_calls);
   UT_ASSERT_EQ_INT('A', ut_putchars[0]);
   UT_ASSERT_EQ_INT('B', ut_putchars[1]);
   UT_ASSERT_EQ_UINT(1, ut_disp_calls);
   UT_ASSERT_EQ_UINT(0, ut_remote_cursor_calls);

   reset_restore();
   ut_snapshot[48] = 'A'; ut_snapshot[50] = 0;
   put_dword(ut_snapshot + 36, 2);
   put_dword(ut_snapshot + 40, 1);
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionScreenRestore(ut_snapshot, 56));
   UT_ASSERT_EQ_UINT(1, ut_putch_calls);

   reset_restore();
   ut_snapshot[48] = 'A'; ut_snapshot[50] = 'B';
   ut_snapshot[52] = 'C'; ut_snapshot[54] = 'D';
   put_dword(ut_snapshot + 36, 1);
   put_dword(ut_snapshot + 40, 2);
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionScreenRestore(ut_snapshot, 56));
   UT_ASSERT_EQ_UINT(0, ut_disp_calls);
}

static void restores_or_rejects_the_fixed_local_screen(void)
{
   reset_restore();
   bSessionScreenAvailable = FALSE;
   ut_local_put_result = FALSE;
   UT_ASSERT_EQ_INT(FALSE, utt_ODSessionScreenRestore(ut_snapshot, 56));
   UT_ASSERT_EQ_UINT(1, ut_local_put_calls);
   UT_ASSERT_EQ_UINT(0, ut_boundary_calls);

   reset_restore();
   bSessionScreenAvailable = FALSE;
   UT_ASSERT_EQ_INT(TRUE, utt_ODSessionScreenRestore(ut_snapshot, 56));
   UT_ASSERT_EQ_UINT(1, ut_boundary_calls);
   UT_ASSERT_EQ_UINT(1, ut_local_cursor_calls);
   UT_ASSERT_EQ_UINT(1, ut_local_attribute_calls);
   UT_ASSERT_EQ_UINT(1, ut_scrolling_calls);
}

static const UTTestCase ut_cases[] = {
   {"signature validation", rejects_missing_short_or_bad_signature_buffers},
   {"payload dimensions", rejects_impossible_payload_dimensions},
   {"metadata validation", rejects_each_inconsistent_metadata_field},
   {"ANSI and AVATAR restore", restores_ansi_and_avatar_session_screens},
   {"plain-text restore", restores_plain_text_with_trimming_and_cursor_protection},
   {"local restore", restores_or_rejects_the_fixed_local_screen}
};
