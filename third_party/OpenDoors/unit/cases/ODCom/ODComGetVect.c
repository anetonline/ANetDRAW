#define UT_CUSTOM_MOCK_UTDosGetVect

static unsigned ut_get_calls;
static unsigned ut_vector;

static void INTERRUPT far ut_dummy_isr(void)
{
}

void (INTERRUPT far *utm_UTDosGetVect(unsigned vector))()
{
   ++ut_get_calls;
   ut_vector = vector;
   return ut_dummy_isr;
}

static void returns_the_handler_for_the_requested_vector(void)
{
   void (INTERRUPT far *handler)(void);

#ifdef __TURBOC__
   void (INTERRUPT far *old_handler)(void);

   old_handler = getvect(0xa5);
   setvect(0xa5, ut_dummy_isr);
   handler = utt_ODComGetVect(0xa5);
   UT_ASSERT(handler == ut_dummy_isr);
   setvect(0xa5, old_handler);
#else
   ut_get_calls = 0;
   ut_vector = 0;
   handler = utt_ODComGetVect(0xa5);

   UT_ASSERT_EQ_UINT(1, ut_get_calls);
   UT_ASSERT_EQ_UINT(0xa5, ut_vector);
   UT_ASSERT(handler == ut_dummy_isr);
#endif
}

static const UTTestCase ut_cases[] = {
   {"get vector", returns_the_handler_for_the_requested_vector}
};
