#ifndef __MEM__
#define __MEM__

PVOID talloc(__in DWORD dwSize);
VOID tfree(__in PVOID pMem);
PVOID talloc_s(__in size_t dwSize);
VOID tfree_s(__in PVOID pMem);

#endif