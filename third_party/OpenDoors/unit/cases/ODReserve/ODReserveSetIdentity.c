static void selects_local_and_node_identities(void)
{
   memset(&od_control, 0, sizeof(od_control));
   od_control.od_force_local = TRUE;
   od_control.od_node = 7;
   utt_ODReserveSetIdentity();
   UT_ASSERT_EQ_INT(OD_RESERVE_KIND_LOCAL, btODReserveKind);
   UT_ASSERT_EQ_INT(0, wODReserveNode);

   od_control.od_force_local = FALSE;
   od_control.baud = 38400;
   utt_ODReserveSetIdentity();
   UT_ASSERT_EQ_INT(OD_RESERVE_KIND_NODE, btODReserveKind);
   UT_ASSERT_EQ_INT(7, wODReserveNode);

#if !defined(ODPLAT_NIX)
   od_control.baud = 0;
   utt_ODReserveSetIdentity();
   UT_ASSERT_EQ_INT(OD_RESERVE_KIND_LOCAL, btODReserveKind);
#endif
}
static const UTTestCase ut_cases[] = {
   {"identity", selects_local_and_node_identities}
};
