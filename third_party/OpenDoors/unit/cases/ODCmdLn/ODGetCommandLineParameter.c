#define UT_CUSTOM_MOCK_strcasecmp
#define UT_CUSTOM_MOCK_stricmp

static int ut_casecmp(const char *left, const char *right)
{
   unsigned char l;
   unsigned char r;
   do
   {
      l = (unsigned char)*left++;
      r = (unsigned char)*right++;
      if(l >= 'a' && l <= 'z') l = (unsigned char)(l - ('a' - 'A'));
      if(r >= 'a' && r <= 'z') r = (unsigned char)(r - ('a' - 'A'));
      if(l != r) return (int)l - (int)r;
   } while(l != 0);
   return 0;
}

#ifdef ODPLAT_NIX
int utm_strcasecmp(const char *left, const char *right)
{
   return ut_casecmp(left, right);
}
#else
int utm_stricmp(const char *left, const char *right)
{
   return ut_casecmp(left, right);
}
#endif

typedef struct ParameterExample
{
   const char *text;
   tCommandLineParameter parameter;
} ParameterExample;

static void recognizes_every_standard_name_and_alias(void)
{
   static const ParameterExample examples[] = {
      {"-C", kParamConfigFile}, {"/CONFIG", kParamConfigFile},
      {"-CONFIGFILE", kParamConfigFile}, {"-CFGFILE", kParamConfigFile},
      {"-CFG", kParamConfigFile}, {"-L", kParamLocal},
      {"-LOCAL", kParamLocal}, {"-B", kParamBPS}, {"-BPS", kParamBPS},
      {"-BAUD", kParamBPS}, {"-P", kParamPort}, {"-PORT", kParamPort},
      {"-N", kParamNode}, {"-NODE", kParamNode}, {"-?", kParamHelp},
      {"-H", kParamHelp}, {"-HELP", kParamHelp},
      {"-PERSONALITY", kParamPersonality}, {"-MAXTIME", kParamMaxTime},
      {"-ADDRESS", kParamAddress}, {"-IRQ", kParamIRQ},
      {"-NOFOSSIL", kParamNoFOSSIL}, {"-NOFIFO", kParamNoFIFO},
      {"-DROPFILE", kParamDropFile}, {"-D", kParamDropFile},
      {"-USERNAME", kParamUserName}, {"-TIMELEFT", kParamTimeLeft},
      {"-SECURITY", kParamSecurity}, {"-LOCATION", kParamLocation},
      {"-GRAPHICS", kParamGraphics}, {"-G", kParamGraphics},
      {"-BBSNAME", kParamBBSName}, {"-HANDLE", kParamPortHandle},
      {"-SOCKET", kParamSocketDescriptor}, {"-SILENT", kParamSilentMode},
      {"-CP437UTF8", kParamCP436UTF8}
   };
   unsigned index;
   char argument[24];
   for(index = 0; index < sizeof(examples) / sizeof(examples[0]); ++index)
   {
      strcpy(argument, examples[index].text);
      UT_ASSERT_EQ_INT(examples[index].parameter,
         utt_ODGetCommandLineParameter(argument));
   }
}

static void distinguishes_values_and_unknown_switches(void)
{
   char value[] = "ordinary";
   char unknown[] = "-UNKNOWN";
   char slash[] = "/unknown";
   UT_ASSERT_EQ_INT(kParamOption, utt_ODGetCommandLineParameter(value));
   UT_ASSERT_EQ_INT(kParamUnknown, utt_ODGetCommandLineParameter(unknown));
   UT_ASSERT_EQ_INT(kParamUnknown, utt_ODGetCommandLineParameter(slash));
}

static const UTTestCase ut_cases[] = {
   {"standard parameters", recognizes_every_standard_name_and_alias},
   {"values and unknown switches", distinguishes_values_and_unknown_switches}
};
