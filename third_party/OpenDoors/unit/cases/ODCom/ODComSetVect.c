#define UT_CUSTOM_MOCK_UTDosSetVect

static unsigned ut_set_calls;
static unsigned ut_vector;
static void (INTERRUPT far *ut_handler)(void);

static void INTERRUPT far ut_dummy_isr(void)
{
}

void utm_UTDosSetVect(unsigned vector,
   void (INTERRUPT far *handler)())
{
   ++ut_set_calls;
   ut_vector = vector;
   ut_handler = handler;
}

static void forwards_the_vector_and_handler(void)
{
#ifdef __TURBOC__
   void (INTERRUPT far *old_handler)(void);

   old_handler = getvect(0x7f);
   utt_ODComSetVect(0x7f, ut_dummy_isr);
   UT_ASSERT(getvect(0x7f) == ut_dummy_isr);
   setvect(0x7f, old_handler);
#else
   ut_set_calls = 0;
   ut_vector = 0;
   ut_handler = NULL;

   utt_ODComSetVect(0x7f, ut_dummy_isr);

   UT_ASSERT_EQ_UINT(1, ut_set_calls);
   UT_ASSERT_EQ_UINT(0x7f, ut_vector);
   UT_ASSERT(ut_handler == ut_dummy_isr);
#endif
}

static const UTTestCase ut_cases[] = {
   {"set vector", forwards_the_vector_and_handler}
};
