#define UT_CUSTOM_MOCK_GetModuleHandleA

typedef union
{
   void *alignment;
   BYTE bytes[sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS)];
} tImageBuffer;

static tImageBuffer ut_image;
static BOOL ut_have_image;

HMODULE WINAPI utm_GetModuleHandleA(LPCSTR name)
{
   UT_ASSERT_NULL(name);
   return(ut_have_image ? (HMODULE)ut_image.bytes : NULL);
}

static IMAGE_NT_HEADERS *make_valid_image(WORD subsystem)
{
   IMAGE_DOS_HEADER *dos_header;
   IMAGE_NT_HEADERS *nt_headers;

   memset(&ut_image, 0, sizeof(ut_image));
   ut_have_image = TRUE;
   dos_header = (IMAGE_DOS_HEADER *)ut_image.bytes;
   dos_header->e_magic = IMAGE_DOS_SIGNATURE;
   dos_header->e_lfanew = sizeof(IMAGE_DOS_HEADER);
   nt_headers = (IMAGE_NT_HEADERS *)(ut_image.bytes + dos_header->e_lfanew);
   nt_headers->Signature = IMAGE_NT_SIGNATURE;
   nt_headers->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);
   nt_headers->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR_MAGIC;
   nt_headers->OptionalHeader.Subsystem = subsystem;
   return(nt_headers);
}

static void recognizes_supported_subsystems(void)
{
   make_valid_image(IMAGE_SUBSYSTEM_WINDOWS_GUI);
   UT_ASSERT_EQ_INT(kODWindowsSubsystemGUI,
      utt_ODPlatGetWindowsSubsystem());
   make_valid_image(IMAGE_SUBSYSTEM_WINDOWS_CUI);
   UT_ASSERT_EQ_INT(kODWindowsSubsystemConsole,
      utt_ODPlatGetWindowsSubsystem());
   make_valid_image(IMAGE_SUBSYSTEM_NATIVE);
   UT_ASSERT_EQ_INT(kODWindowsSubsystemUnknown,
      utt_ODPlatGetWindowsSubsystem());
}

static void rejects_missing_or_invalid_dos_headers(void)
{
   IMAGE_DOS_HEADER *dos_header;

   ut_have_image = FALSE;
   UT_ASSERT_EQ_INT(kODWindowsSubsystemUnknown,
      utt_ODPlatGetWindowsSubsystem());
   make_valid_image(IMAGE_SUBSYSTEM_WINDOWS_GUI);
   dos_header = (IMAGE_DOS_HEADER *)ut_image.bytes;
   dos_header->e_magic = 0;
   UT_ASSERT_EQ_INT(kODWindowsSubsystemUnknown,
      utt_ODPlatGetWindowsSubsystem());
   make_valid_image(IMAGE_SUBSYSTEM_WINDOWS_GUI);
   dos_header->e_lfanew = 0;
   UT_ASSERT_EQ_INT(kODWindowsSubsystemUnknown,
      utt_ODPlatGetWindowsSubsystem());
}

static void rejects_each_invalid_nt_header_field(void)
{
   IMAGE_NT_HEADERS *nt_headers;

   nt_headers = make_valid_image(IMAGE_SUBSYSTEM_WINDOWS_GUI);
   nt_headers->Signature = 0;
   UT_ASSERT_EQ_INT(kODWindowsSubsystemUnknown,
      utt_ODPlatGetWindowsSubsystem());
   nt_headers = make_valid_image(IMAGE_SUBSYSTEM_WINDOWS_GUI);
   nt_headers->FileHeader.SizeOfOptionalHeader = 0;
   UT_ASSERT_EQ_INT(kODWindowsSubsystemUnknown,
      utt_ODPlatGetWindowsSubsystem());
   nt_headers = make_valid_image(IMAGE_SUBSYSTEM_WINDOWS_GUI);
   nt_headers->OptionalHeader.Magic = 0;
   UT_ASSERT_EQ_INT(kODWindowsSubsystemUnknown,
      utt_ODPlatGetWindowsSubsystem());
}

static const UTTestCase ut_cases[] = {
   {"supported subsystems", recognizes_supported_subsystems},
   {"DOS headers", rejects_missing_or_invalid_dos_headers},
   {"NT headers", rejects_each_invalid_nt_header_field}
};
