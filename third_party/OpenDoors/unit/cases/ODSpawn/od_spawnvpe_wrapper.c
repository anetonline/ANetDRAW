#define UT_CUSTOM_MOCK_ODSpawnVPEInternal

static const char *ut_arguments[] = {"program", "argument", NULL};
static const char *ut_environment[] = {"NAME=value", NULL};
static unsigned ut_calls;

INT16 utm_ODSpawnVPEInternal(INT16 mode, const char *path,
   const char *const arguments[], const char *const environment[]
#ifdef ODPLAT_WIN32
   , BOOL quote_arguments
#endif
   )
{
   ++ut_calls;
   UT_ASSERT_EQ_INT(P_NOWAIT, mode);
   UT_ASSERT_EQ_INT(0, strcmp("program", path));
   UT_ASSERT_EQ_PTR(ut_arguments, arguments);
   UT_ASSERT_EQ_PTR(ut_environment, environment);
#ifdef ODPLAT_WIN32
   UT_ASSERT(quote_arguments);
#endif
   return(23);
}

static void delegates_to_the_internal_spawn_path(void)
{
   ut_calls = 0;
   UT_ASSERT_EQ_INT(23, utt_od_spawnvpe(P_NOWAIT, "program",
      ut_arguments, ut_environment));
   UT_ASSERT_EQ_UINT(1, ut_calls);
}

static const UTTestCase ut_cases[] = {
   {"delegate", delegates_to_the_internal_spawn_path}
};
