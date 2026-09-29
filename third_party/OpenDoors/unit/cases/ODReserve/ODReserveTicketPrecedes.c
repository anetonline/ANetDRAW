static void orders_tickets_then_record_indices(void)
{
   dwODReserveTicketHigh = 2;
   dwODReserveTicketLow = 5;
   dwODReserveRecord = 4;
   UT_ASSERT(utt_ODReserveTicketPrecedes(1, 99, 99));
   UT_ASSERT(utt_ODReserveTicketPrecedes(2, 4, 99));
   UT_ASSERT(utt_ODReserveTicketPrecedes(2, 5, 3));
   UT_ASSERT(!utt_ODReserveTicketPrecedes(3, 0, 0));
   UT_ASSERT(!utt_ODReserveTicketPrecedes(2, 6, 0));
   UT_ASSERT(!utt_ODReserveTicketPrecedes(2, 5, 4));
   UT_ASSERT(!utt_ODReserveTicketPrecedes(2, 5, 5));
}
static const UTTestCase ut_cases[] = {
   {"order", orders_tickets_then_record_indices}
};
