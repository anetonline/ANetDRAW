#define UT_CUSTOM_MOCK_pdef_pcboard
#ifdef ODPLAT_WIN32
static void ODPersonalityOpenDoors(BYTE operation) { (void)operation; }
static void ODPersonalityOneRow(BYTE operation) { (void)operation; }
static void ODPersonalityRemoteAccess(BYTE operation) { (void)operation; }
static void ODPersonalityWildcat(BYTE operation) { (void)operation; }
#endif
static BYTE ut_operation;
void ODCALL utm_pdef_pcboard(BYTE operation) { ut_operation = operation; }
static void forwards_operation(void)
{
   ut_operation = 0;
   utt_ODPersonalityPCBoard(20);
   UT_ASSERT_EQ_INT(20, ut_operation);
}
static const UTTestCase ut_cases[] = {{"forward", forwards_operation}};
