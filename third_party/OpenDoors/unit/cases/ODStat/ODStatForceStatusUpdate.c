#define UT_CUSTOM_MOCK_od_kernel

static int ut_kernel_calls;

void ODCALL utm_od_kernel(void)
{
   ++ut_kernel_calls;
   UT_ASSERT(bForceStatusUpdate);
}

static void forces_update_before_entering_kernel(void)
{
   bForceStatusUpdate = FALSE;
   ut_kernel_calls = 0;
   utt_ODStatForceStatusUpdate();
   UT_ASSERT(bForceStatusUpdate);
   UT_ASSERT_EQ_INT(1, ut_kernel_calls);
}

static const UTTestCase ut_cases[] = {
   {"force status update", forces_update_before_entering_kernel}
};
