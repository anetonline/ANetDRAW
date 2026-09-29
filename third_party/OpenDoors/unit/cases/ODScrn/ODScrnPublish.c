#define UT_CUSTOM_MOCK_ODMutexLock
#define UT_CUSTOM_MOCK_ODMutexUnlock
#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_InvalidateRect
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#define UT_CUSTOM_MOCK_ODScrnPublishConsole

static BYTE ut_owner[OD_SCREEN_WIDTH * OD_SCREEN_HEIGHT * 2];
static BYTE ut_display[OD_SCREEN_WIDTH * OD_SCREEN_HEIGHT * 2];
static unsigned ut_locks;
static unsigned ut_unlocks;
static unsigned ut_copies;
static unsigned ut_invalidates;
static tODWindowsSubsystem ut_subsystem;
static BOOL ut_console_result;
static unsigned ut_console_calls;

void utm_ODMutexLock(tODMutex *mutex)
{
   UT_ASSERT(mutex == &ScreenPresentationMutex);
   ++ut_locks;
}

void utm_ODMutexUnlock(tODMutex *mutex)
{
   UT_ASSERT(mutex == &ScreenPresentationMutex);
   ++ut_unlocks;
}

void *utm_memcpy(void *destination, const void *source, size_t size)
{
   BYTE *out = (BYTE *)destination;
   const BYTE *in = (const BYTE *)source;
   size_t index;
   UT_ASSERT_EQ_UINT(sizeof(ut_owner), size);
   ++ut_copies;
   for(index = 0; index < size; ++index)
      out[index] = in[index];
   return(destination);
}

BOOL WINAPI utm_InvalidateRect(HWND window, const RECT *rectangle, BOOL erase)
{
   UT_ASSERT(window == (HWND)(UINT_PTR)7);
   UT_ASSERT(rectangle == NULL);
   UT_ASSERT_EQ_INT(FALSE, erase);
   ++ut_invalidates;
   return(TRUE);
}
tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{ return(ut_subsystem); }
BOOL utm_ODScrnPublishConsole(void)
{ ++ut_console_calls; return(ut_console_result); }

static void reset_publish(void)
{
   memset(ut_owner, 0, sizeof(ut_owner));
   memset(ut_display, 0x55, sizeof(ut_display));
   pScrnBuffer = ut_owner;
   pDisplayBuffer = ut_display;
   bScreenPresentationActive = TRUE;
   bScreenDirty = FALSE;
   hwndScreenWindow = (HWND)(UINT_PTR)7;
   ut_subsystem = kODWindowsSubsystemGUI;
   ut_console_result = TRUE;
   ut_console_calls = 0;
   ut_locks = ut_unlocks = ut_copies = ut_invalidates = 0;
}

static void ignores_a_clean_or_inactive_screen(void)
{
   reset_publish();
   utt_ODScrnPublish();
   UT_ASSERT_EQ_UINT(0, ut_locks);
   bScreenDirty = TRUE;
   bScreenPresentationActive = FALSE;
   utt_ODScrnPublish();
   UT_ASSERT_EQ_UINT(0, ut_locks);
}

static void exchanges_complete_generations(void)
{
   reset_publish();
   ut_owner[0] = 'A';
   ut_owner[1] = 0x1f;
   btCursorColumn = 3;
   btCursorRow = 4;
   btLeftBoundary = 5;
   btTopBoundary = 6;
   bCaretOn = TRUE;
   bScreenDirty = TRUE;

   utt_ODScrnPublish();

   UT_ASSERT_EQ_PTR(ut_owner, pDisplayBuffer);
   UT_ASSERT_EQ_PTR(ut_display, pScrnBuffer);
   UT_ASSERT_EQ_INT('A', ut_display[0]);
   UT_ASSERT_EQ_INT(0x1f, ut_display[1]);
   UT_ASSERT_EQ_UINT(1, ut_locks);
   UT_ASSERT_EQ_UINT(1, ut_unlocks);
   UT_ASSERT_EQ_UINT(1, ut_copies);
   UT_ASSERT_EQ_UINT(1, ut_invalidates);
   UT_ASSERT_EQ_UINT(8, btDisplayCursorColumn);
   UT_ASSERT_EQ_UINT(10, btDisplayCursorRow);
   UT_ASSERT_EQ_INT(TRUE, bDisplayCaretOn);
   UT_ASSERT_EQ_INT(FALSE, bScreenDirty);
}

static void publishes_before_the_window_exists(void)
{
   reset_publish();
   hwndScreenWindow = NULL;
   bScreenDirty = TRUE;
   utt_ODScrnPublish();
   UT_ASSERT_EQ_UINT(1, ut_locks);
   UT_ASSERT_EQ_UINT(1, ut_unlocks);
   UT_ASSERT_EQ_UINT(1, ut_copies);
   UT_ASSERT_EQ_UINT(0, ut_invalidates);
   UT_ASSERT_EQ_INT(FALSE, bScreenDirty);
}

static void publishes_directly_to_a_console(void)
{
   reset_publish();
   ut_subsystem = kODWindowsSubsystemConsole;
   bScreenDirty = TRUE;
   utt_ODScrnPublish();
   UT_ASSERT_EQ_UINT(1, ut_console_calls);
   UT_ASSERT_EQ_INT(FALSE, bScreenDirty);
   UT_ASSERT_EQ_UINT(0, ut_locks);

   reset_publish();
   ut_subsystem = kODWindowsSubsystemConsole;
   ut_console_result = FALSE;
   bScreenDirty = TRUE;
   utt_ODScrnPublish();
   UT_ASSERT_EQ_UINT(1, ut_console_calls);
   UT_ASSERT_EQ_INT(TRUE, bScreenDirty);
}

static const UTTestCase ut_cases[] = {
   {"clean state", ignores_a_clean_or_inactive_screen},
   {"generation exchange", exchanges_complete_generations},
   {"no window", publishes_before_the_window_exists},
   {"console", publishes_directly_to_a_console}
};
