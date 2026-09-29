#define UT_CUSTOM_MOCK_ODStringCopy

static unsigned ut_copy_calls;
static char *ut_destinations[2];
static const char *ut_sources[2];
static INT ut_sizes[2];

void utm_ODStringCopy(char *destination, const char *source, INT size)
{
   unsigned index = ut_copy_calls++;
   UT_ASSERT(index < 2);
   ut_destinations[index] = destination;
   ut_sources[index] = source;
   ut_sizes[index] = size;
}

static void reset_user_fixture(void)
{
   memset(&od_control, 0, sizeof(od_control));
   memset(ut_destinations, 0, sizeof(ut_destinations));
   memset(ut_sources, 0, sizeof(ut_sources));
   memset(ut_sizes, 0, sizeof(ut_sizes));
   ut_copy_calls = 0;
}

static void ignores_a_missing_account_record(void)
{
   reset_user_fixture();
   utt_ODInitApplyUserInfo(NULL);
   UT_ASSERT_EQ_UINT(0, ut_copy_calls);
}

static void copies_each_available_account_name(void)
{
   struct passwd user;
   reset_user_fixture();
   memset(&user, 0, sizeof(user));
   user.pw_name = (char *)"login";
   user.pw_gecos = (char *)"Full Name";
   utt_ODInitApplyUserInfo(&user);
   UT_ASSERT_EQ_UINT(2, ut_copy_calls);
   UT_ASSERT_EQ_PTR(od_control.user_handle, ut_destinations[0]);
   UT_ASSERT_EQ_PTR(user.pw_name, ut_sources[0]);
   UT_ASSERT_EQ_UINT(sizeof(od_control.user_handle), ut_sizes[0]);
   UT_ASSERT_EQ_PTR(od_control.user_name, ut_destinations[1]);
   UT_ASSERT_EQ_PTR(user.pw_gecos, ut_sources[1]);
   UT_ASSERT_EQ_UINT(sizeof(od_control.user_name), ut_sizes[1]);
}

static void retains_fields_whose_source_name_is_missing(void)
{
   struct passwd user;
   reset_user_fixture();
   memset(&user, 0, sizeof(user));
   user.pw_gecos = (char *)"Only Gecos";
   utt_ODInitApplyUserInfo(&user);
   UT_ASSERT_EQ_UINT(1, ut_copy_calls);
   UT_ASSERT_EQ_PTR(od_control.user_name, ut_destinations[0]);

   reset_user_fixture();
   memset(&user, 0, sizeof(user));
   user.pw_name = (char *)"only-login";
   utt_ODInitApplyUserInfo(&user);
   UT_ASSERT_EQ_UINT(1, ut_copy_calls);
   UT_ASSERT_EQ_PTR(od_control.user_handle, ut_destinations[0]);
}

static const UTTestCase ut_cases[] = {
   {"missing account", ignores_a_missing_account_record},
   {"complete account", copies_each_available_account_name},
   {"partial account", retains_fields_whose_source_name_is_missing}
};
