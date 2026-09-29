#define UT_CUSTOM_MOCK_malloc
#define UT_CUSTOM_MOCK_ODScrnClear
#define UT_CUSTOM_MOCK_ODScrnEnableCaret
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_free
#define UT_CUSTOM_MOCK_ODConsoleInitialize
#define UT_CUSTOM_MOCK_ODMutexInitialize
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#define UT_CUSTOM_MOCK_memcpy
#endif

#ifdef ODPLAT_DOS32
#define UT_CUSTOM_MOCK_memset
#define UT_CUSTOM_MOCK_int386
#endif
#if defined(ODPLAT_DOS) && !defined(ODPLAT_DOS32) && defined(__WATCOMC__)
#define UT_CUSTOM_MOCK_intdos
#endif

static BYTE ut_buffer[SCREEN_BUFFER_SIZE];
static unsigned ut_malloc_fail_call;
static unsigned ut_malloc_calls;
static unsigned ut_clear_calls;
static unsigned ut_caret_calls;
#ifdef ODPLAT_WIN32
static BYTE ut_display_buffer[SCREEN_BUFFER_SIZE];
static unsigned ut_free_calls;
static unsigned ut_console_calls;
static BOOL ut_console_result;
static tODWindowsSubsystem ut_subsystem;
static tODResult ut_mutex_result;
static unsigned ut_mutex_calls;
#endif
#if defined(ODPLAT_DOS) && !defined(ODPLAT_DOS32) && defined(__WATCOMC__)
static BOOL ut_desqview_present;
static unsigned ut_dos_calls;
#endif

static void *ut_fill(void *memory, int value, size_t count)
{
   BYTE *bytes = (BYTE *)memory;
   size_t index;
   for(index = 0; index < count; ++index) bytes[index] = (BYTE)value;
   return memory;
}

#ifdef ODPLAT_DOS32
static BYTE ut_video_mode;
static BYTE ut_video_page;
static unsigned ut_bios_query_calls;
static unsigned ut_bios_set_calls;
#endif

void *utm_malloc(size_t size)
{
   ++ut_malloc_calls; UT_ASSERT_EQ_UINT(SCREEN_BUFFER_SIZE, size);
   if(ut_malloc_calls == ut_malloc_fail_call) return NULL;
#ifdef ODPLAT_WIN32
   if(ut_malloc_calls == 2) return ut_display_buffer;
#endif
   return ut_buffer;
}

#ifdef ODPLAT_WIN32
void utm_free(void *memory)
{
   ++ut_free_calls;
   UT_ASSERT(memory == ut_buffer || memory == ut_display_buffer);
}

BOOL utm_ODConsoleInitialize(void)
{
   ++ut_console_calls;
   return(ut_console_result);
}

tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{
   return(ut_subsystem);
}

tODResult utm_ODMutexInitialize(tODMutex *mutex)
{
   ++ut_mutex_calls; UT_ASSERT(mutex == &ScreenPresentationMutex);
   return ut_mutex_result;
}

void *utm_memcpy(void *destination, const void *source, size_t count)
{
   size_t index;
   BYTE *output = (BYTE *)destination;
   const BYTE *input = (const BYTE *)source;
   UT_ASSERT_EQ_UINT(SCREEN_BUFFER_SIZE, count);
   for(index = 0; index < count; ++index) output[index] = input[index];
   return destination;
}
#endif

void utm_ODScrnClear(void)
{
   ++ut_clear_calls;
}

void utm_ODScrnEnableCaret(BOOL enabled)
{
   ++ut_caret_calls; UT_ASSERT(enabled); UT_ASSERT_EQ_INT(FALSE, bCaretOn);
}

#ifdef ODPLAT_DOS32
void *utm_memset(void *memory, int value, size_t count)
{
   return ut_fill(memory, value, count);
}

int utm_int386(int interrupt_number, const union REGS *input,
   union REGS *output)
{
   UT_ASSERT_EQ_INT(0x10, interrupt_number);
   UT_ASSERT(input == (const union REGS *)output);
   if(input->h.ah == 0x0f)
   {
      ++ut_bios_query_calls; output->h.al = ut_video_mode;
      output->h.bh = ut_video_page;
   }
   else
   {
      ++ut_bios_set_calls;
      UT_ASSERT(input->w.ax == 0x0003 || input->w.ax == 0x0007);
   }
   return 0;
}
#endif

#if defined(ODPLAT_DOS) && !defined(ODPLAT_DOS32)
static void set_video_mode(BYTE mode)
{
   union REGS registers;
   ut_fill(&registers, 0, sizeof(registers));
   registers.h.ah = 0;
   registers.h.al = mode;
   int86(0x10, &registers, &registers);
}

static void set_video_page(BYTE page)
{
   union REGS registers;
   ut_fill(&registers, 0, sizeof(registers));
   registers.h.ah = 0x05; registers.h.al = page;
   int86(0x10, &registers, &registers);
}

static void force_reported_video_mode(BYTE mode)
{
   BYTE ODFAR *bios_mode = (BYTE ODFAR *)MK_FP(0x0040, 0x0049);
   *bios_mode = mode;
}
#if defined(__WATCOMC__)
int utm_intdos(const union REGS *input, union REGS *output)
{
   ++ut_dos_calls; UT_ASSERT(input == (const union REGS *)output);
   UT_ASSERT_EQ_UINT(0x2b02, input->x.ax); UT_ASSERT_EQ_UINT(0x4445, input->x.cx);
   UT_ASSERT_EQ_UINT(0x5351, input->x.dx);
   output->x.bx = ut_desqview_present ? 0x1950 : 0;
   if(ut_desqview_present) output->x.dx = 0xa123;
   return 0;
}
#endif
#endif

static void reset_initialize(void)
{
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   ut_fill(&od_control, 0, sizeof(od_control));
#endif
   ut_fill(ut_buffer, 0x5a, sizeof(ut_buffer));
   ut_malloc_fail_call = 0; ut_malloc_calls = ut_clear_calls = 0;
   ut_caret_calls = 0; pScrnBuffer = NULL; bCaretOn = TRUE;
#ifdef ODPLAT_WIN32
   ut_fill(ut_display_buffer, 0xa5, sizeof(ut_display_buffer));
   pDisplayBuffer = NULL; bScreenPresentationActive = FALSE;
   bScreenDirty = TRUE; hwndScreenWindow = (HWND)1;
   ut_free_calls = ut_console_calls = ut_mutex_calls = 0;
   ut_console_result = TRUE;
   ut_subsystem = kODWindowsSubsystemGUI;
   ut_mutex_result = kODRCSuccess;
   od_control.baud = 9600;
   od_control.od_silent_mode = FALSE;
#endif
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_NIX)
   pAllocatedBufferMemory = NULL;
#endif
#ifdef ODPLAT_DOS32
   ut_video_mode = 3; ut_video_page = 0;
   ut_bios_query_calls = ut_bios_set_calls = 0;
#endif
#if defined(ODPLAT_DOS) && !defined(ODPLAT_DOS32) && defined(__WATCOMC__)
   ut_desqview_present = FALSE; ut_dos_calls = 0;
#endif
}

static void assert_common_success_state(void)
{
   UT_ASSERT_EQ_UINT(0, btLeftBoundary); UT_ASSERT_EQ_UINT(79, btRightBoundary);
   UT_ASSERT_EQ_UINT(0, btTopBoundary); UT_ASSERT_EQ_UINT(24, btBottomBoundary);
   UT_ASSERT_EQ_UINT(0x07, btCurrentAttribute); UT_ASSERT(bScrollEnabled);
   UT_ASSERT_EQ_UINT(1, ut_caret_calls);
}

static void reports_allocation_failure_for_a_memory_backed_screen(void)
{
   reset_initialize(); ut_malloc_fail_call = 1;
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   od_control.od_silent_mode = TRUE;
#endif
   UT_ASSERT_EQ_INT(kODRCNoMemory, utt_ODScrnInitialize());
   UT_ASSERT_EQ_UINT(1, ut_malloc_calls); UT_ASSERT_EQ_UINT(0, ut_clear_calls);
   UT_ASSERT_EQ_UINT(0, ut_caret_calls);
}

#ifdef ODPLAT_WIN32
static void releases_the_owner_buffer_when_display_allocation_fails(void)
{
   reset_initialize(); ut_malloc_fail_call = 2;
   UT_ASSERT_EQ_INT(kODRCNoMemory, utt_ODScrnInitialize());
   UT_ASSERT_EQ_UINT(2, ut_malloc_calls); UT_ASSERT_EQ_UINT(1, ut_free_calls);
   UT_ASSERT_EQ_PTR(NULL, pScrnBuffer); UT_ASSERT_EQ_UINT(0, ut_mutex_calls);
}

static void releases_both_buffers_when_mutex_initialization_fails(void)
{
   reset_initialize(); ut_mutex_result = kODRCGeneralFailure;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure, utt_ODScrnInitialize());
   UT_ASSERT_EQ_UINT(2, ut_malloc_calls); UT_ASSERT_EQ_UINT(2, ut_free_calls);
   UT_ASSERT_EQ_PTR(NULL, pScrnBuffer); UT_ASSERT_EQ_PTR(NULL, pDisplayBuffer);
   UT_ASSERT_EQ_UINT(1, ut_mutex_calls);
}

static void initializes_or_rejects_a_requested_console(void)
{
   reset_initialize();
   ut_subsystem = kODWindowsSubsystemConsole;
   od_control.od_silent_mode = FALSE;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODScrnInitialize());
   UT_ASSERT_EQ_UINT(1, ut_console_calls);
   UT_ASSERT_EQ_UINT(1, ut_malloc_calls);
   UT_ASSERT_EQ_UINT(0, ut_mutex_calls);
   UT_ASSERT_NULL(pDisplayBuffer);

   reset_initialize();
   ut_subsystem = kODWindowsSubsystemConsole;
   od_control.baud = 0;
   od_control.od_silent_mode = FALSE;
   ut_console_result = FALSE;
   UT_ASSERT_EQ_INT(kODRCGeneralFailure, utt_ODScrnInitialize());
   UT_ASSERT_EQ_UINT(1, ut_console_calls);
   UT_ASSERT_EQ_UINT(1, ut_free_calls);
   UT_ASSERT_NULL(pScrnBuffer);
   UT_ASSERT_EQ_INT(FALSE, bScreenPresentationActive);
}

static void remote_console_failure_falls_back_to_silent_mode(void)
{
   reset_initialize();
   ut_subsystem = kODWindowsSubsystemConsole;
   ut_console_result = FALSE;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODScrnInitialize());
   UT_ASSERT_EQ_UINT(1, ut_console_calls);
   UT_ASSERT_EQ_INT(TRUE, od_control.od_silent_mode);
   UT_ASSERT_EQ_UINT(0, ut_free_calls);
   UT_ASSERT_EQ_PTR(ut_buffer, pScrnBuffer);
}

static void silent_console_mode_does_not_manage_a_console(void)
{
   reset_initialize();
   ut_subsystem = kODWindowsSubsystemConsole;
   od_control.od_silent_mode = TRUE;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODScrnInitialize());
   UT_ASSERT_EQ_UINT(0, ut_console_calls);
   UT_ASSERT_EQ_UINT(1, ut_malloc_calls);
   UT_ASSERT_EQ_UINT(0, ut_mutex_calls);
}
#endif

static void initializes_a_memory_backed_screen(void)
{
   reset_initialize();
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32)
   od_control.od_silent_mode = TRUE;
#endif
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODScrnInitialize());
   UT_ASSERT_EQ_PTR(ut_buffer, pScrnBuffer);
#ifdef ODPLAT_WIN32
   UT_ASSERT_EQ_PTR(ut_display_buffer, pDisplayBuffer);
   UT_ASSERT_EQ_UINT(2, ut_malloc_calls); UT_ASSERT_EQ_UINT(1, ut_mutex_calls);
   UT_ASSERT_EQ_INT(TRUE, bScreenPresentationActive);
   UT_ASSERT_EQ_INT(FALSE, bScreenDirty); UT_ASSERT_EQ_PTR(NULL, hwndScreenWindow);
   UT_ASSERT_EQ_INT(ut_buffer[0], ut_display_buffer[0]);
#endif
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_NIX)
   UT_ASSERT_EQ_PTR(ut_buffer, pAllocatedBufferMemory);
#endif
   UT_ASSERT_EQ_UINT(1, ut_clear_calls); assert_common_success_state();
}

#ifdef ODPLAT_DOS32
static void initialize_dos32_video(BYTE mode, BYTE page,
   DWORD expected_address, unsigned expected_sets, unsigned expected_clears)
{
   reset_initialize(); ut_video_mode = mode; ut_video_page = page;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODScrnInitialize());
   UT_ASSERT_EQ_UINT(expected_address, (DWORD)pScrnBuffer);
   UT_ASSERT_EQ_UINT(1, ut_bios_query_calls);
   UT_ASSERT_EQ_UINT(expected_sets, ut_bios_set_calls);
   UT_ASSERT_EQ_UINT(expected_clears, ut_clear_calls);
   assert_common_success_state();
}

static void selects_dos32_mono_and_colour_video_memory(void)
{
   initialize_dos32_video(0x07, 1,
      0x000b0000UL + (DWORD)SCREEN_BUFFER_SIZE, 0, 1);
   initialize_dos32_video(0x02, 1,
      0x000b8000UL + (DWORD)SCREEN_BUFFER_SIZE, 0, 1);
   initialize_dos32_video(0x03, 0, 0x000b8000UL, 0, 1);
}

static void changes_unsupported_dos32_video_modes_without_clearing(void)
{
   initialize_dos32_video(0x21, 2, 0x000b0000UL, 1, 0);
   initialize_dos32_video(0x01, 2, 0x000b8000UL, 1, 0);
}
#endif

#if defined(ODPLAT_DOS) && !defined(ODPLAT_DOS32)
static void initializes_supported_dos16_video_modes(void)
{
   reset_initialize(); set_video_mode(0x02);
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODScrnInitialize());
   UT_ASSERT_EQ_UINT(0xb800, wBufferSegment); UT_ASSERT_EQ_UINT(1, ut_clear_calls);
   assert_common_success_state();

   reset_initialize(); set_video_mode(0x03);
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODScrnInitialize());
   UT_ASSERT_EQ_UINT(0xb800, wBufferSegment); UT_ASSERT_EQ_UINT(1, ut_clear_calls);
   assert_common_success_state();

   reset_initialize(); set_video_mode(0x07);
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODScrnInitialize());
   UT_ASSERT_EQ_UINT(0xb000, wBufferSegment); UT_ASSERT_EQ_UINT(1, ut_clear_calls);
   assert_common_success_state();
}

static void initializes_a_nonzero_dos16_display_page(void)
{
   reset_initialize(); set_video_mode(0x03); set_video_page(1);
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODScrnInitialize());
   UT_ASSERT_EQ_UINT(0xb800 + SCREEN_BUFFER_SEGMENT_SIZE, wBufferSegment);
   UT_ASSERT_EQ_UINT(1, btDisplayPage); UT_ASSERT_EQ_UINT(1, ut_clear_calls);
   assert_common_success_state(); set_video_page(0);
}

static void changes_dos16_mode_21_to_monochrome_without_clearing(void)
{
   reset_initialize(); set_video_mode(0x03); force_reported_video_mode(0x21);
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODScrnInitialize());
   UT_ASSERT_EQ_UINT(0xb000, wBufferSegment); UT_ASSERT_EQ_UINT(0, ut_clear_calls);
   assert_common_success_state();
}

static void changes_an_unsupported_dos16_mode_without_clearing(void)
{
   reset_initialize(); set_video_mode(0x01);
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODScrnInitialize());
   UT_ASSERT_EQ_UINT(0xb800, wBufferSegment); UT_ASSERT_EQ_UINT(0, ut_clear_calls);
   assert_common_success_state();
}

static void probes_and_uses_the_desqview_screen_when_available(void)
{
   reset_initialize(); set_video_mode(0x03); ODMultitasker = kMultitaskerDV;
#if defined(__WATCOMC__)
   ut_desqview_present = FALSE;
#endif
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODScrnInitialize());
   UT_ASSERT_EQ_UINT(0xb800, wBufferSegment);

#if defined(__WATCOMC__)
   UT_ASSERT_EQ_UINT(1, ut_dos_calls);
   reset_initialize(); set_video_mode(0x03); ODMultitasker = kMultitaskerDV;
   ut_desqview_present = TRUE;
   UT_ASSERT_EQ_INT(kODRCSuccess, utt_ODScrnInitialize());
   UT_ASSERT_EQ_UINT(1, ut_dos_calls); UT_ASSERT_EQ_UINT(0xa123, wBufferSegment);
#endif
}
#endif

static const UTTestCase ut_cases[] = {
   {"allocation failure", reports_allocation_failure_for_a_memory_backed_screen},
#ifdef ODPLAT_WIN32
   {"display allocation failure", releases_the_owner_buffer_when_display_allocation_fails},
   {"mutex failure", releases_both_buffers_when_mutex_initialization_fails},
   {"console availability", initializes_or_rejects_a_requested_console},
   {"remote console fallback", remote_console_failure_falls_back_to_silent_mode},
   {"silent console", silent_console_mode_does_not_manage_a_console},
#endif
   {"memory screen", initializes_a_memory_backed_screen},
#ifdef ODPLAT_DOS32
   {"dos32 video", selects_dos32_mono_and_colour_video_memory},
   {"dos32 mode change", changes_unsupported_dos32_video_modes_without_clearing},
#endif
#if defined(ODPLAT_DOS) && !defined(ODPLAT_DOS32)
   {"dos16 video", initializes_supported_dos16_video_modes},
   {"dos16 page", initializes_a_nonzero_dos16_display_page},
   {"dos16 mono change", changes_dos16_mode_21_to_monochrome_without_clearing},
   {"dos16 mode change", changes_an_unsupported_dos16_mode_without_clearing},
   {"desqview", probes_and_uses_the_desqview_screen_when_available},
#endif
};
