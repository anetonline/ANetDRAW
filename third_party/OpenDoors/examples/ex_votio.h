#ifndef EX_VOTIO_H
#define EX_VOTIO_H

#include <stdio.h>

#include "OpenDoor.h"

#define MAX_QUESTIONS             200
#define MAX_USERS                 30000
#define MAX_ANSWERS               15
#define QUESTION_STR_SIZE         71
#define ANSWER_STR_SIZE           31

#define VOTE_USER_RECORD_SIZE     236
#define VOTE_QUESTION_RECORD_SIZE 664

typedef struct
{
   char szUserName[36];
   BYTE bVotedOnQuestion[MAX_QUESTIONS];
} tUserRecord;

typedef struct
{
   char szQuestion[72];
   char aszAnswer[MAX_ANSWERS][32];
   INT32 nTotalAnswers;
   DWORD auVotesForAnswer[MAX_ANSWERS];
   DWORD uTotalVotes;
   DWORD bCanAddAnswers;
   char szCreatorName[36];
   DWORD uCreationTime;
} tQuestionRecord;

typedef enum
{
   kVoteIOError = -1,
   kVoteIOEnd = 0,
   kVoteIOSuccess = 1,
   kVoteIOInvalid = 2
} tVoteIOResult;

typedef struct
{
   FILE *pfFile;
   BOOL bReserved;
   char szError[160];
} tVoteFile;

BOOL VoteUserRecordEncode(unsigned char *pachRecord,
   const tUserRecord *pUser);
tVoteIOResult VoteUserRecordDecode(tUserRecord *pUser,
   const unsigned char *pachRecord);
BOOL VoteQuestionRecordEncode(unsigned char *pachRecord,
   const tQuestionRecord *pQuestion);
tVoteIOResult VoteQuestionRecordDecode(tQuestionRecord *pQuestion,
   const unsigned char *pachRecord);

tVoteIOResult VoteUserRecordRead(FILE *pfFile, tUserRecord *pUser);
BOOL VoteUserRecordWrite(FILE *pfFile, const tUserRecord *pUser);
BOOL VoteUserRecordSeek(FILE *pfFile, long nRecord);
BOOL VoteUserFileValidate(FILE *pfFile);
tVoteIOResult VoteQuestionRecordRead(FILE *pfFile,
   tQuestionRecord *pQuestion);
BOOL VoteQuestionRecordWrite(FILE *pfFile,
   const tQuestionRecord *pQuestion);
BOOL VoteQuestionRecordSeek(FILE *pfFile, long nRecord);
BOOL VoteQuestionFileValidate(FILE *pfFile);

BOOL VoteFileOpen(tVoteFile *pFile, const char *pszDataName,
   const char *pszReservationName, const char *pszMode,
   unsigned int nWaitSeconds);
BOOL VoteFileClose(tVoteFile *pFile);
const char *VoteFileError(const tVoteFile *pFile);

#endif
