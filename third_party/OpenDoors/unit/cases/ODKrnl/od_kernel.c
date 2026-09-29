#ifndef UT_TURBO_SHARD
#define UT_TURBO_SHARD 0
#endif

#define UT_CUSTOM_MOCK_od_init
#define UT_CUSTOM_MOCK_ODSyncAPIEntry
#define UT_CUSTOM_MOCK_ODSyncAPIExit
#define UT_CUSTOM_MOCK_ODComCarrier
#define UT_CUSTOM_MOCK_ODComGetByte
#define UT_CUSTOM_MOCK_ODInQueueReserveEvent
#define UT_CUSTOM_MOCK_ODInQueueCancelReservedEvent
#define UT_CUSTOM_MOCK_ODInQueueCommitReservedEvent
#define UT_CUSTOM_MOCK_ODKrnlHandleReceivedChar
#define UT_CUSTOM_MOCK_ODKrnlForceOpenDoorsShutdown
#define UT_CUSTOM_MOCK_ODKrnlTimeUpdate
#define UT_CUSTOM_MOCK_ODTimerStart
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_WIN32)
#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODConsoleReadKey
#define UT_CUSTOM_MOCK_ODPlatGetWindowsSubsystem
#endif
#ifdef __WATCOMC__
#define UT_CUSTOM_MOCK__bios_keybrd
#endif
#define UT_CUSTOM_MOCK_ODKrnlHandleLocalKey
#define UT_CUSTOM_MOCK_od_disp_str
#define UT_CUSTOM_MOCK_getenv
#define UT_CUSTOM_MOCK_od_spawnvpe
#define UT_CUSTOM_MOCK_ODKrnlEndChatMode
#define UT_CUSTOM_MOCK_ODKrnlChatMode
#define UT_CUSTOM_MOCK_od_set_statusline
#define UT_CUSTOM_MOCK_time
#define UT_CUSTOM_MOCK_ODStoreTextInfo
#define UT_CUSTOM_MOCK_ODRestoreTextInfo
#define UT_CUSTOM_MOCK_ODScrnSetBoundary
#define UT_CUSTOM_MOCK_ODScrnEnableCaret
#endif

#ifdef ODPLAT_DOS32
#define UT_APPLICATION_CALLBACK ODCALL
#else
#define UT_APPLICATION_CALLBACK
#endif

static unsigned ut_init_calls;
static BOOL ut_init_succeeds;
static unsigned ut_entry_calls;
static unsigned ut_exit_calls;
static unsigned ut_exec_calls;
static BOOL ut_exec_ends_session;
static BOOL ut_exec_uninitializes_session;
static BOOL ut_carrier;
static unsigned ut_carrier_calls;
static char ut_remote_bytes[4];
static unsigned ut_remote_count;
static unsigned ut_remote_index;
static tODResult ut_reserve_result;
static unsigned ut_reserve_calls;
static unsigned ut_cancel_calls;
static unsigned ut_received_calls;
static char ut_received_bytes[4];
static unsigned ut_shutdown_calls;
static BYTE ut_shutdown_reason;
static unsigned ut_time_update_calls;
static BOOL ut_time_update_callbacks;
static unsigned ut_timer_calls;
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_WIN32)
#define UT_KERNEL_MAX_KEYS 16
static WORD ut_bios_keys[UT_KERNEL_MAX_KEYS];
static BYTE ut_bios_shifts[UT_KERNEL_MAX_KEYS];
static unsigned ut_bios_count;
static unsigned ut_bios_index;
static unsigned ut_local_key_calls;
static WORD ut_local_key;
static unsigned ut_local_callback_calls;
static WORD ut_local_callback_key;
static unsigned ut_log_calls;
static INT ut_log_events[4];
static unsigned ut_before_shell_callback_calls;
static unsigned ut_after_shell_callback_calls;
static unsigned ut_display_calls;
static unsigned ut_before_shell_calls;
static unsigned ut_after_shell_calls;
static char *ut_comspec;
static unsigned ut_spawn_calls;
static const char *ut_spawn_path;
static unsigned ut_chat_start_calls;
static unsigned ut_chat_end_calls;
static unsigned ut_status_calls;
static INT ut_status_value;
static time_t ut_now;
static unsigned ut_time_calls;
static unsigned ut_store_calls;
static unsigned ut_restore_calls;
static unsigned ut_boundary_calls;
static unsigned ut_caret_calls;
static BOOL ut_caret_values[2];
static unsigned ut_personality_calls;
static BYTE ut_personality_operations[4];
static unsigned ut_hot_callback_calls;
#ifdef ODPLAT_WIN32
static tODWindowsSubsystem ut_subsystem;
#endif
#endif

tODMilliSec ODMaxMSToWait;
tODInQueueHandle hODInputQueue;

void ODCALL utm_od_init(void)
{
   ++ut_init_calls;
   if(ut_init_succeeds)
      bODInitialized = TRUE;
}

void utm_ODSyncAPIEntry(void) { ++ut_entry_calls; }
void utm_ODSyncAPIExit(void) { ++ut_exit_calls; }
#ifdef ODPLAT_DOS32
static void ODCALL ut_kernel_exec(void)
{
   ++ut_exec_calls;
   if(ut_exec_ends_session)
      eODLifecycleState = kODLifecycleExitPending;
   if(ut_exec_uninitializes_session)
      bODInitialized = FALSE;
}
#else
static void ut_kernel_exec(void)
{
   ++ut_exec_calls;
   if(ut_exec_ends_session)
      eODLifecycleState = kODLifecycleExitPending;
   if(ut_exec_uninitializes_session)
      bODInitialized = FALSE;
}
#endif

tODResult utm_ODComCarrier(tPortHandle handle, BOOL *carrier)
{
   (void)handle;
   UT_ASSERT(carrier != NULL);
   ++ut_carrier_calls;
   *carrier = ut_carrier;
   return(kODRCSuccess);
}

tODResult utm_ODComGetByte(tPortHandle handle, char *value, BOOL wait)
{
   (void)handle;
   UT_ASSERT(value != NULL);
   UT_ASSERT(!wait);
   if(ut_remote_index == ut_remote_count)
      return(kODRCNothingWaiting);
   *value = ut_remote_bytes[ut_remote_index++];
   return(kODRCSuccess);
}

tODResult utm_ODInQueueReserveEvent(tODInQueueHandle queue)
{
   UT_ASSERT(queue == hODInputQueue);
   ++ut_reserve_calls;
   return(ut_reserve_result);
}

void utm_ODInQueueCancelReservedEvent(tODInQueueHandle queue)
{
   UT_ASSERT(queue == hODInputQueue);
   ++ut_cancel_calls;
}

tODResult utm_ODInQueueCommitReservedEvent(tODInQueueHandle queue,
   const tODInputEvent *event)
{
   UT_ASSERT(queue == hODInputQueue);
   UT_ASSERT(event != NULL);
   UT_ASSERT_EQ_INT(EVENT_CHARACTER, event->EventType);
   UT_ASSERT(event->bFromRemote);
   UT_ASSERT(ut_received_calls < sizeof(ut_received_bytes));
   ut_received_bytes[ut_received_calls++] = event->chKeyPress;
   return(kODRCSuccess);
}

void utm_ODKrnlHandleReceivedChar(char value, BOOL remote)
{
   UT_ASSERT(remote);
   UT_ASSERT(ut_received_calls < sizeof(ut_received_bytes));
   ut_received_bytes[ut_received_calls++] = value;
}

void utm_ODKrnlForceOpenDoorsShutdown(BYTE reason)
{
   ++ut_shutdown_calls;
   ut_shutdown_reason = reason;
}

BOOL utm_ODKrnlTimeUpdate(BOOL callbacks)
{
   ++ut_time_update_calls;
   ut_time_update_callbacks = callbacks;
   return(FALSE);
}

void utm_ODTimerStart(tODTimer *timer, tODMilliSec duration)
{
   UT_ASSERT_EQ_PTR(&RunKernelTimer, timer);
   UT_ASSERT_EQ_UINT(250, duration);
   ++ut_timer_calls;
}

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_WIN32)
#ifdef ODPLAT_WIN32
tODWindowsSubsystem utm_ODPlatGetWindowsSubsystem(void)
{
   return(ut_subsystem);
}

BOOL utm_ODConsoleReadKey(WORD *key, BYTE *shift)
{
   if(ut_bios_index >= ut_bios_count)
      return(FALSE);
   *key = ut_bios_keys[ut_bios_index];
   *shift = ut_bios_shifts[ut_bios_index++];
   return(TRUE);
}
#else /* !ODPLAT_WIN32 */
#if defined(__WATCOMC__)
unsigned short utm__bios_keybrd(unsigned command)
{
   if(command == _KEYBRD_READY)
      return((unsigned short)(ut_bios_index < ut_bios_count));
   UT_ASSERT(ut_bios_index < ut_bios_count);
   if(command == _KEYBRD_READ)
      return(ut_bios_keys[ut_bios_index]);
   UT_ASSERT_EQ_UINT(_KEYBRD_SHIFTSTATUS, command);
   return((unsigned short)ut_bios_shifts[ut_bios_index++]);
}
#endif /* __WATCOMC__ */
#endif /* !ODPLAT_WIN32 */

static void queue_bios_key(WORD key, BYTE shift)
{
   UT_ASSERT(ut_bios_count < UT_KERNEL_MAX_KEYS);
   ut_bios_keys[ut_bios_count] = key;
   ut_bios_shifts[ut_bios_count] = shift;
   ++ut_bios_count;
#ifdef __TURBOC__
   {
      BYTE result;
      ASM push ax
      ASM push bx
      ASM push es
      ASM mov ax, 0x0040
      ASM mov es, ax
      ASM mov bx, 0x0017
      ASM mov al, shift
      ASM mov es:[bx], al
      ASM pop es
      ASM pop bx
      ASM pop ax
      ASM mov ah, 5
      ASM mov cx, key
      ASM int 0x16
      ASM mov result, al
      UT_ASSERT_EQ_UINT(0, result);
   }
#endif
}

void utm_ODKrnlHandleLocalKey(WORD key)
{
   ++ut_local_key_calls;
   ut_local_key = key;
}

static void UT_APPLICATION_CALLBACK ut_local_input(INT16 key)
{
   ++ut_local_callback_calls;
   ut_local_callback_key = (WORD)key;
}

static BOOL ODCALL ut_log(INT event)
{
   UT_ASSERT(ut_log_calls < sizeof(ut_log_events) / sizeof(ut_log_events[0]));
   ut_log_events[ut_log_calls++] = event;
   return(TRUE);
}

static void UT_APPLICATION_CALLBACK ut_before_shell_callback(void)
{
   ++ut_before_shell_callback_calls;
   UT_ASSERT(bShellChatActive);
}

static void UT_APPLICATION_CALLBACK ut_after_shell_callback(void)
{
   ++ut_after_shell_callback_calls;
   UT_ASSERT(bShellChatActive);
}

void ODCALL utm_od_disp_str(const char *text)
{
   ++ut_display_calls;
   if(text == od_control.od_before_shell) ++ut_before_shell_calls;
   if(text == od_control.od_after_shell) ++ut_after_shell_calls;
}

char *utm_getenv(const char *name)
{
   UT_ASSERT(strcmp("COMSPEC", name) == 0);
   return(ut_comspec);
}

INT16 ODCALL utm_od_spawnvpe(INT16 mode, const char *path,
   const char *const arguments[], const char *const environment[])
{
   UT_ASSERT_EQ_INT(P_WAIT, mode);
   UT_ASSERT(arguments == NULL);
   UT_ASSERT(environment == NULL);
   UT_ASSERT(bIsShell);
   ++ut_spawn_calls;
   ut_spawn_path = path;
   return(0);
}

void utm_ODKrnlEndChatMode(void) { ++ut_chat_end_calls; }
void utm_ODKrnlChatMode(void) { ++ut_chat_start_calls; }

void ODCALL utm_od_set_statusline(INT value)
{
   ++ut_status_calls;
   ut_status_value = value;
}

time_t utm_time(time_t *storage)
{
   UT_ASSERT(storage == NULL);
   ++ut_time_calls;
   return(ut_now);
}

void utm_ODStoreTextInfo(void) { ++ut_store_calls; }
void utm_ODRestoreTextInfo(void) { ++ut_restore_calls; }
void utm_ODScrnSetBoundary(BYTE left, BYTE top, BYTE right, BYTE bottom)
{
   ++ut_boundary_calls;
   UT_ASSERT_EQ_UINT(1, left);
   UT_ASSERT_EQ_UINT(1, top);
   UT_ASSERT_EQ_UINT(80, right);
   UT_ASSERT_EQ_UINT(25, bottom);
}
void utm_ODScrnEnableCaret(BOOL enable)
{
   UT_ASSERT(ut_caret_calls < 2);
   ut_caret_values[ut_caret_calls++] = enable;
}

static void UT_APPLICATION_CALLBACK ut_personality(BYTE operation)
{
   UT_ASSERT(ut_personality_calls < sizeof(ut_personality_operations));
   ut_personality_operations[ut_personality_calls++] = operation;
}

static void UT_APPLICATION_CALLBACK ut_hot_callback(void)
{
   ++ut_hot_callback_calls;
}
#endif

static void reset_kernel(void)
{
   unsigned index;
   ut_init_calls = ut_entry_calls = ut_exit_calls = ut_exec_calls = 0;
   ut_init_succeeds = TRUE;
   ut_exec_ends_session = FALSE;
   ut_exec_uninitializes_session = FALSE;
   bODInitialized = TRUE;
   eODLifecycleState = kODLifecycleActive;
   bKernelActive = FALSE;
   od_control.od_ker_exec = NULL;
   ut_carrier = TRUE;
   ut_carrier_calls = 0;
   ut_remote_count = ut_remote_index = ut_received_calls = 0;
   ut_reserve_result = kODRCSuccess;
   ut_reserve_calls = 0;
   ut_cancel_calls = 0;
   ut_shutdown_calls = 0;
   ut_shutdown_reason = 0;
   ut_time_update_calls = ut_timer_calls = 0;
   ut_time_update_callbacks = FALSE;
   ODMaxMSToWait = 100;
   od_control.baud = 0;
   od_control.od_disable = 0;
   od_control.od_user_keyboard_on = TRUE;
   for(index = 0; index < sizeof(ut_remote_bytes); ++index)
      ut_remote_bytes[index] = ut_received_bytes[index] = 0;
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_WIN32)
   ut_bios_count = ut_bios_index = 0;
   ut_local_key_calls = ut_local_callback_calls = 0;
   ut_local_key = ut_local_callback_key = 0;
   ut_log_calls = 0;
   ut_before_shell_callback_calls = ut_after_shell_callback_calls = 0;
   ut_display_calls = ut_before_shell_calls = ut_after_shell_calls = 0;
   ut_comspec = NULL;
   ut_spawn_calls = 0;
   ut_spawn_path = NULL;
   ut_chat_start_calls = ut_chat_end_calls = 0;
   ut_status_calls = 0;
   ut_status_value = -1;
   ut_now = 100;
   ut_time_calls = 0;
   ut_store_calls = ut_restore_calls = ut_boundary_calls = ut_caret_calls = 0;
   ut_personality_calls = ut_hot_callback_calls = 0;
   nKrnlFuncPending = 0;
   bShellChatActive = FALSE;
   bIsShell = FALSE;
   nArrowUseCount = 0;
   bLastStatusSetting = FALSE;
   nNextStatusUpdateTime = 100;
   bForceStatusUpdate = FALSE;
   btCurrentStatusLine = 0;
   pfLogWrite = NULL;
   pfCurrentPersonality = ut_personality;
   od_control.od_silent_mode = TRUE;
   od_control.od_status_on = FALSE;
   od_control.od_update_status_now = FALSE;
   od_control.od_chat_active = FALSE;
   od_control.od_local_input = NULL;
   od_control.od_cbefore_shell = NULL;
   od_control.od_cafter_shell = NULL;
   od_control.od_before_shell = NULL;
   od_control.od_after_shell = NULL;
   od_control.od_num_keys = 0;
   od_control.user_timelimit = 10;
   od_control.user_security = 50;
   od_control.sysop_next = FALSE;
   od_control.key_hangup = 0x1101;
   od_control.key_drop2bbs = 0x1102;
   od_control.key_dosshell = 0x1103;
   od_control.key_chat = 0x1104;
   od_control.key_sysopnext = 0x1105;
   od_control.key_lockout = 0x1106;
   od_control.key_keyboardoff = 0x1107;
   od_control.key_moretime = 0x1108;
   od_control.key_lesstime = 0x1109;
   for(index = 0; index < 9; ++index)
      od_control.key_status[index] = (WORD)(0x1200 + index);
#ifdef ODPLAT_WIN32
   ut_subsystem = kODWindowsSubsystemConsole;
#endif
#endif
}

static void initializes_before_rejecting_a_recursive_call(void)
{
   reset_kernel();
   bODInitialized = FALSE;
   bKernelActive = TRUE;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_UINT(0, ut_entry_calls);

   reset_kernel();
   bODInitialized = FALSE;
   ut_init_succeeds = FALSE;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_init_calls);
   UT_ASSERT_EQ_INT(ERR_GENERALFAILURE, od_control.od_error);
   UT_ASSERT_EQ_UINT(0, ut_entry_calls);

   reset_kernel();
   bKernelActive = TRUE;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(0, ut_init_calls);
   UT_ASSERT_EQ_UINT(0, ut_entry_calls);
}

static void runs_the_optional_hook_inside_the_api_boundary(void)
{
   reset_kernel();
   od_control.od_ker_exec = ut_kernel_exec;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_entry_calls);
   UT_ASSERT_EQ_UINT(1, ut_exec_calls);
   UT_ASSERT_EQ_UINT(1, ut_exit_calls);
   UT_ASSERT(!bKernelActive);
   UT_ASSERT_EQ_UINT(1, ut_time_update_calls);
   UT_ASSERT(ut_time_update_callbacks);
   UT_ASSERT_EQ_UINT(1, ut_timer_calls);

   reset_kernel();
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(0, ut_exec_calls);

   reset_kernel();
   od_control.od_ker_exec = ut_kernel_exec;
   ut_exec_ends_session = TRUE;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_exec_calls);
   UT_ASSERT_EQ_UINT(0, ut_time_update_calls);
   UT_ASSERT_EQ_UINT(1, ut_exit_calls);
   UT_ASSERT(!bKernelActive);
}

static void continues_initialization_after_the_optional_hook(void)
{
   reset_kernel();
   eODLifecycleState = kODLifecycleInitializing;
   od_control.od_ker_exec = ut_kernel_exec;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(0, ut_exec_calls);
   UT_ASSERT_EQ_UINT(1, ut_time_update_calls);
   UT_ASSERT_EQ_UINT(1, ut_timer_calls);
   UT_ASSERT_EQ_UINT(1, ut_exit_calls);
   UT_ASSERT(!bKernelActive);

   reset_kernel();
   od_control.od_ker_exec = ut_kernel_exec;
   ut_exec_uninitializes_session = TRUE;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_exec_calls);
   UT_ASSERT_EQ_UINT(0, ut_time_update_calls);
   UT_ASSERT_EQ_UINT(1, ut_exit_calls);
   UT_ASSERT(!bKernelActive);
}

static void applies_carrier_detection_policy_in_remote_mode(void)
{
   reset_kernel();
   od_control.baud = 38400L;
   od_control.od_disable = DIS_CARRIERDETECT;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(0, ut_carrier_calls);
   UT_ASSERT_EQ_UINT(0, ut_shutdown_calls);

   reset_kernel();
   od_control.baud = 38400L;
   ut_carrier = TRUE;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_carrier_calls);
   UT_ASSERT_EQ_UINT(0, ut_shutdown_calls);

   reset_kernel();
   od_control.baud = 38400L;
   ut_carrier = FALSE;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_shutdown_calls);
   UT_ASSERT_EQ_UINT(ERRORLEVEL_NOCARRIER, ut_shutdown_reason);
}

static void drains_all_available_remote_input(void)
{
   reset_kernel();
   od_control.baud = 38400L;
   od_control.od_disable = DIS_CARRIERDETECT;
   ut_remote_bytes[0] = 'a';
   ut_remote_bytes[1] = 'b';
   ut_remote_count = 2;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(2, ut_received_calls);
   UT_ASSERT_EQ_INT('a', ut_received_bytes[0]);
   UT_ASSERT_EQ_INT('b', ut_received_bytes[1]);
   UT_ASSERT_EQ_UINT(3, ut_reserve_calls);
   UT_ASSERT_EQ_UINT(1, ut_cancel_calls);
   UT_ASSERT_EQ_UINT(0, ODMaxMSToWait);
}

static void leaves_remote_input_at_the_source_when_the_queue_is_full(void)
{
   reset_kernel();
   od_control.baud = 38400L;
   od_control.od_disable = DIS_CARRIERDETECT;
   ut_remote_bytes[0] = 'a';
   ut_remote_count = 1;
   ut_reserve_result = kODRCNoMemory;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_reserve_calls);
   UT_ASSERT_EQ_UINT(0, ut_remote_index);
   UT_ASSERT_EQ_UINT(0, ut_received_calls);
}

static void discards_remote_input_while_the_user_keyboard_is_disabled(void)
{
   reset_kernel();
   od_control.baud = 38400L;
   od_control.od_disable = DIS_CARRIERDETECT;
   od_control.od_user_keyboard_on = FALSE;
   ut_remote_bytes[0] = 'a';
   ut_remote_count = 1;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_remote_index);
   UT_ASSERT_EQ_UINT(0, ut_reserve_calls);
   UT_ASSERT_EQ_UINT(0, ut_received_calls);
   UT_ASSERT_EQ_UINT(0, ut_cancel_calls);
}

#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_WIN32)
static void prepare_keyboard(void)
{
   od_control.od_disable = 0;
   od_control.od_silent_mode = FALSE;
}

static void covers_pending_and_keyboard_suppression_policy(void)
{
   reset_kernel();
   od_control.od_disable = DIS_SYSOP_KEYS;
   od_control.od_silent_mode = FALSE;
   utt_od_kernel();

   reset_kernel();
   nKrnlFuncPending = 0x0002;
   utt_od_kernel();
   UT_ASSERT_EQ_INT(0x0002, nKrnlFuncPending);

   reset_kernel();
   nKrnlFuncPending = KERNEL_FUNC_CHATTOGGLE;
   bShellChatActive = TRUE;
   utt_od_kernel();
   UT_ASSERT_EQ_INT(KERNEL_FUNC_CHATTOGGLE, nKrnlFuncPending);

   reset_kernel();
   prepare_keyboard();
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(0, ut_bios_index);

   reset_kernel();
   nKrnlFuncPending = KERNEL_FUNC_CHATTOGGLE;
   utt_od_kernel();
   UT_ASSERT_EQ_INT(0, nKrnlFuncPending);
   UT_ASSERT_EQ_UINT(1, ut_chat_start_calls);
}

static void routes_arrow_keys_only_under_the_arrow_policy(void)
{
   reset_kernel();
   prepare_keyboard();
   queue_bios_key(0x4800, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_local_key_calls);

   reset_kernel();
   prepare_keyboard();
   nArrowUseCount = 1;
   queue_bios_key(0x4700, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_local_key_calls);

   reset_kernel();
   prepare_keyboard();
   nArrowUseCount = 1;
   queue_bios_key(0x4800, 2);
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_local_key_calls);

   reset_kernel();
   prepare_keyboard();
   nArrowUseCount = 1;
   od_control.od_local_input = ut_local_input;
   queue_bios_key(0x4800, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_local_callback_calls);
   UT_ASSERT_EQ_UINT(0x4800, ut_local_callback_key);
   UT_ASSERT_EQ_UINT(1, ut_local_key_calls);

   reset_kernel();
   prepare_keyboard();
   nArrowUseCount = 1;
   queue_bios_key(0x5000, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_local_key_calls);
}

static void handles_hangup_and_drop_keys(void)
{
   reset_kernel();
   prepare_keyboard();
   queue_bios_key(od_control.key_hangup, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_shutdown_calls);
   UT_ASSERT_EQ_UINT(ERRORLEVEL_HANGUP, ut_shutdown_reason);

   reset_kernel();
   prepare_keyboard();
   queue_bios_key(od_control.key_drop2bbs, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_shutdown_calls);
   UT_ASSERT_EQ_UINT(ERRORLEVEL_DROPTOBBS, ut_shutdown_reason);
}

static void runs_or_suppresses_the_dos_shell(void)
{
   reset_kernel();
   prepare_keyboard();
   bShellChatActive = TRUE;
   queue_bios_key(od_control.key_dosshell, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(0, ut_spawn_calls);

   reset_kernel();
   prepare_keyboard();
   pfLogWrite = ut_log;
   od_control.od_cbefore_shell = ut_before_shell_callback;
   od_control.od_cafter_shell = ut_after_shell_callback;
   od_control.od_before_shell = (char *)"before shell";
   od_control.od_after_shell = (char *)"after shell";
   ut_comspec = (char *)"C:\\DOS\\COMMAND.COM";
   queue_bios_key(od_control.key_dosshell, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(2, ut_log_calls);
   UT_ASSERT_EQ_INT(6, ut_log_events[0]);
   UT_ASSERT_EQ_INT(7, ut_log_events[1]);
   UT_ASSERT_EQ_UINT(1, ut_before_shell_callback_calls);
   UT_ASSERT_EQ_UINT(1, ut_after_shell_callback_calls);
   UT_ASSERT_EQ_UINT(1, ut_before_shell_calls);
   UT_ASSERT_EQ_UINT(1, ut_after_shell_calls);
   UT_ASSERT_EQ_UINT(1, ut_spawn_calls);
   UT_ASSERT_EQ_PTR(ut_comspec, ut_spawn_path);
   UT_ASSERT(!bIsShell);
   UT_ASSERT(!bShellChatActive);

   reset_kernel();
   prepare_keyboard();
   queue_bios_key(od_control.key_dosshell, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_spawn_calls);
   UT_ASSERT(strcmp("COMMAND.COM", ut_spawn_path) == 0);
}

static void toggles_or_defers_chat_according_to_shell_state(void)
{
   reset_kernel();
   prepare_keyboard();
   queue_bios_key(od_control.key_chat, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_chat_start_calls);

   reset_kernel();
   prepare_keyboard();
   bShellChatActive = TRUE;
   od_control.od_chat_active = TRUE;
   queue_bios_key(od_control.key_chat, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_chat_end_calls);

   reset_kernel();
   prepare_keyboard();
   bShellChatActive = TRUE;
   queue_bios_key(od_control.key_chat, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_INT(KERNEL_FUNC_CHATTOGGLE, nKrnlFuncPending);

   reset_kernel();
   prepare_keyboard();
   bShellChatActive = TRUE;
   nKrnlFuncPending = KERNEL_FUNC_CHATTOGGLE;
   queue_bios_key(od_control.key_chat, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_INT(0, nKrnlFuncPending);
}

static void handles_simple_sysop_control_keys(void)
{
   reset_kernel();
   prepare_keyboard();
   queue_bios_key(od_control.key_sysopnext, 0);
   utt_od_kernel();
   UT_ASSERT(od_control.sysop_next);

   reset_kernel();
   prepare_keyboard();
   od_control.od_chat_active = FALSE;
   queue_bios_key(27, 0);
   utt_od_kernel();
   UT_ASSERT(!od_control.od_chat_active);

   reset_kernel();
   prepare_keyboard();
   od_control.od_chat_active = TRUE;
   queue_bios_key(27, 0);
   utt_od_kernel();
   UT_ASSERT(!od_control.od_chat_active);

   reset_kernel();
   prepare_keyboard();
   queue_bios_key(od_control.key_lockout, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_INT(0, od_control.user_security);
   UT_ASSERT_EQ_UINT(ERRORLEVEL_HANGUP, ut_shutdown_reason);

   reset_kernel();
   prepare_keyboard();
   od_control.od_user_keyboard_on = FALSE;
   queue_bios_key(od_control.key_keyboardoff, 0);
   utt_od_kernel();
   UT_ASSERT(od_control.od_user_keyboard_on);
}

static void clamps_keyboard_time_adjustments(void)
{
   reset_kernel();
   prepare_keyboard();
   od_control.user_timelimit = 10;
   queue_bios_key(od_control.key_moretime, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_INT(11, od_control.user_timelimit);

   reset_kernel();
   prepare_keyboard();
   od_control.user_timelimit = 1440;
   queue_bios_key(od_control.key_moretime, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_INT(1440, od_control.user_timelimit);

   reset_kernel();
   prepare_keyboard();
   od_control.user_timelimit = 10;
   queue_bios_key(od_control.key_lesstime, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_INT(9, od_control.user_timelimit);

   reset_kernel();
   prepare_keyboard();
   od_control.user_timelimit = 0;
   queue_bios_key(od_control.key_lesstime, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_INT(0, od_control.user_timelimit);
}

static void selects_status_lines_under_the_documented_policy(void)
{
   reset_kernel();
   prepare_keyboard();
   od_control.od_status_on = TRUE;
   btCurrentStatusLine = 0;
   queue_bios_key(od_control.key_status[3], 0);
   utt_od_kernel();
   UT_ASSERT(ut_status_calls >= 1);

   reset_kernel();
   prepare_keyboard();
   od_control.od_status_on = TRUE;
   btCurrentStatusLine = 3;
   queue_bios_key(od_control.key_status[3], 0);
   utt_od_kernel();

   reset_kernel();
   prepare_keyboard();
   od_control.od_status_on = FALSE;
   btCurrentStatusLine = 0;
   queue_bios_key(od_control.key_status[3], 0);
   utt_od_kernel();
}

static void dispatches_hotkeys_or_routes_unmatched_local_input(void)
{
   reset_kernel();
   prepare_keyboard();
   od_control.od_num_keys = 2;
   od_control.od_hot_key[0] = 0x2101;
   od_control.od_hot_key[1] = 0x2102;
   od_control.od_hot_function[0] = NULL;
   queue_bios_key(0x2101, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_personality_calls);
   UT_ASSERT_EQ_UINT(21, ut_personality_operations[0]);
   UT_ASSERT_EQ_INT(0x2101, od_control.od_last_hot);
   UT_ASSERT_EQ_UINT(0, ut_local_key_calls);

   reset_kernel();
   prepare_keyboard();
   od_control.od_num_keys = 2;
   od_control.od_hot_key[0] = 0x2101;
   od_control.od_hot_key[1] = 0x2102;
   od_control.od_hot_function[1] = ut_hot_callback;
   queue_bios_key(0x2102, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_hot_callback_calls);
   UT_ASSERT_EQ_UINT(0, ut_local_key_calls);

   reset_kernel();
   prepare_keyboard();
   od_control.od_num_keys = 2;
   od_control.od_hot_key[0] = 0x2101;
   od_control.od_hot_key[1] = 0x2102;
   queue_bios_key(0x2103, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_local_key_calls);

   reset_kernel();
   prepare_keyboard();
   od_control.od_local_input = ut_local_input;
   queue_bios_key(0x2103, 0);
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_local_callback_calls);
   UT_ASSERT_EQ_UINT(1, ut_local_key_calls);
}

static void updates_status_only_when_requested_or_due(void)
{
   reset_kernel();
   od_control.od_status_on = TRUE;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_status_calls);
   UT_ASSERT_EQ_INT(0, ut_status_value);

   reset_kernel();
   od_control.od_update_status_now = TRUE;
   btCurrentStatusLine = 4;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_status_calls);
   UT_ASSERT_EQ_INT(4, ut_status_value);
   UT_ASSERT(!od_control.od_update_status_now);

   reset_kernel();
   nNextStatusUpdateTime = 99;
   od_control.od_status_on = TRUE;
   btCurrentStatusLine = 2;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(1, ut_store_calls);
   UT_ASSERT_EQ_UINT(1, ut_restore_calls);
   UT_ASSERT_EQ_UINT(1, ut_boundary_calls);
   UT_ASSERT_EQ_UINT(2, ut_caret_calls);
   UT_ASSERT(!ut_caret_values[0]);
   UT_ASSERT(ut_caret_values[1]);
   UT_ASSERT_EQ_UINT(1, ut_personality_calls);
   UT_ASSERT_EQ_UINT(12, ut_personality_operations[0]);

   reset_kernel();
   bForceStatusUpdate = TRUE;
   od_control.od_status_on = TRUE;
   btCurrentStatusLine = 8;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(0, ut_store_calls);
   UT_ASSERT(!bForceStatusUpdate);

   reset_kernel();
   bForceStatusUpdate = TRUE;
   od_control.od_status_on = FALSE;
   btCurrentStatusLine = 0;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(0, ut_store_calls);

}

#ifdef ODPLAT_WIN32
static void gui_mode_skips_the_cooperative_text_interface(void)
{
   reset_kernel();
   ut_subsystem = kODWindowsSubsystemGUI;
   od_control.od_status_on = TRUE;
   utt_od_kernel();
   UT_ASSERT_EQ_UINT(0, ut_bios_index);
   UT_ASSERT_EQ_UINT(0, ut_status_calls);
   UT_ASSERT_EQ_UINT(0, ut_personality_calls);
   UT_ASSERT_EQ_UINT(1, ut_time_update_calls);
}
#endif
#endif

static const UTTestCase ut_cases[] = {
   {"initialization and recursion", initializes_before_rejecting_a_recursive_call},
   {"API boundary", runs_the_optional_hook_inside_the_api_boundary},
   {"initialization hook", continues_initialization_after_the_optional_hook},
   {"carrier", applies_carrier_detection_policy_in_remote_mode},
   {"remote input", drains_all_available_remote_input},
   {"remote backpressure", leaves_remote_input_at_the_source_when_the_queue_is_full},
   {"disabled remote input", discards_remote_input_while_the_user_keyboard_is_disabled},
#if defined(ODPLAT_DOS) || defined(ODPLAT_DOS32) || defined(ODPLAT_WIN32)
   {"keyboard policy", covers_pending_and_keyboard_suppression_policy},
   {"arrow keys", routes_arrow_keys_only_under_the_arrow_policy},
   {"shutdown keys", handles_hangup_and_drop_keys},
   {"DOS shell", runs_or_suppresses_the_dos_shell},
   {"chat toggle", toggles_or_defers_chat_according_to_shell_state},
   {"sysop controls", handles_simple_sysop_control_keys},
   {"time controls", clamps_keyboard_time_adjustments},
   {"status keys", selects_status_lines_under_the_documented_policy},
   {"hotkeys", dispatches_hotkeys_or_routes_unmatched_local_input},
   {"status updates", updates_status_only_when_requested_or_due}
#ifdef ODPLAT_WIN32
   ,{"GUI skips text interface", gui_mode_skips_the_cooperative_text_interface}
#endif
#endif
};
