static void compares_wide_tickets(void)
{
   UT_ASSERT(utt_ODReserveTicketGreater(2, 0, 1, 99));
   UT_ASSERT(utt_ODReserveTicketGreater(2, 4, 2, 3));
   UT_ASSERT(!utt_ODReserveTicketGreater(1, 99, 2, 0));
   UT_ASSERT(!utt_ODReserveTicketGreater(2, 3, 2, 4));
   UT_ASSERT(!utt_ODReserveTicketGreater(2, 3, 2, 3));
}
static const UTTestCase ut_cases[] = {
   {"compare", compares_wide_tickets}
};
