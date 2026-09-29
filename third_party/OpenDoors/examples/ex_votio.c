#include "ex_votio.h"

#include <string.h>

static void VotePutDWORD(unsigned char *pachBuffer, DWORD uValue)
{
   pachBuffer[0] = (unsigned char)(uValue & 0xffUL);
   pachBuffer[1] = (unsigned char)((uValue >> 8) & 0xffUL);
   pachBuffer[2] = (unsigned char)((uValue >> 16) & 0xffUL);
   pachBuffer[3] = (unsigned char)((uValue >> 24) & 0xffUL);
}

static DWORD VoteGetDWORD(const unsigned char *pachBuffer)
{
   return((DWORD)pachBuffer[0]
      | ((DWORD)pachBuffer[1] << 8)
      | ((DWORD)pachBuffer[2] << 16)
      | ((DWORD)pachBuffer[3] << 24));
}

static BOOL VoteStringTerminated(const char *pszString, size_t nSize)
{
   return(memchr(pszString, '\0', nSize) != NULL);
}

BOOL VoteUserRecordEncode(unsigned char *pachRecord,
   const tUserRecord *pUser)
{
   int nQuestion;

   if(pachRecord == NULL || pUser == NULL
      || !VoteStringTerminated(pUser->szUserName,
         sizeof(pUser->szUserName)))
   {
      return(FALSE);
   }

   for(nQuestion = 0; nQuestion < MAX_QUESTIONS; ++nQuestion)
   {
      if(pUser->bVotedOnQuestion[nQuestion] > 1)
      {
         return(FALSE);
      }
   }

   memcpy(pachRecord, pUser->szUserName, sizeof(pUser->szUserName));
   memcpy(pachRecord + sizeof(pUser->szUserName),
      pUser->bVotedOnQuestion, sizeof(pUser->bVotedOnQuestion));
   return(TRUE);
}

tVoteIOResult VoteUserRecordDecode(tUserRecord *pUser,
   const unsigned char *pachRecord)
{
   int nQuestion;

   if(pUser == NULL || pachRecord == NULL)
   {
      return(kVoteIOError);
   }
   if(memchr(pachRecord, '\0', sizeof(pUser->szUserName)) == NULL)
   {
      return(kVoteIOInvalid);
   }
   for(nQuestion = 0; nQuestion < MAX_QUESTIONS; ++nQuestion)
   {
      if(pachRecord[sizeof(pUser->szUserName) + nQuestion] > 1)
      {
         return(kVoteIOInvalid);
      }
   }

   memcpy(pUser->szUserName, pachRecord, sizeof(pUser->szUserName));
   memcpy(pUser->bVotedOnQuestion,
      pachRecord + sizeof(pUser->szUserName),
      sizeof(pUser->bVotedOnQuestion));
   return(kVoteIOSuccess);
}

BOOL VoteQuestionRecordEncode(unsigned char *pachRecord,
   const tQuestionRecord *pQuestion)
{
   int nAnswer;

   if(pachRecord == NULL || pQuestion == NULL
      || pQuestion->nTotalAnswers < 0
      || pQuestion->nTotalAnswers > MAX_ANSWERS
      || pQuestion->bCanAddAnswers > 1
      || !VoteStringTerminated(pQuestion->szQuestion,
         sizeof(pQuestion->szQuestion))
      || !VoteStringTerminated(pQuestion->szCreatorName,
         sizeof(pQuestion->szCreatorName)))
   {
      return(FALSE);
   }
   for(nAnswer = 0; nAnswer < pQuestion->nTotalAnswers; ++nAnswer)
   {
      if(!VoteStringTerminated(pQuestion->aszAnswer[nAnswer],
         sizeof(pQuestion->aszAnswer[nAnswer])))
      {
         return(FALSE);
      }
   }

   memset(pachRecord, 0, VOTE_QUESTION_RECORD_SIZE);
   memcpy(pachRecord, pQuestion->szQuestion,
      sizeof(pQuestion->szQuestion));
   for(nAnswer = 0; nAnswer < pQuestion->nTotalAnswers; ++nAnswer)
   {
      memcpy(pachRecord + 72 + nAnswer * 32,
         pQuestion->aszAnswer[nAnswer],
         sizeof(pQuestion->aszAnswer[nAnswer]));
   }
   VotePutDWORD(pachRecord + 552, (DWORD)pQuestion->nTotalAnswers);
   for(nAnswer = 0; nAnswer < pQuestion->nTotalAnswers; ++nAnswer)
   {
      VotePutDWORD(pachRecord + 556 + nAnswer * 4,
         pQuestion->auVotesForAnswer[nAnswer]);
   }
   VotePutDWORD(pachRecord + 616, pQuestion->uTotalVotes);
   VotePutDWORD(pachRecord + 620, pQuestion->bCanAddAnswers);
   memcpy(pachRecord + 624, pQuestion->szCreatorName,
      sizeof(pQuestion->szCreatorName));
   VotePutDWORD(pachRecord + 660, pQuestion->uCreationTime);
   return(TRUE);
}

tVoteIOResult VoteQuestionRecordDecode(tQuestionRecord *pQuestion,
   const unsigned char *pachRecord)
{
   DWORD uAnswers;
   DWORD bCanAddAnswers;
   int nAnswer;

   if(pQuestion == NULL || pachRecord == NULL)
   {
      return(kVoteIOError);
   }
   uAnswers = VoteGetDWORD(pachRecord + 552);
   bCanAddAnswers = VoteGetDWORD(pachRecord + 620);
   if(uAnswers > MAX_ANSWERS || bCanAddAnswers > 1
      || memchr(pachRecord, '\0', 72) == NULL
      || memchr(pachRecord + 624, '\0', 36) == NULL)
   {
      return(kVoteIOInvalid);
   }
   for(nAnswer = 0; nAnswer < (int)uAnswers; ++nAnswer)
   {
      if(memchr(pachRecord + 72 + nAnswer * 32, '\0', 32) == NULL)
      {
         return(kVoteIOInvalid);
      }
   }

   memset(pQuestion, 0, sizeof(*pQuestion));
   memcpy(pQuestion->szQuestion, pachRecord, 72);
   for(nAnswer = 0; nAnswer < (int)uAnswers; ++nAnswer)
   {
      memcpy(pQuestion->aszAnswer[nAnswer],
         pachRecord + 72 + nAnswer * 32,
         sizeof(pQuestion->aszAnswer[nAnswer]));
   }
   pQuestion->nTotalAnswers = (INT32)uAnswers;
   for(nAnswer = 0; nAnswer < (int)uAnswers; ++nAnswer)
   {
      pQuestion->auVotesForAnswer[nAnswer]
         = VoteGetDWORD(pachRecord + 556 + nAnswer * 4);
   }
   pQuestion->uTotalVotes = VoteGetDWORD(pachRecord + 616);
   pQuestion->bCanAddAnswers = bCanAddAnswers;
   memcpy(pQuestion->szCreatorName, pachRecord + 624, 36);
   pQuestion->uCreationTime = VoteGetDWORD(pachRecord + 660);
   return(kVoteIOSuccess);
}

static tVoteIOResult VoteRecordRead(FILE *pfFile,
   unsigned char *pachRecord, size_t nSize)
{
   size_t nRead;

   nRead = fread(pachRecord, 1, nSize, pfFile);
   if(nRead == nSize)
   {
      return(kVoteIOSuccess);
   }
   if(nRead == 0 && feof(pfFile))
   {
      return(kVoteIOEnd);
   }
   return(ferror(pfFile) ? kVoteIOError : kVoteIOInvalid);
}

tVoteIOResult VoteUserRecordRead(FILE *pfFile, tUserRecord *pUser)
{
   unsigned char achRecord[VOTE_USER_RECORD_SIZE];
   tVoteIOResult Result;

   Result = VoteRecordRead(pfFile, achRecord, sizeof(achRecord));
   return(Result == kVoteIOSuccess
      ? VoteUserRecordDecode(pUser, achRecord) : Result);
}

BOOL VoteUserRecordWrite(FILE *pfFile, const tUserRecord *pUser)
{
   unsigned char achRecord[VOTE_USER_RECORD_SIZE];

   return(VoteUserRecordEncode(achRecord, pUser)
      && fwrite(achRecord, sizeof(achRecord), 1, pfFile) == 1);
}

BOOL VoteUserRecordSeek(FILE *pfFile, long nRecord)
{
   return(nRecord >= 0
      && fseek(pfFile, nRecord * VOTE_USER_RECORD_SIZE, SEEK_SET) == 0);
}

tVoteIOResult VoteQuestionRecordRead(FILE *pfFile,
   tQuestionRecord *pQuestion)
{
   unsigned char achRecord[VOTE_QUESTION_RECORD_SIZE];
   tVoteIOResult Result;

   Result = VoteRecordRead(pfFile, achRecord, sizeof(achRecord));
   return(Result == kVoteIOSuccess
      ? VoteQuestionRecordDecode(pQuestion, achRecord) : Result);
}

BOOL VoteQuestionRecordWrite(FILE *pfFile,
   const tQuestionRecord *pQuestion)
{
   unsigned char achRecord[VOTE_QUESTION_RECORD_SIZE];

   return(VoteQuestionRecordEncode(achRecord, pQuestion)
      && fwrite(achRecord, sizeof(achRecord), 1, pfFile) == 1);
}

BOOL VoteQuestionRecordSeek(FILE *pfFile, long nRecord)
{
   return(nRecord >= 0
      && fseek(pfFile, nRecord * VOTE_QUESTION_RECORD_SIZE, SEEK_SET) == 0);
}

static BOOL VoteFileValidationStart(FILE *pfFile, size_t nRecordSize,
   long nMaximumRecords, long *pnOriginal)
{
   long nLength;

   *pnOriginal = ftell(pfFile);
   if(*pnOriginal < 0 || fseek(pfFile, 0, SEEK_END) != 0
      || (nLength = ftell(pfFile)) < 0
      || nLength % (long)nRecordSize != 0
      || nLength / (long)nRecordSize > nMaximumRecords
      || fseek(pfFile, 0, SEEK_SET) != 0)
   {
      return(FALSE);
   }
   return(TRUE);
}

BOOL VoteUserFileValidate(FILE *pfFile)
{
   tUserRecord User;
   tVoteIOResult Result;
   long nOriginal;
   BOOL bValid;

   if(!VoteFileValidationStart(pfFile, VOTE_USER_RECORD_SIZE, MAX_USERS,
      &nOriginal))
   {
      return(FALSE);
   }
   while((Result = VoteUserRecordRead(pfFile, &User)) == kVoteIOSuccess)
   {
   }
   bValid = Result == kVoteIOEnd;
   clearerr(pfFile);
   return(fseek(pfFile, nOriginal, SEEK_SET) == 0 && bValid);
}

BOOL VoteQuestionFileValidate(FILE *pfFile)
{
   tQuestionRecord Question;
   tVoteIOResult Result;
   long nOriginal;
   BOOL bValid;

   if(!VoteFileValidationStart(pfFile, VOTE_QUESTION_RECORD_SIZE,
      MAX_QUESTIONS, &nOriginal))
   {
      return(FALSE);
   }
   while((Result = VoteQuestionRecordRead(pfFile, &Question))
      == kVoteIOSuccess)
   {
   }
   bValid = Result == kVoteIOEnd;
   clearerr(pfFile);
   return(fseek(pfFile, nOriginal, SEEK_SET) == 0 && bValid);
}

BOOL VoteFileOpen(tVoteFile *pFile, const char *pszDataName,
   const char *pszReservationName, const char *pszMode,
   unsigned int nWaitSeconds)
{
#ifdef MULTINODE_AWARE
   tODReserveResult ReserveResult;
#endif

   memset(pFile, 0, sizeof(*pFile));
#ifdef MULTINODE_AWARE
   if(!od_reserve_request(pszReservationName))
   {
      sprintf(pFile->szError, "Unable to request access to %s", pszDataName);
      return(FALSE);
   }
   pFile->bReserved = TRUE;
   ReserveResult = od_reserve_wait((tODMilliSec)nWaitSeconds * 1000UL);
   if(ReserveResult != OD_RESERVE_ACQUIRED)
   {
      (void)od_reserve_end();
      pFile->bReserved = FALSE;
      sprintf(pFile->szError, ReserveResult == OD_RESERVE_PENDING
         ? "Timed out waiting for %s" : "Unable to reserve %s", pszDataName);
      return(FALSE);
   }
#else
   (void)pszReservationName;
   (void)nWaitSeconds;
#endif

   pFile->pfFile = fopen(pszDataName, pszMode);
   if(pFile->pfFile == NULL)
   {
#ifdef MULTINODE_AWARE
      if(pFile->bReserved)
      {
         (void)od_reserve_end();
         pFile->bReserved = FALSE;
      }
#endif
      sprintf(pFile->szError, "Unable to open %s", pszDataName);
      return(FALSE);
   }
   return(TRUE);
}

BOOL VoteFileClose(tVoteFile *pFile)
{
   BOOL bSuccess = TRUE;

   if(pFile->pfFile != NULL)
   {
      if(fclose(pFile->pfFile) != 0)
      {
         bSuccess = FALSE;
      }
      pFile->pfFile = NULL;
   }
#ifdef MULTINODE_AWARE
   if(pFile->bReserved)
   {
      if(!od_reserve_end())
      {
         strcpy(pFile->szError, "Unable to end file reservation");
         bSuccess = FALSE;
      }
      pFile->bReserved = FALSE;
   }
#endif
   return(bSuccess);
}

const char *VoteFileError(const tVoteFile *pFile)
{
   return(pFile->szError);
}
