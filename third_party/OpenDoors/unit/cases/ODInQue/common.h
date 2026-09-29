static tInputQueueInfo ut_queue;
static tODInputEvent ut_events[4];
#ifdef OD_THREAD_SUPPORT
static char ut_semaphore_token;
#endif

static tODInQueueHandle ut_queue_handle(INT entries, INT input, INT output)
{
   memset(&ut_queue, 0, sizeof(ut_queue));
   memset(ut_events, 0, sizeof(ut_events));
   ut_queue.paEvents = ut_events;
   ut_queue.nQueueEntries = entries;
   ut_queue.nInIndex = input;
   ut_queue.nOutIndex = output;
   ut_queue.nReservedEntries = 0;
#ifdef OD_THREAD_SUPPORT
   ut_queue.hItemCountSemaphore =
      (tODSemaphoreHandle)(void *)&ut_semaphore_token;
#endif
   return ODPTR2HANDLE(&ut_queue, tInputQueueInfo);
}
