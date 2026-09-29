static void applies_dos_selective_attribute_rules(void)
{
   UT_ASSERT_EQ_INT(TRUE, utt_ODDirAttributesMatch(DIR_ATTRIB_ARCH, 0));
   UT_ASSERT_EQ_INT(TRUE, utt_ODDirAttributesMatch(
      DIR_ATTRIB_HIDDEN | DIR_ATTRIB_DIREC,
      DIR_ATTRIB_HIDDEN | DIR_ATTRIB_DIREC));
   UT_ASSERT_EQ_INT(FALSE, utt_ODDirAttributesMatch(
      DIR_ATTRIB_HIDDEN | DIR_ATTRIB_SYSTEM,
      DIR_ATTRIB_HIDDEN));
}

static const UTTestCase ut_cases[] = {
   {"attributes", applies_dos_selective_attribute_rules}
};
