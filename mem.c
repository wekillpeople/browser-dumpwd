#include <Windows.h>
#include <stdio.h>

#include "mem.h"

// normal functions

PVOID talloc(__in DWORD dwSize)
{
	PBYTE pMem = (PBYTE)malloc(dwSize);
	RtlSecureZeroMemory(pMem, dwSize);
	return(pMem);
}

VOID tfree(__in PVOID pMem)
{
	if (pMem)
		free(pMem);
}

// secure funcions

PVOID talloc_s(__in size_t dwSize)
{

	if (dwSize == 0)
		return NULL;

	LPBYTE pMem = (LPBYTE)malloc(dwSize);
	SecureZeroMemory(pMem, dwSize);

	return pMem;
}

VOID tfree_s(__in PVOID pMem)
{
	if (pMem)
	{
		free(pMem);
		pMem = NULL;
	}

}