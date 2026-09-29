#define UT_CUSTOM_MOCK_free
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODConsoleShutdown
#define UT_CUSTOM_MOCK_ODMutexDestroy
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#endif

static unsigned ut_free_calls;
static void *ut_freed[2];

void utm_free(void *memory)
{
   UT_ASSERT(ut_free_calls < 2); ut_freed[ut_free_calls++] = memory;
}

#ifdef ODPLAT_WIN32
static tODWindowsSubsystem ut_subsystem;
static unsigned ut_console_calls;
static unsigned ut_destroy_calls;
void utm_ODConsoleShutdown(void) { ++ut_console_calls; }
void utm_ODMutexDestroy(tODMutex *mutex)
{
   ++ut_destroy_calls; UT_ASSERT(mutex == &ScreenPresentationMutex);
}
tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{ return(ut_subsystem); }

static void releases_both_screen_generations_when_active(void)
{
   static BYTE owner[2];
   static BYTE display[2];
   ut_free_calls = ut_destroy_calls = ut_console_calls = 0;
   ut_subsystem = kODWindowsSubsystemGUI;
   pScrnBuffer = owner; pDisplayBuffer = display;
   bScreenPresentationActive = FALSE; bScreenDirty = TRUE;
   utt_ODScrnShutdown();
   UT_ASSERT_EQ_UINT(0, ut_free_calls); UT_ASSERT_EQ_UINT(0, ut_destroy_calls);

   bScreenPresentationActive = TRUE;
   utt_ODScrnShutdown();
   UT_ASSERT_EQ_UINT(2, ut_free_calls);
   UT_ASSERT_EQ_PTR(owner, ut_freed[0]); UT_ASSERT_EQ_PTR(display, ut_freed[1]);
   UT_ASSERT_EQ_PTR(NULL, pScrnBuffer); UT_ASSERT_EQ_PTR(NULL, pDisplayBuffer);
   UT_ASSERT_EQ_INT(FALSE, bScreenPresentationActive);
   UT_ASSERT_EQ_INT(FALSE, bScreenDirty); UT_ASSERT_EQ_UINT(1, ut_destroy_calls);

   ut_free_calls = ut_destroy_calls = ut_console_calls = 0;
   pScrnBuffer = owner; pDisplayBuffer = display;
   bScreenPresentationActive = TRUE; bScreenDirty = TRUE;
   ut_subsystem = kODWindowsSubsystemConsole;
   utt_ODScrnShutdown();
   UT_ASSERT_EQ_UINT(2, ut_free_calls);
   UT_ASSERT_EQ_UINT(1, ut_console_calls);
   UT_ASSERT_EQ_UINT(0, ut_destroy_calls);
}

static const UTTestCase ut_cases[] = {
   {"screen generations", releases_both_screen_generations_when_active}
};
#else
static void releases_only_owned_screen_storage(void)
{
   static BYTE memory[2];
   ut_free_calls = 0; ut_freed[0] = NULL;
#ifdef ODPLAT_NIX
   pAllocatedBufferMemory = memory; pScrnBuffer = memory;
   utt_ODScrnShutdown();
   UT_ASSERT_EQ_UINT(1, ut_free_calls); UT_ASSERT_EQ_PTR(memory, ut_freed[0]);
   UT_ASSERT_EQ_PTR(NULL, pAllocatedBufferMemory); UT_ASSERT_EQ_PTR(NULL, pScrnBuffer);
#else
   od_control.od_silent_mode = FALSE; pAllocatedBufferMemory = memory;
   utt_ODScrnShutdown(); UT_ASSERT_EQ_UINT(0, ut_free_calls);
   od_control.od_silent_mode = TRUE; pAllocatedBufferMemory = NULL;
   utt_ODScrnShutdown(); UT_ASSERT_EQ_UINT(0, ut_free_calls);
   pAllocatedBufferMemory = memory; pScrnBuffer = memory;
   utt_ODScrnShutdown(); UT_ASSERT_EQ_UINT(1, ut_free_calls);
   UT_ASSERT_EQ_PTR(NULL, pAllocatedBufferMemory); UT_ASSERT_EQ_PTR(NULL, pScrnBuffer);
#endif
}

static const UTTestCase ut_cases[] = {
   {"owned storage", releases_only_owned_screen_storage}
};
#endif
