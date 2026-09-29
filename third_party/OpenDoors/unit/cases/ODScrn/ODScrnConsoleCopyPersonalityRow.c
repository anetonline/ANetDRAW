#define UT_CUSTOM_MOCK_memcpy

static BYTE ut_screen[OD_SCREEN_WIDTH * OD_SCREEN_HEIGHT * 2];

void *utm_memcpy(void *destination, const void *source, size_t size)
{
   BYTE *out = (BYTE *)destination;
   const BYTE *in = (const BYTE *)source;
   size_t index;
   for(index = 0; index < size; ++index)
      out[index] = in[index];
   return(destination);
}

static void reset_screen(void)
{
   INT column;
   memset(ut_screen, 0, sizeof(ut_screen));
   pScrnBuffer = ut_screen;
   for(column = 0; column < OD_SCREEN_WIDTH; ++column)
   {
      ut_screen[column * 2] = (BYTE)('A' + column % 26);
      ut_screen[column * 2 + 1] = (BYTE)(column == 0 ? 0x1f :
         (column == OD_SCREEN_WIDTH - 1 ? 0x4e : 0x07));
   }
}

static void clips_personality_below_eighty_columns(void)
{
   BYTE destination[20];
   reset_screen(); memset(destination, 0, sizeof(destination));
   utt_ODScrnConsoleCopyPersonalityRow(destination, 10, 0);
   UT_ASSERT_EQ_INT('A', destination[0]);
   UT_ASSERT_EQ_UINT(0x1f, destination[1]);
   UT_ASSERT_EQ_INT('J', destination[18]);
}

static void centers_and_extends_edge_backgrounds(void)
{
   BYTE destination[204];
   reset_screen(); memset(destination, 0, sizeof(destination));
   utt_ODScrnConsoleCopyPersonalityRow(destination, 101, 0);
   UT_ASSERT_EQ_INT(' ', destination[0]);
   UT_ASSERT_EQ_UINT(0x10, destination[1]);
   UT_ASSERT_EQ_INT('A', destination[20]);
   UT_ASSERT_EQ_INT(' ', destination[200]);
   UT_ASSERT_EQ_UINT(0x40, destination[201]);
   UT_ASSERT_EQ_INT(0, destination[202]);
   UT_ASSERT_EQ_UINT(0, destination[203]);
}

static const UTTestCase ut_cases[] = {
   {"clip", clips_personality_below_eighty_columns},
   {"center and margins", centers_and_extends_edge_backgrounds}
};
