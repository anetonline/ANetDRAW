#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ex_votio.h"

static int nFailures;

#define CHECK(condition) do { \
   if(!(condition)) { \
      fprintf(stderr, "%s:%d: check failed: %s\n", \
         __FILE__, __LINE__, #condition); \
      ++nFailures; \
   } \
} while(0)

static void TestRecords(void)
{
   static unsigned char achUser[VOTE_USER_RECORD_SIZE];
   static unsigned char achQuestion[VOTE_QUESTION_RECORD_SIZE];
   static tUserRecord User;
   static tUserRecord DecodedUser;
   static tQuestionRecord Question;
   static tQuestionRecord DecodedQuestion;

   memset(&User, 0, sizeof(User));
   strcpy(User.szUserName, "Example User");
   User.bVotedOnQuestion[0] = 1;
   User.bVotedOnQuestion[MAX_QUESTIONS - 1] = 1;
   CHECK(VoteUserRecordEncode(achUser, &User));
   CHECK(achUser[0] == 'E');
   CHECK(achUser[35] == 0);
   CHECK(achUser[36] == 1);
   CHECK(achUser[VOTE_USER_RECORD_SIZE - 1] == 1);
   CHECK(VoteUserRecordDecode(&DecodedUser, achUser) == kVoteIOSuccess);
   CHECK(strcmp(DecodedUser.szUserName, User.szUserName) == 0);
   CHECK(DecodedUser.bVotedOnQuestion[MAX_QUESTIONS - 1] == 1);

   memset(&Question, 0, sizeof(Question));
   strcpy(Question.szQuestion, "Portable question?");
   strcpy(Question.aszAnswer[0], "Yes");
   strcpy(Question.aszAnswer[1], "No");
   Question.nTotalAnswers = 2;
   Question.auVotesForAnswer[0] = 0x12345678UL;
   Question.auVotesForAnswer[1] = 3;
   Question.uTotalVotes = 0x89abcdefUL;
   Question.bCanAddAnswers = 1;
   strcpy(Question.szCreatorName, "Creator");
   Question.uCreationTime = 0xffffffffUL;

   CHECK(VoteQuestionRecordEncode(achQuestion, &Question));
   CHECK(achQuestion[552] == 2);
   CHECK(achQuestion[556] == 0x78);
   CHECK(achQuestion[557] == 0x56);
   CHECK(achQuestion[558] == 0x34);
   CHECK(achQuestion[559] == 0x12);
   CHECK(achQuestion[616] == 0xef);
   CHECK(achQuestion[617] == 0xcd);
   CHECK(achQuestion[618] == 0xab);
   CHECK(achQuestion[619] == 0x89);
   CHECK(achQuestion[660] == 0xff);
   CHECK(achQuestion[663] == 0xff);
   CHECK(VoteQuestionRecordDecode(&DecodedQuestion, achQuestion)
      == kVoteIOSuccess);
   CHECK(DecodedQuestion.nTotalAnswers == 2);
   CHECK(DecodedQuestion.auVotesForAnswer[0] == 0x12345678UL);
   CHECK(DecodedQuestion.uTotalVotes == 0x89abcdefUL);
   CHECK(DecodedQuestion.uCreationTime == 0xffffffffUL);

   Question.uCreationTime = 0x80000000UL;
   CHECK(VoteQuestionRecordEncode(achQuestion, &Question));
   CHECK(VoteQuestionRecordDecode(&DecodedQuestion, achQuestion)
      == kVoteIOSuccess);
   CHECK(DecodedQuestion.uCreationTime == 0x80000000UL);

   memset(Question.aszAnswer[2], 0xa5,
      sizeof(Question.aszAnswer[2]));
   CHECK(VoteQuestionRecordEncode(achQuestion, &Question));
   CHECK(achQuestion[72 + 2 * 32] == 0);
   memset(achQuestion + 72 + 2 * 32, 0xa5, 32);
   CHECK(VoteQuestionRecordDecode(&DecodedQuestion, achQuestion)
      == kVoteIOSuccess);

   achQuestion[552] = MAX_ANSWERS + 1;
   achQuestion[553] = 0;
   achQuestion[554] = 0;
   achQuestion[555] = 0;
   CHECK(VoteQuestionRecordDecode(&DecodedQuestion, achQuestion)
      == kVoteIOInvalid);
}

static void TestFiles(void)
{
   static char szDataName[] = "VTIO.DAT";
   static tVoteFile First;
   FILE *pfFile;
   static unsigned char achWrongRecord[672];

   remove(szDataName);
   CHECK(VoteFileOpen(&First, szDataName, "VoteUsers", "a+b", 0));
   CHECK(VoteFileClose(&First));

   CHECK(!VoteFileOpen(&First, "VTNONE/VTIO.DAT", "VoteUsers", "r+b",
      0));
   CHECK(strstr(VoteFileError(&First), "Unable to open") != NULL);

   memset(achWrongRecord, 0, sizeof(achWrongRecord));
   pfFile = fopen(szDataName, "w+b");
   CHECK(pfFile != NULL);
   if(pfFile != NULL)
   {
      CHECK(fwrite(achWrongRecord, sizeof(achWrongRecord), 1, pfFile) == 1);
      CHECK(!VoteQuestionFileValidate(pfFile));
      fclose(pfFile);
   }

   remove(szDataName);
}

int main(void)
{
   TestRecords();
   TestFiles();
   return(nFailures == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
