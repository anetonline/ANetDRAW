#ifdef ODPLAT_DOS32
static void ODCALL marker_callback(void)
#else
static void marker_callback(void)
#endif
{
}

static void leaves_array_when_key_is_absent(void)
{
   od_control.od_num_keys = 2;
   od_control.od_hot_key[0] = 10;
   od_control.od_hot_key[1] = 20;
   utt_ODStatRemoveKey(30);
   UT_ASSERT_EQ_INT(2, od_control.od_num_keys);
   UT_ASSERT_EQ_UINT(10, od_control.od_hot_key[0]);
   UT_ASSERT_EQ_UINT(20, od_control.od_hot_key[1]);
}

static void removes_last_entry(void)
{
   od_control.od_num_keys = 2;
   od_control.od_hot_key[0] = 10;
   od_control.od_hot_key[1] = 20;
   od_control.od_hot_function[1] = marker_callback;
   utt_ODStatRemoveKey(20);
   UT_ASSERT_EQ_INT(1, od_control.od_num_keys);
   UT_ASSERT_EQ_UINT(0, od_control.od_hot_key[1]);
   UT_ASSERT(od_control.od_hot_function[1] == NULL);
}

static void fills_removed_slot_from_last_entry(void)
{
   od_control.od_num_keys = 3;
   od_control.od_hot_key[0] = 10;
   od_control.od_hot_key[1] = 20;
   od_control.od_hot_key[2] = 30;
   od_control.od_hot_function[2] = marker_callback;
   utt_ODStatRemoveKey(10);
   UT_ASSERT_EQ_INT(2, od_control.od_num_keys);
   UT_ASSERT_EQ_UINT(30, od_control.od_hot_key[0]);
   UT_ASSERT(od_control.od_hot_function[0] == marker_callback);
   UT_ASSERT_EQ_UINT(0, od_control.od_hot_key[2]);
   UT_ASSERT(od_control.od_hot_function[2] == NULL);
}

static const UTTestCase ut_cases[] = {
   {"absent hot key", leaves_array_when_key_is_absent},
   {"remove last hot key", removes_last_entry},
   {"remove interior hot key", fills_removed_slot_from_last_entry}
};
