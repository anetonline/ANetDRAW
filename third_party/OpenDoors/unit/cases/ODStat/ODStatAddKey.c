static void appends_until_fixed_capacity(void)
{
   int index;
   od_control.od_num_keys = 0;
   utt_ODStatAddKey(0x1234);
   UT_ASSERT_EQ_INT(1, od_control.od_num_keys);
   UT_ASSERT_EQ_UINT(0x1234, od_control.od_hot_key[0]);

   od_control.od_num_keys = 15;
   utt_ODStatAddKey(0x5678);
   UT_ASSERT_EQ_INT(16, od_control.od_num_keys);
   UT_ASSERT_EQ_UINT(0x5678, od_control.od_hot_key[15]);

   for(index = 0; index < 16; ++index)
      od_control.od_hot_key[index] = (WORD)index;
   utt_ODStatAddKey(0x9abc);
   UT_ASSERT_EQ_INT(16, od_control.od_num_keys);
   UT_ASSERT_EQ_UINT(15, od_control.od_hot_key[15]);
}

static const UTTestCase ut_cases[] = {
   {"bounded hot-key append", appends_until_fixed_capacity}
};
