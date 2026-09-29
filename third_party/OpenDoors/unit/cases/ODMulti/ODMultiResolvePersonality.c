#ifdef ODPLAT_WIN32
#define UT_CUSTOM_MOCK_ODPersonalityOpenDoors
#define UT_CUSTOM_MOCK_ODPersonalityOneRow
#define UT_CUSTOM_MOCK_ODPersonalityPCBoard
#define UT_CUSTOM_MOCK_ODPersonalityRemoteAccess
#define UT_CUSTOM_MOCK_ODPersonalityWildcat

void utm_ODPersonalityOpenDoors(BYTE operation) { (void)operation; }
void utm_ODPersonalityOneRow(BYTE operation) { (void)operation; }
void utm_ODPersonalityPCBoard(BYTE operation) { (void)operation; }
void utm_ODPersonalityRemoteAccess(BYTE operation) { (void)operation; }
void utm_ODPersonalityWildcat(BYTE operation) { (void)operation; }
#else
void ODCALL pdef_opendoors(BYTE operation) { (void)operation; }
void ODCALL pdef_od_onerow(BYTE operation) { (void)operation; }
void ODCALL pdef_pcboard(BYTE operation) { (void)operation; }
void ODCALL pdef_ra(BYTE operation) { (void)operation; }
void ODCALL pdef_wildcat(BYTE operation) { (void)operation; }
#endif

#ifdef ODPLAT_DOS32
#define UT_PERSONALITY_CALL ODCALL
#else
#define UT_PERSONALITY_CALL
#endif

static void UT_PERSONALITY_CALL ut_custom_personality(BYTE operation)
{ (void)operation; }

static void preserves_public_callbacks(void)
{
   UT_ASSERT_EQ_PTR(ut_custom_personality,
      utt_ODMultiResolvePersonality(ut_custom_personality));
}

#ifdef ODPLAT_WIN32
static void adapts_every_builtin(void)
{
   UT_ASSERT_EQ_PTR(utm_ODPersonalityOpenDoors,
      utt_ODMultiResolvePersonality(NULL));
   UT_ASSERT_EQ_PTR(utm_ODPersonalityOpenDoors,
      utt_ODMultiResolvePersonality((OD_PERSONALITY_PROC *)utm_pdef_opendoors));
   UT_ASSERT_EQ_PTR(utm_ODPersonalityOneRow,
      utt_ODMultiResolvePersonality((OD_PERSONALITY_PROC *)utm_pdef_od_onerow));
   UT_ASSERT_EQ_PTR(utm_ODPersonalityRemoteAccess,
      utt_ODMultiResolvePersonality((OD_PERSONALITY_PROC *)utm_pdef_ra));
   UT_ASSERT_EQ_PTR(utm_ODPersonalityWildcat,
      utt_ODMultiResolvePersonality((OD_PERSONALITY_PROC *)utm_pdef_wildcat));
   UT_ASSERT_EQ_PTR(utm_ODPersonalityPCBoard,
      utt_ODMultiResolvePersonality((OD_PERSONALITY_PROC *)utm_pdef_pcboard));
}
#endif

static const UTTestCase ut_cases[] = {
   {"custom callback", preserves_public_callbacks},
#ifdef ODPLAT_WIN32
   {"built-in adapters", adapts_every_builtin}
#endif
};
