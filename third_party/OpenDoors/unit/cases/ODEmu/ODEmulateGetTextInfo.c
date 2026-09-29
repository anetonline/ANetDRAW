#define UT_CUSTOM_MOCK_ODSessionScreenIsEmulating
#define UT_CUSTOM_MOCK_ODSessionScreenGetInfo
#define UT_CUSTOM_MOCK_ODScrnGetTextInfo

static BOOL ut_emulating;
static unsigned ut_virtual_calls;
static unsigned ut_local_calls;

BOOL utm_ODSessionScreenIsEmulating(void)
{
   return ut_emulating;
}

void utm_ODSessionScreenGetInfo(tODVScreenInfo *info)
{
   ++ut_virtual_calls;
   memset(info, 0, sizeof(*info));
   info->winright = 132;
   info->winbottom = 60;
   info->curx = 99;
   info->scrolling = FALSE;
}

void utm_ODScrnGetTextInfo(tODScrnTextInfo *info)
{
   ++ut_local_calls;
   info->winleft = 2;
   info->wintop = 3;
   info->winright = 79;
   info->winbottom = 24;
   info->attribute = 0x1e;
   info->curx = 7;
   info->cury = 8;
}

static void obtains_virtual_screen_information_directly(void)
{
   tODVScreenInfo info;
   ut_emulating = TRUE;
   ut_virtual_calls = ut_local_calls = 0;
   memset(&info, 0xa5, sizeof(info));

   utt_ODEmulateGetTextInfo(&info);

   UT_ASSERT_EQ_UINT(1, ut_virtual_calls);
   UT_ASSERT_EQ_UINT(0, ut_local_calls);
   UT_ASSERT_EQ_INT(132, info.winright);
   UT_ASSERT_EQ_INT(60, info.winbottom);
   UT_ASSERT_EQ_INT(99, info.curx);
   UT_ASSERT_EQ_INT(FALSE, info.scrolling);
}

static void maps_legacy_local_screen_information(void)
{
   tODVScreenInfo info;
   ut_emulating = FALSE;
   ut_virtual_calls = ut_local_calls = 0;
   memset(&info, 0, sizeof(info));

   utt_ODEmulateGetTextInfo(&info);

   UT_ASSERT_EQ_UINT(0, ut_virtual_calls);
   UT_ASSERT_EQ_UINT(1, ut_local_calls);
   UT_ASSERT_EQ_INT(2, info.winleft);
   UT_ASSERT_EQ_INT(3, info.wintop);
   UT_ASSERT_EQ_INT(79, info.winright);
   UT_ASSERT_EQ_INT(24, info.winbottom);
   UT_ASSERT_EQ_INT(0x1e, info.attribute);
   UT_ASSERT_EQ_INT(7, info.curx);
   UT_ASSERT_EQ_INT(8, info.cury);
   UT_ASSERT_EQ_INT(TRUE, info.scrolling);
}

static const UTTestCase ut_cases[] = {
   {"virtual", obtains_virtual_screen_information_directly},
   {"legacy", maps_legacy_local_screen_information}
};
