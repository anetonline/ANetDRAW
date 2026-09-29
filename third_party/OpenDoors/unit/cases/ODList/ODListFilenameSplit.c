#define UT_CUSTOM_MOCK_memcpy
#define UT_CUSTOM_MOCK_strchr
#define UT_CUSTOM_MOCK_strlen
#define UT_CUSTOM_MOCK_strrchr

size_t utm_strlen(const char *text)
{
   size_t length = 0;
   while(text[length] != '\0') ++length;
   return length;
}

void *utm_memcpy(void *destination, const void *source, size_t count)
{
   size_t index;
   unsigned char *out = destination;
   const unsigned char *in = source;
   for(index = 0; index < count; ++index) out[index] = in[index];
   return destination;
}

char *utm_strchr(const char *text, int character)
{
   do {
      if(*text == (char)character) return (char *)text;
   } while(*text++ != '\0');
   return NULL;
}

char *utm_strrchr(const char *text, int character)
{
   const char *found = NULL;
   do {
      if(*text == (char)character) found = text;
   } while(*text++ != '\0');
   return (char *)found;
}

static INT split(const char *path, char *drive, size_t drive_size,
   char *dir, size_t dir_size, char *name, size_t name_size,
   char *extension, size_t extension_size)
{
   return utt_ODListFilenameSplit(path, drive, drive_size, dir, dir_size,
      name, name_size, extension, extension_size);
}

static void splits_a_complete_platform_path(void)
{
   char drive[4], dir[32], name[16], extension[8];
#ifdef ODPLAT_NIX
   INT result = split("dir/file.ext", drive, sizeof(drive), dir, sizeof(dir),
      name, sizeof(name), extension, sizeof(extension));
   UT_ASSERT_EQ_INT(DIRECTORY | FILENAME | EXTENSION, result);
   UT_ASSERT(strcmp(drive, "") == 0);
   UT_ASSERT(strcmp(dir, "dir/") == 0);
#else
   INT result = split("C:\\DIR\\FILE.EXT", drive, sizeof(drive), dir,
      sizeof(dir), name, sizeof(name), extension, sizeof(extension));
   UT_ASSERT_EQ_INT(DRIVE | DIRECTORY | FILENAME | EXTENSION, result);
   UT_ASSERT(strcmp(drive, "C:") == 0);
   UT_ASSERT(strcmp(dir, "\\DIR\\") == 0);
#endif
   UT_ASSERT(strcmp(name, "FILE") == 0 || strcmp(name, "file") == 0);
   UT_ASSERT(strcmp(extension, ".EXT") == 0 || strcmp(extension, ".ext") == 0);
}

static void handles_empty_plain_and_wildcard_names(void)
{
   char drive[4], dir[4], name[16], extension[8];
   INT result;
   result = split("", drive, sizeof(drive), dir, sizeof(dir), name,
      sizeof(name), extension, sizeof(extension));
   UT_ASSERT_EQ_INT(0, result);
   UT_ASSERT(strcmp(name, "") == 0);
   UT_ASSERT(strcmp(extension, "") == 0);

   result = split("PLAIN", drive, sizeof(drive), dir, sizeof(dir), name,
      sizeof(name), extension, sizeof(extension));
   UT_ASSERT_EQ_INT(FILENAME, result);
   UT_ASSERT(strcmp(name, "PLAIN") == 0);

   result = split("*.ZIP", drive, sizeof(drive), dir, sizeof(dir), name,
      sizeof(name), extension, sizeof(extension));
   UT_ASSERT((result & WILDCARDS) != 0);
   result = split("A?.ZIP", drive, sizeof(drive), dir, sizeof(dir), name,
      sizeof(name), extension, sizeof(extension));
   UT_ASSERT((result & WILDCARDS) != 0);
}

static void applies_dos_component_length_limits(void)
{
   char drive[4], dir[16], name[16], extension[8];
   INT result = split("123456789.12345", drive, sizeof(drive), dir,
      sizeof(dir), name, sizeof(name), extension, sizeof(extension));
   UT_ASSERT((result & (FILENAME | EXTENSION)) == (FILENAME | EXTENSION));
   UT_ASSERT(strcmp(name, "12345678") == 0);
   UT_ASSERT(strcmp(extension, ".123") == 0);

   result = split("123456789", drive, sizeof(drive), dir, sizeof(dir), name,
      sizeof(name), extension, sizeof(extension));
   UT_ASSERT_EQ_INT(FILENAME, result);
   UT_ASSERT(strcmp(name, "12345678") == 0);
}

static void rejects_each_undersized_output_component(void)
{
   char drive[4], dir[16], name[16], extension[8];
#ifndef ODPLAT_NIX
   UT_ASSERT_EQ_INT(-1, split("C:X", drive, 2, dir, sizeof(dir), name,
      sizeof(name), extension, sizeof(extension)));
#endif
   UT_ASSERT_EQ_INT(-1, split("DIR" DIRSEP_STR "X", drive, sizeof(drive),
      dir, 4, name, sizeof(name), extension, sizeof(extension)));
   UT_ASSERT_EQ_INT(-1, split("NAME", drive, sizeof(drive), dir, sizeof(dir),
      name, 4, extension, sizeof(extension)));
   UT_ASSERT_EQ_INT(-1, split("NAME.X", drive, sizeof(drive), dir, sizeof(dir),
      name, sizeof(name), extension, 2));
   UT_ASSERT_EQ_INT(-1, split("NAME.X", drive, sizeof(drive), dir, sizeof(dir),
      name, 4, extension, sizeof(extension)));
}

static void treats_colons_as_names_only_on_unix(void)
{
   char drive[4], dir[8], name[16], extension[8];
   INT result = split("A:B", drive, sizeof(drive), dir, sizeof(dir), name,
      sizeof(name), extension, sizeof(extension));
#ifdef ODPLAT_NIX
   UT_ASSERT_EQ_INT(FILENAME, result);
   UT_ASSERT(strcmp(drive, "") == 0);
   UT_ASSERT(strcmp(name, "A:B") == 0);
#else
   UT_ASSERT((result & DRIVE) != 0);
   UT_ASSERT(strcmp(drive, "A:") == 0);
   UT_ASSERT(strcmp(name, "B") == 0);

   result = split("LONG:C:X", drive, sizeof(drive), dir, sizeof(dir), name,
      sizeof(name), extension, sizeof(extension));
   UT_ASSERT((result & DRIVE) != 0);
   UT_ASSERT(strcmp(drive, "LO") == 0);
   UT_ASSERT(strcmp(name, "X") == 0);
#endif
}

static const UTTestCase ut_cases[] = {
   {"complete path", splits_a_complete_platform_path},
   {"simple names", handles_empty_plain_and_wildcard_names},
   {"component truncation", applies_dos_component_length_limits},
   {"output capacity", rejects_each_undersized_output_component},
   {"colon handling", treats_colons_as_names_only_on_unix}
};
