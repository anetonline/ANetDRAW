#define UT_CUSTOM_MOCK_od_set_personality
#define UT_CUSTOM_MOCK_ODSyncPublicCallAllowed

#ifdef ODPLAT_WIN32
static void ODPersonalityOpenDoors(BYTE operation) { (void)operation; }
static void ODPersonalityOneRow(BYTE operation) { (void)operation; }
static void ODPersonalityPCBoard(BYTE operation) { (void)operation; }
static void ODPersonalityRemoteAccess(BYTE operation) { (void)operation; }
static void ODPersonalityWildcat(BYTE operation) { (void)operation; }
#endif

static unsigned ut_set_calls;
static const char ut_test_name[] = "TEST";
static BOOL ut_public_call_allowed = TRUE;

BOOL utm_ODSyncPublicCallAllowed(void)
{
   return(ut_public_call_allowed);
}

#ifdef OD_PERSONALITY_SUPPORT
void ODCALL pdef_opendoors(BYTE operation) { (void)operation; }
void ODCALL pdef_od_onerow(BYTE operation) { (void)operation; }
void ODCALL pdef_ra(BYTE operation) { (void)operation; }
void ODCALL pdef_wildcat(BYTE operation) { (void)operation; }
void ODCALL pdef_pcboard(BYTE operation) { (void)operation; }
#endif

BOOL ODCALL utm_od_set_personality(const char *name)
{
   UT_ASSERT_EQ_PTR(ut_test_name, name);
   ++ut_set_calls;
   return TRUE;
}

static void installs_the_public_selector(void)
{
   pfSetPersonality = NULL;
   ut_set_calls = 0;
   utt_ODMPSEnable();
   UT_ASSERT_NOT_NULL(pfSetPersonality);
   UT_ASSERT((*pfSetPersonality)(ut_test_name));
   UT_ASSERT_EQ_UINT(1, ut_set_calls);
}

static void rejects_a_terminal_session(void)
{
   pfSetPersonality = NULL;
   ut_public_call_allowed = FALSE;
   utt_ODMPSEnable();
   UT_ASSERT_NULL(pfSetPersonality);
   ut_public_call_allowed = TRUE;
}

static const UTTestCase ut_cases[] = {
   {"selector hook", installs_the_public_selector},
   {"terminal session", rejects_a_terminal_session}
};
