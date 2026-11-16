// Import.cpp : finding older copies of DiskLED
//

#include "stdafx.h"
#include "Import.h"
#include "Helpers.h"
#include <tlhelp32.h>
#include <shlobj.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")


#define RUN_KEY				TEXT ("Software\\Microsoft\\Windows\\CurrentVersion\\Run")

// The entry of an older version for one copy of the program: "DiskLED - <path of the program>"
#define OLD_RUN_PREFIX		TEXT ("DiskLED - ")

#define PROGRAM_FILE		TEXT ("DiskLED.exe")


struct Candidate
{
	CString		sExe;
	CString		sRunName;
};

// For comparing paths
static CString PathKey (const CString& sPath)
{
	TCHAR	sFull[MAX_PATH]	= { 0 };
	DWORD	nLength			= GetFullPathName (sPath, MAX_PATH, sFull, NULL);
	CString	sKey			= (nLength > 0 && nLength < MAX_PATH) ? CString (sFull) : sPath;

	sKey.MakeLower ();

	return sKey;
}

static CString GetFolder (const CString& sPath)
{
	return sPath.Left (sPath.ReverseFind ('\\'));
}

static CString GetLeaf (const CString& sPath)
{
	return sPath.Mid (sPath.ReverseFind ('\\') + 1);
}

// The program in a command line, e.g. "C:\Program Files\DiskLED\DiskLED.exe" /option
static CString ProgramFromCommand (const CString& sCommand)
{
	CString	sText	= sCommand;

	sText.Trim ();
	if (sText.IsEmpty ())
	{
		return sText;
	}
	if (sText[0] == '"')
	{
		int	nEnd	= sText.Find ('"', 1);

		return (nEnd > 0) ? sText.Mid (1, nEnd - 1) : sText.Mid (1);
	}

	CString	sLower	= sText;
	sLower.MakeLower ();

	int	nExe	= sLower.Find (TEXT (".exe"));

	return (nExe > 0) ? sText.Left (nExe + 4) : sText;
}

static void AddCandidate (std::vector<Candidate>& aCandidates, const CString& sExe, const CString& sRunName)
{
	CString	sKey	= PathKey (sExe);

	for (size_t i = 0; i < aCandidates.size (); i++)
	{
		if (PathKey (aCandidates[i].sExe) == sKey)
		{
			if (aCandidates[i].sRunName.IsEmpty ())
			{
				aCandidates[i].sRunName	= sRunName;
			}
			return;
		}
	}

	Candidate	oCandidate;

	oCandidate.sExe		= sExe;
	oCandidate.sRunName	= sRunName;
	aCandidates.push_back (oCandidate);
}


////////////////////////////////////////////////////////////////////////////////
//
//	The three places where older copies show up
//
////////////////////////////////////////////////////////////////////////////////

// Version 2 made an entry per copy
static void AddFromRunKey (std::vector<Candidate>& aCandidates)
{
	HKEY	hKey	= NULL;

	if (RegOpenKeyEx (HKEY_CURRENT_USER, RUN_KEY, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
	{
		return;
	}

	for (DWORD nIndex = 0; ; nIndex++)
	{
		TCHAR	sName[512]		= { 0 };
		TCHAR	sData[2048]		= { 0 };
		DWORD	nName			= _countof (sName);
		DWORD	nData			= sizeof (sData) - sizeof (TCHAR);
		DWORD	nType			= 0;
		LONG	nResult			= RegEnumValue (hKey, nIndex, sName, &nName, NULL, &nType, (LPBYTE) sData, &nData);

		if (nResult == ERROR_NO_MORE_ITEMS)
		{
			break;
		}
		if (nResult != ERROR_SUCCESS || (nType != REG_SZ && nType != REG_EXPAND_SZ) || _tcsnicmp (sName, OLD_RUN_PREFIX, _tcslen (OLD_RUN_PREFIX)) != 0)
		{
			continue;
		}

		CString	sCommand	= sData;

		if (nType == REG_EXPAND_SZ)
		{
			TCHAR	sExpanded[2048]	= { 0 };

			if (ExpandEnvironmentStrings (sData, sExpanded, _countof (sExpanded)) > 0)
			{
				sCommand	= sExpanded;
			}
		}

		CString	sExe	= ProgramFromCommand (sCommand);

		// The data should say where the program is. If not, the name does.
		if (sExe.IsEmpty ())
		{
			sExe	= CString (sName).Mid ((int) _tcslen (OLD_RUN_PREFIX));
		}
		AddCandidate (aCandidates, sExe, sName);
	}

	RegCloseKey (hKey);
}

// Version 1 had no option to start with Windows: a shortcut in the Startup folder is how it was done
static void AddFromStartupFolder (std::vector<Candidate>& aCandidates, REFKNOWNFOLDERID nFolderID)
{
	PWSTR	sFolder	= NULL;

	if (FAILED (SHGetKnownFolderPath (nFolderID, 0, NULL, &sFolder)) || sFolder == NULL)
	{
		return;
	}

	CString			sPattern	= CString (sFolder) + TEXT ("\\*.lnk");
	WIN32_FIND_DATA	oFound;
	HANDLE			hFind		= FindFirstFile (sPattern, &oFound);

	CoTaskMemFree (sFolder);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return;
	}

	// Resolving a shortcut needs COM. It may already be started on this thread, in another way: that is fine, then it is left alone.
	HRESULT	hrInit	= CoInitializeEx (NULL, COINIT_APARTMENTTHREADED);

	do
	{
		IShellLink*		pLink	= NULL;
		IPersistFile*	pFile	= NULL;
		CString			sShortcut	= sPattern.Left (sPattern.GetLength () - 5) + oFound.cFileName;

		if (SUCCEEDED (CoCreateInstance (CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**) &pLink)))
		{
			if (SUCCEEDED (pLink->QueryInterface (IID_IPersistFile, (void**) &pFile)))
			{
				TCHAR	sTarget[MAX_PATH]	= { 0 };

				if (SUCCEEDED (pFile->Load (sShortcut, STGM_READ)) && SUCCEEDED (pLink->GetPath (sTarget, MAX_PATH, NULL, 0))
					&& _tcsicmp (GetLeaf (sTarget), PROGRAM_FILE) == 0)
				{
					AddCandidate (aCandidates, sTarget, TEXT (""));
				}
				pFile->Release ();
			}
			pLink->Release ();
		}
	}
	while (FindNextFile (hFind, &oFound));

	FindClose (hFind);
	if (SUCCEEDED (hrInit))
	{
		CoUninitialize ();
	}
}

static void AddFromRunningPrograms (std::vector<Candidate>& aCandidates)
{
	HANDLE	hSnapshot	= CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);

	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		return;
	}

	PROCESSENTRY32	oProcess;

	oProcess.dwSize	= sizeof (oProcess);
	if (Process32First (hSnapshot, &oProcess))
	{
		do
		{
			if (_tcsicmp (oProcess.szExeFile, PROGRAM_FILE) != 0)
			{
				continue;
			}

			// Programs of other users or with more rights cannot be asked: they are not found
			HANDLE	hProcess	= OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION, FALSE, oProcess.th32ProcessID);

			if (hProcess != NULL)
			{
				TCHAR	sPath[MAX_PATH]	= { 0 };
				DWORD	nPath			= _countof (sPath);

				if (QueryFullProcessImageName (hProcess, 0, sPath, &nPath))
				{
					AddCandidate (aCandidates, sPath, TEXT (""));
				}
				CloseHandle (hProcess);
			}
		}
		while (Process32Next (hSnapshot, &oProcess));
	}

	CloseHandle (hSnapshot);
}


////////////////////////////////////////////////////////////////////////////////
//
//	The functions to use
//
////////////////////////////////////////////////////////////////////////////////

BOOL LoadOldCopy (const CString& sINIFile, OldCopy& oCopy)
{
	CStringArray	asSections;
	BOOL			bSettings	= FALSE;

	// A file without the sections of DiskLED would only give the default meter
	GetINISectionNames (sINIFile, asSections);
	for (int i = 0; i < asSections.GetSize (); i++)
	{
		bSettings	= bSettings || asSections[i].CompareNoCase (INI_LEGACY_COUNTER_1_SECTION) == 0
			|| asSections[i].Left ((int) _tcslen (INI_METER_SECTION_PREFIX)).CompareNoCase (INI_METER_SECTION_PREFIX) == 0;
	}
	if (!bSettings)
	{
		return FALSE;
	}

	oCopy.sIni	= sINIFile;
	oCopy.sRunName.Empty ();

	return LoadSettings (sINIFile, oCopy.oSettings) == ERROR_SUCCESS;
}

CString GetOldCopyName (const OldCopy& oCopy)
{
	return GetLeaf (GetFolder (oCopy.sIni));
}

void FindOldCopies (std::vector<OldCopy>& aCopies, const CString& sOwnINIFile)
{
	std::vector<Candidate>	aCandidates;
	TCHAR					sOwnExe[MAX_PATH]	= { 0 };

	aCopies.clear ();
	GetModuleFileName (NULL, sOwnExe, MAX_PATH);

	AddFromRunKey (aCandidates);
	AddFromStartupFolder (aCandidates, FOLDERID_Startup);
	AddFromStartupFolder (aCandidates, FOLDERID_CommonStartup);
	AddFromRunningPrograms (aCandidates);

	CString	sOwnExeKey	= PathKey (sOwnExe);
	CString	sOwnIniKey	= PathKey (sOwnINIFile);

	for (size_t i = 0; i < aCandidates.size (); i++)
	{
		// The settings of a copy are next to its program. A copy without an INI file runs on the defaults, there is nothing to import.
		CString		sIni	= GetFolder (aCandidates[i].sExe) + TEXT ("\\") + INI_SETTINGS_FILE;
		OldCopy		oCopy;
		BOOL		bKnown	= FALSE;

		if (PathKey (aCandidates[i].sExe) == sOwnExeKey || PathKey (sIni) == sOwnIniKey || !LoadOldCopy (sIni, oCopy))
		{
			continue;
		}
		oCopy.sRunName	= aCandidates[i].sRunName;

		for (size_t j = 0; j < aCopies.size (); j++)
		{
			bKnown	= bKnown || (PathKey (aCopies[j].sIni) == PathKey (sIni));
		}
		if (!bKnown)
		{
			aCopies.push_back (oCopy);
		}
	}
}

void RemoveRunEntries (const CStringArray& asNames)
{
	HKEY	hKey	= NULL;

	if (RegOpenKeyEx (HKEY_CURRENT_USER, RUN_KEY, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
	{
		return;
	}

	for (int i = 0; i < asNames.GetSize (); i++)
	{
		// Only the entries of older copies, which is what the list holds
		if (_tcsnicmp (asNames[i], OLD_RUN_PREFIX, _tcslen (OLD_RUN_PREFIX)) == 0)
		{
			RegDeleteValue (hKey, asNames[i]);
		}
	}

	RegCloseKey (hKey);
}
