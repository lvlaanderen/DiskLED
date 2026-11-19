#include "stdafx.h"
#include "Helpers.h"
#include "Settings.h"
#include "GpuSensor.h"
#include <pdh.h>
#include <PDHMsg.h>
#include <iphlpapi.h>
#include <shlobj.h>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "shell32.lib")


//////////////////////////////////////////////////////////////////////
//
//		ReadINIValue:
//!	Read the value of a key in a specified section of an INI file
//
//////////////////////////////////////////////////////////////////////

DWORD ReadINIValue (CString sINIFile, CString sSection, CString sKey, CString* sValue)
{
	// Check parameters
	if (! sValue)
	{
		return ERROR_INVALID_PARAMETER;
	}

	// Empty the output string
	sValue->Empty ();

	//
	// Check the INI file for existence
	//
	if (! IsFileExistent (sINIFile))
	{
		return ERROR_FILE_NOT_FOUND;
	}

	//
	// Read the value from the config file
	//
	DWORD		nBuffer	= 4096;
	TCHAR*	pBuffer	= NULL;
	DWORD		nCopied	= 0;
	do
	{
		// Allocate a buffer
		pBuffer	= (TCHAR*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof (TCHAR) * nBuffer);
		if (pBuffer == NULL)
		{
			return ERROR_OUTOFMEMORY;
		}

		// Try to read the value in the section specified by sName
		nCopied	= GetPrivateProfileString (sSection, sKey, NULL, pBuffer, nBuffer, sINIFile);

		if (nCopied == 0)
		{
			HeapFree(GetProcessHeap(), 0, pBuffer);
			pBuffer	= NULL;
			return ERROR_NOT_FOUND;
		}
		else if (nCopied == nBuffer - 2)
		{
			// The buffer is too small
			nBuffer	+=	4096;
			HeapFree(GetProcessHeap(), 0, pBuffer);
			pBuffer	= NULL;
			continue;
		}
		else if (nCopied > nBuffer - 2)
		{
			// This should not happen
			HeapFree(GetProcessHeap(), 0, pBuffer);
			pBuffer	= NULL;
			return ERROR_NOT_FOUND;
		}
		else
		{
			break;
		}
	} while (1);

	// Copy the returned string into the out parameter
	sValue->Format (TEXT ("%s"), pBuffer);

	HeapFree(GetProcessHeap(), 0, pBuffer);
	pBuffer	= NULL;

	return ERROR_SUCCESS;
}


//////////////////////////////////////////////////////////////////////
//
//		IsFileExistent:
//!	Checks whether file exists (and is accessible)
//
//////////////////////////////////////////////////////////////////////

BOOL IsFileExistent (CString sPath)
{
	BOOL	bExists	= FALSE;
	DWORD	nAttrs	= 0;
	
	DWORD	nError	= GetFileAttributesAPIWrapper (sPath, &nAttrs);
	if (nError == ERROR_SUCCESS)
	{
		if (nAttrs & FILE_ATTRIBUTE_DIRECTORY)
		{
			bExists	= FALSE;
		}
		else
		{
			bExists	= TRUE;
		}
	}

	return bExists;
}


//////////////////////////////////////////////////////////////////////
//
//		GetFileAttributesAPIWrapper:
//!	Wrapper for the API function GetFileAttributes
//
//////////////////////////////////////////////////////////////////////

DWORD GetFileAttributesAPIWrapper (CString sPath, DWORD* nAttributes)
{
	DWORD nError		= ERROR_INVALID_FUNCTION;

	// Check parameters
	if (! nAttributes)
	{
		return ERROR_INVALID_PARAMETER;
	}

	// Call GetFileAttributes
	*nAttributes	= GetFileAttributes (sPath);
	if (*nAttributes == INVALID_FILE_ATTRIBUTES)
	{
		nError	= GetLastError ();
	}
	else
	{
		nError	= ERROR_SUCCESS;
	}

	return nError;
}


//////////////////////////////////////////////////////////////////////
//
//		WriteINIValue:
//!		Writes the value of a key in a specified section of an INI file
//
//////////////////////////////////////////////////////////////////////

DWORD WriteINIValue (const CString &sINIFile, const CString &sSection, const CString &sKey, const CString &sValue)
{
	DWORD nError;
	
	//
	// Write the value to the config file
	//
	nError	= WritePrivateProfileString (sSection, sKey, sValue, sINIFile);
	if (nError == 0)
	{
		nError = GetLastError();
		return nError;
	}

	return ERROR_SUCCESS;
}

DWORD WriteINIValue (const CString &sINIFile, const CString &sSection, const CString &sKey, const int &nValue)
{
	CString	sValue;
	sValue.Format (TEXT ("%d"), nValue);

	return WriteINIValue (sINIFile, sSection, sKey, sValue);
}

DWORD WriteINIValue (const CString &sINIFile, const CString &sSection, const CString &sKey, const double &nValue)
{
	CString	sValue;
sValue.Format (TEXT ("%.15g"), nValue);

	return WriteINIValue (sINIFile, sSection, sKey, sValue);
}


//////////////////////////////////////////////////////////////////////
//
//		The names of all sections of an INI file
//
//////////////////////////////////////////////////////////////////////

void GetINISectionNames (const CString& sINIFile, CStringArray& asNames)
{
	DWORD	nBuffer	= 2048;

	asNames.RemoveAll ();
	if (!IsFileExistent (sINIFile))
	{
		return;
	}

	for (;;)
	{
		TCHAR*	pBuffer	= (TCHAR*) HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof (TCHAR) * nBuffer);
		if (pBuffer == NULL)
		{
			return;
		}

		DWORD	nCopied	= GetPrivateProfileSectionNames (pBuffer, nBuffer, sINIFile);
		if (nCopied == nBuffer - 2)
		{
			// The buffer is too small
			HeapFree (GetProcessHeap (), 0, pBuffer);
			nBuffer	*= 2;
			continue;
		}

		for (const TCHAR* sName = pBuffer; *sName != 0; sName += _tcslen (sName) + 1)
		{
			asNames.Add (sName);
		}
		HeapFree (GetProcessHeap (), 0, pBuffer);
		return;
	}
}

DWORD DeleteINISection (const CString& sINIFile, const CString& sSection)
{
	if (!IsFileExistent (sINIFile))
	{
		return ERROR_SUCCESS;
	}

	return WritePrivateProfileString (sSection, NULL, NULL, sINIFile) ? ERROR_SUCCESS : GetLastError ();
}


//////////////////////////////////////////////////////////////////////
//
//		Where the settings are kept
//
//////////////////////////////////////////////////////////////////////

// A file with one of these names next to the executable turns on the portable mode
#define PORTABLE_MARKER_1		TEXT ("portable")
#define PORTABLE_MARKER_2		TEXT ("portable.txt")

#define SETTINGS_FOLDER			TEXT ("DiskLED")

static CString GetExecutableFolder ()
{
	TCHAR	sPath[MAX_PATH]	= { 0 };

	GetModuleFileName (NULL, sPath, MAX_PATH);

	CString	sFolder	= sPath;

	return sFolder.Left (sFolder.ReverseFind ('\\'));
}

BOOL IsPortableMode ()
{
	CString	sFolder	= GetExecutableFolder ();

	return IsFileExistent (sFolder + TEXT ("\\") + PORTABLE_MARKER_1) || IsFileExistent (sFolder + TEXT ("\\") + PORTABLE_MARKER_2);
}

DWORD CreateParentFolder (const CString& sFile)
{
	CString	sFolder	= sFile.Left (sFile.ReverseFind ('\\'));
	int		nError	= SHCreateDirectoryEx (NULL, sFolder, NULL);

	return (nError == ERROR_SUCCESS || nError == ERROR_ALREADY_EXISTS || nError == ERROR_FILE_EXISTS) ? ERROR_SUCCESS : (DWORD) nError;
}

CString GetSettingsFile ()
{
	CString	sExeFile	= GetExecutableFolder () + TEXT ("\\") + INI_SETTINGS_FILE;

	if (IsPortableMode ())
	{
		return sExeFile;
	}

	// %AppData%\DiskLED, which the user can always write to (the installation folder may be read-only)
	PWSTR	sAppData	= NULL;
	CString	sFile;

	if (FAILED (SHGetKnownFolderPath (FOLDERID_RoamingAppData, KF_FLAG_CREATE, NULL, &sAppData)) || sAppData == NULL)
	{
		CoTaskMemFree (sAppData);
		return sExeFile;
	}
	sFile	= CString (sAppData) + TEXT ("\\") + SETTINGS_FOLDER + TEXT ("\\") + INI_SETTINGS_FILE;
	CoTaskMemFree (sAppData);

	// Settings from the installation folder (older versions): copy them once. The old file is renamed if possible, so it is not mistaken for current settings.
	if (!IsFileExistent (sFile) && IsFileExistent (sExeFile))
	{
		if (CreateParentFolder (sFile) == ERROR_SUCCESS && CopyFile (sExeFile, sFile, TRUE))
		{
			MoveFileEx (sExeFile, sExeFile + TEXT (".migrated"), MOVEFILE_REPLACE_EXISTING);
		}
	}

	return sFile;
}


//////////////////////////////////////////////////////////////////////
//
// Split: Called by various CSetACL functions. Emulation of Perl's split function.
//
//////////////////////////////////////////////////////////////////////

BOOL Split (CString sDelimiter, CString sInput, CStringArray* asOutput)
{
	try
	{
		// Delete contents of the output array
		asOutput->RemoveAll ();

		// Remove leading and trailing whitespace
		sInput.TrimLeft ();
		sInput.TrimRight ();

		// Find each substring and add it to the output array
		int i = 0, j;
		while ((j = sInput.Find (sDelimiter, i)) != -1)
		{
			asOutput->Add (sInput.Mid (i, j - i));
			i = j + 1;
		}

		// Is there an element left at the end?
		if (sInput.GetLength () > i)
		{
			asOutput->Add (sInput.Mid (i, sInput.GetLength () - i));
		}

		return TRUE;
	}
	catch (CMemoryException* exc)
	{
		exc->Delete ();

		return FALSE;
	}
}


//////////////////////////////////////////////////////////////////////
//
//      Performance counter paths
//      Counter names are localized. Windows keeps the English names in the registry,
//      which lets the settings use English paths that work on any system language.
//
//////////////////////////////////////////////////////////////////////

static CMap<DWORD, DWORD, CString, LPCTSTR>		g_oEnglishNames;		// Index -> English name
static CMap<CString, LPCTSTR, DWORD, DWORD>		g_oEnglishIndexes;		// Lower-case English name -> index
static BOOL										g_bEnglishNamesLoaded	= FALSE;

static void LoadEnglishNames ()
{
	if (g_bEnglishNamesLoaded)
	{
		return;
	}
	g_bEnglishNamesLoaded	= TRUE;

	HKEY	hKey	= NULL;
	if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, TEXT ("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib\\009"), 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
	{
		return;
	}

	DWORD	nSize	= 0;
	DWORD	nType	= 0;
	if (RegQueryValueEx (hKey, TEXT ("Counter"), NULL, &nType, NULL, &nSize) == ERROR_SUCCESS && nType == REG_MULTI_SZ && nSize > 0)
	{
		TCHAR*	pBuffer	= (TCHAR*) HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, nSize + 2 * sizeof (TCHAR));
		if (pBuffer && RegQueryValueEx (hKey, TEXT ("Counter"), NULL, &nType, (LPBYTE) pBuffer, &nSize) == ERROR_SUCCESS)
		{
			// The value is a list of string pairs: index, name
			for (const TCHAR* p = pBuffer; *p != 0; )
			{
				DWORD	nIndex	= (DWORD) _ttoi (p);
				p				+= _tcslen (p) + 1;
				if (*p == 0)
				{
					break;
				}

				CString	sName (p);
				p				+= _tcslen (p) + 1;

				g_oEnglishNames[nIndex]	= sName;

				CString	sLower (sName);
				sLower.MakeLower ();
				DWORD	nExisting	= 0;
				if (!g_oEnglishIndexes.Lookup (sLower, nExisting))
				{
					g_oEnglishIndexes[sLower]	= nIndex;
				}
			}
		}
		if (pBuffer)
		{
			HeapFree (GetProcessHeap (), 0, pBuffer);
		}
	}

	RegCloseKey (hKey);
}

// The English name of a (possibly localized) object or counter name. Returns the name unchanged if there is no translation.
static CString LocalNameToEnglish (const CString& sName)
{
	LoadEnglishNames ();

	DWORD	nIndex	= 0;
	CString	sEnglish;
	if (PdhLookupPerfIndexByName (NULL, sName, &nIndex) == ERROR_SUCCESS && g_oEnglishNames.Lookup (nIndex, sEnglish))
	{
		return sEnglish;
	}

	return sName;
}

// The localized name of an English object or counter name. Returns the name unchanged if there is no translation.
static CString EnglishNameToLocal (const CString& sName)
{
	LoadEnglishNames ();

	CString	sLower (sName);
	sLower.MakeLower ();

	DWORD	nIndex	= 0;
	if (!g_oEnglishIndexes.Lookup (sLower, nIndex))
	{
		return sName;
	}

	TCHAR	sBuffer[256]	= { 0 };
	DWORD	nBuffer			= 256;
	if (PdhLookupPerfNameByIndex (NULL, nIndex, sBuffer, &nBuffer) == ERROR_SUCCESS)
	{
		return CString (sBuffer);
	}

	return sName;
}

// Is this a valid counter path when interpreted as English?
static BOOL IsValidEnglishPath (const CString& sPath)
{
	PDH_HQUERY		hQuery		= NULL;
	PDH_HCOUNTER	hCounter	= NULL;
	BOOL			bValid		= FALSE;

	if (PdhOpenQuery (NULL, 0, &hQuery) == ERROR_SUCCESS)
	{
		bValid	= (PdhAddEnglishCounter (hQuery, sPath, 0, &hCounter) == ERROR_SUCCESS);
		PdhCloseQuery (hQuery);
	}

	return bValid;
}

// Split a counter path like \Object(Instance)\Counter or \\Machine\Object(Instance)\Counter into its parts.
// An instance name can contain backslashes (e.g. \Thermal Zone Information(\_SB.TZ03)\Temperature): it ends at the last ")\".
BOOL ParsePerfPath (const CString& sPath, CString& sMachine, CString& sObject, CString& sInstance, CString& sCounter)
{
	CString	sRest	= sPath;

	sMachine.Empty ();
	sObject.Empty ();
	sInstance.Empty ();
	sCounter.Empty ();

	if (sRest.Left (2) == TEXT ("\\\\"))
	{
		int	nEnd	= sRest.Find ('\\', 2);

		if (nEnd < 0)
		{
			return FALSE;
		}
		sMachine	= sRest.Mid (2, nEnd - 2);
		sRest		= sRest.Mid (nEnd);
	}
	if (sRest.IsEmpty () || sRest[0] != '\\')
	{
		return FALSE;
	}
	sRest	= sRest.Mid (1);

	int	nOpen		= sRest.Find ('(');
	int	nSeparator	= sRest.Find ('\\');

	if (nOpen >= 0 && (nSeparator < 0 || nOpen < nSeparator))
	{
		// Object(Instance)\Counter
		int	nClose	= sRest.ReverseFind (')');

		while (nClose > nOpen && (nClose + 1 >= sRest.GetLength () || sRest[nClose + 1] != '\\'))
		{
			nClose	= (nClose > 0) ? sRest.Left (nClose).ReverseFind (')') : -1;
		}
		if (nClose <= nOpen)
		{
			return FALSE;
		}
		sObject		= sRest.Left (nOpen);
		sInstance	= sRest.Mid (nOpen + 1, nClose - nOpen - 1);
		sCounter	= sRest.Mid (nClose + 2);
	}
	else if (nSeparator >= 0)
	{
		sObject		= sRest.Left (nSeparator);
		sCounter	= sRest.Mid (nSeparator + 1);
	}
	else
	{
		return FALSE;
	}

	// A counter name has no backslash
	return !sObject.IsEmpty () && !sCounter.IsEmpty () && sCounter.Find ('\\') < 0;
}

CString MakePerfPath (const CString& sMachine, const CString& sObject, const CString& sInstance, const CString& sCounter)
{
	CString	sPath;

	if (!sMachine.IsEmpty ())
	{
		sPath	= TEXT ("\\\\") + sMachine;
	}
	sPath	+= TEXT ("\\") + sObject;
	if (!sInstance.IsEmpty ())
	{
		sPath	+= TEXT ("(") + sInstance + TEXT (")");
	}
	sPath	+= TEXT ("\\") + sCounter;

	return sPath;
}

// A path in the system language to an English path. Returns the path unchanged if no valid English path results.
CString LocalPathToEnglish (const CString& sPath)
{
	CString	sMachine;
	CString	sObject;
	CString	sInstance;
	CString	sCounter;

	if (!ParsePerfPath (sPath, sMachine, sObject, sInstance, sCounter))
	{
		return sPath;
	}

	CString	sEnglish	= MakePerfPath (sMachine, LocalNameToEnglish (sObject), sInstance, LocalNameToEnglish (sCounter));

	return IsValidEnglishPath (sEnglish) ? sEnglish : sPath;
}

// An English path to a path in the system language. Returns the path unchanged if it is already valid or no valid path results.
CString EnglishPathToLocal (const CString& sPath)
{
	CString	sMachine;
	CString	sObject;
	CString	sInstance;
	CString	sCounter;

	if (PdhValidatePath (sPath) == ERROR_SUCCESS)
	{
		return sPath;
	}
	if (!ParsePerfPath (sPath, sMachine, sObject, sInstance, sCounter))
	{
		return sPath;
	}

	CString	sLocal	= MakePerfPath (sMachine, EnglishNameToLocal (sObject), sInstance, EnglishNameToLocal (sCounter));

	return (PdhValidatePath (sLocal) == ERROR_SUCCESS) ? sLocal : sPath;
}


// Is this a valid counter path when interpreted as English? (A graphics adapter temperature is valid if the adapter reports one.)
BOOL IsValidEnglishCounterPath (const CString& sPath)
{
	CString	sAdapter;

	if (IsGpuPath (sPath, &sAdapter))
	{
		double	dCelsius	= 0;

		return ReadGpuTemperature (sAdapter, dCelsius);
	}

	return IsValidEnglishPath (sPath);
}

// Thermal zone temperatures are reported in Kelvin: whole degrees by "Temperature", tenths of a degree by "High Precision Temperature"
BOOL GetCelsiusConversion (const CString& sPath, double& dFactor, double& dOffset)
{
	CString	sMachine;
	CString	sObject;
	CString	sInstance;
	CString	sCounter;

	dFactor	= 1;
	dOffset	= 0;

	if (!ParsePerfPath (LocalPathToEnglish (sPath), sMachine, sObject, sInstance, sCounter) || sObject.CompareNoCase (TEXT ("Thermal Zone Information")) != 0)
	{
		return FALSE;
	}
	if (sCounter.CompareNoCase (TEXT ("High Precision Temperature")) == 0)
	{
		dFactor	= 0.1;
	}
	else if (sCounter.CompareNoCase (TEXT ("Temperature")) != 0)
	{
		return FALSE;
	}
	dOffset	= -273.15;

	return TRUE;
}

// The current temperature of a thermal zone (an instance of "Thermal Zone Information"), in degrees Celsius
BOOL GetThermalZoneCelsius (const CString& sInstance, double& dCelsius)
{
	PDH_HQUERY		hQuery		= NULL;
	PDH_HCOUNTER	hCounter	= NULL;
	BOOL			bResult		= FALSE;
	CString			sPath		= MakePerfPath (TEXT (""), TEXT ("Thermal Zone Information"), sInstance, TEXT ("High Precision Temperature"));
	double			dFactor		= 1;
	double			dOffset		= 0;

	dCelsius	= 0;

	if (!GetCelsiusConversion (sPath, dFactor, dOffset))
	{
		return FALSE;
	}

	if (PdhOpenQuery (NULL, 0, &hQuery) == ERROR_SUCCESS)
	{
		if (PdhAddEnglishCounter (hQuery, sPath, 0, &hCounter) == ERROR_SUCCESS && PdhCollectQueryData (hQuery) == ERROR_SUCCESS)
		{
			PDH_FMT_COUNTERVALUE	oValue;

			if (PdhGetFormattedCounterValue (hCounter, PDH_FMT_DOUBLE, NULL, &oValue) == ERROR_SUCCESS
				&& (oValue.CStatus == PDH_CSTATUS_VALID_DATA || oValue.CStatus == PDH_CSTATUS_NEW_DATA))
			{
				dCelsius	= oValue.doubleValue * dFactor + dOffset;

				// A zone that reports 0 Kelvin has no sensor
				bResult		= (dCelsius > -100.0);
			}
		}
		PdhCloseQuery (hQuery);
	}

	return bResult;
}

// Center a window on the monitor with the mouse pointer. The hidden main window is no place to center on, it can be on another monitor.
void CenterOnPointerMonitor (CWnd* pWindow)
{
	POINT		oCursor;
	MONITORINFO	oMonitor;
	CRect		oWindow;

	GetCursorPos (&oCursor);
	oMonitor.cbSize	= sizeof (oMonitor);
	if (GetMonitorInfo (MonitorFromPoint (oCursor, MONITOR_DEFAULTTOPRIMARY), &oMonitor))
	{
		pWindow->GetWindowRect (oWindow);
		pWindow->SetWindowPos (NULL, oMonitor.rcWork.left + (oMonitor.rcWork.right - oMonitor.rcWork.left - oWindow.Width ()) / 2,
			oMonitor.rcWork.top + (oMonitor.rcWork.bottom - oMonitor.rcWork.top - oWindow.Height ()) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
}

// The instances of a performance object (an English name, e.g. "Network Interface"), as the counters know them
void GetPerfInstances (const CString& sEnglishObject, CStringArray& asInstances)
{
	CString	sObject		= EnglishNameToLocal (sEnglishObject);
	DWORD	nCounters	= 0;
	DWORD	nInstances	= 0;

	asInstances.RemoveAll ();

	// The first call returns the sizes of the lists
	if (PdhEnumObjectItems (NULL, NULL, sObject, NULL, &nCounters, NULL, &nInstances, PERF_DETAIL_WIZARD, 0) != PDH_MORE_DATA || nInstances == 0)
	{
		return;
	}

	TCHAR*	sCounters	= (TCHAR*) HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof (TCHAR) * (nCounters + 2));
	TCHAR*	sInstances	= (TCHAR*) HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, sizeof (TCHAR) * (nInstances + 2));

	if (sCounters != NULL && sInstances != NULL
		&& PdhEnumObjectItems (NULL, NULL, sObject, sCounters, &nCounters, sInstances, &nInstances, PERF_DETAIL_WIZARD, 0) == ERROR_SUCCESS)
	{
		for (const TCHAR* sName = sInstances; *sName != 0; sName += _tcslen (sName) + 1)
		{
			asInstances.Add (sName);
		}
	}

	if (sCounters != NULL)
	{
		HeapFree (GetProcessHeap (), 0, sCounters);
	}
	if (sInstances != NULL)
	{
		HeapFree (GetProcessHeap (), 0, sInstances);
	}
}


// The speed of the network adapter behind a Bytes counter of the Network Interface object, in bytes/s
BOOL GetNetworkLinkSpeed (const CString& sEnglishPath, double& dBytesPerSec)
{
	CString			sMachine;
	CString			sObject;
	CString			sInstance;
	CString			sCounter;
	PDH_HQUERY		hQuery		= NULL;
	PDH_HCOUNTER	hCounter	= NULL;
	BOOL			bResult		= FALSE;

	dBytesPerSec	= 0;

	if (!ParsePerfPath (sEnglishPath, sMachine, sObject, sInstance, sCounter))
	{
		return FALSE;
	}
	if (sObject.CompareNoCase (TEXT ("Network Interface")) != 0 || sInstance.IsEmpty () || sInstance == TEXT ("*") || sCounter.Left (5).CompareNoCase (TEXT ("Bytes")) != 0)
	{
		return FALSE;
	}

	if (PdhOpenQuery (NULL, 0, &hQuery) == ERROR_SUCCESS)
	{
		if (PdhAddEnglishCounter (hQuery, MakePerfPath (sMachine, sObject, sInstance, TEXT ("Current Bandwidth")), 0, &hCounter) == ERROR_SUCCESS
			&& PdhCollectQueryData (hQuery) == ERROR_SUCCESS)
		{
			PDH_FMT_COUNTERVALUE	oValue;

			if (PdhGetFormattedCounterValue (hCounter, PDH_FMT_DOUBLE, NULL, &oValue) == ERROR_SUCCESS && oValue.doubleValue > 0)
			{
				// Bits per second. Both directions together can carry twice that.
				dBytesPerSec	= oValue.doubleValue / 8.0;
				if (sCounter.CompareNoCase (TEXT ("Bytes Total/sec")) == 0)
				{
					dBytesPerSec	*= 2.0;
				}
				bResult			= TRUE;
			}
		}
		PdhCloseQuery (hQuery);
	}

	return bResult;
}


// The performance counter instance name of the adapter that has the best route to a public address
CString GetDefaultNetworkInstance ()
{
	CString						sInstance;
	DWORD						nIndex		= 0;
	ULONG						nSize		= 16 * 1024;
	ULONG						nResult		= ERROR_INVALID_FUNCTION;
	IP_ADAPTER_ADDRESSES*		pAddresses	= NULL;

	// 8.8.8.8: only used to find the route, nothing is sent
	if (GetBestInterface (0x08080808, &nIndex) != NO_ERROR)
	{
		return sInstance;
	}

	for (int nTry = 0; nTry < 2; nTry++)
	{
		pAddresses	= (IP_ADAPTER_ADDRESSES*) HeapAlloc (GetProcessHeap (), 0, nSize);
		if (pAddresses == NULL)
		{
			return sInstance;
		}

		nResult	= GetAdaptersAddresses (AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, NULL, pAddresses, &nSize);
		if (nResult != ERROR_BUFFER_OVERFLOW)
		{
			break;
		}
		HeapFree (GetProcessHeap (), 0, pAddresses);
		pAddresses	= NULL;
	}

	if (pAddresses == NULL)
	{
		return sInstance;
	}

	if (nResult == NO_ERROR)
	{
		for (IP_ADAPTER_ADDRESSES* pAdapter = pAddresses; pAdapter != NULL; pAdapter = pAdapter->Next)
		{
			if (pAdapter->IfIndex == nIndex && pAdapter->Description != NULL)
			{
				sInstance	= pAdapter->Description;
				break;
			}
		}
	}
	HeapFree (GetProcessHeap (), 0, pAddresses);

	// Performance counter instance names do not contain these characters
	sInstance.Replace ('(', '[');
	sInstance.Replace (')', ']');
	sInstance.Replace ('#', '_');
	sInstance.Replace ('\\', '_');
	sInstance.Replace ('/', '_');

	return sInstance;
}


//////////////////////////////////////////////////////////////////////
//
//      Autostart: a per-user Run entry for this executable
//      There is one entry, named DiskLED, because only one DiskLED runs per user. Versions up to 2.1 used one entry per copy,
//      named "DiskLED - <path>", see MigrateAutoStart.
//
//////////////////////////////////////////////////////////////////////

#define AUTOSTART_REG_KEY	TEXT ("Software\\Microsoft\\Windows\\CurrentVersion\\Run")
#define AUTOSTART_VALUE		TEXT ("DiskLED")

// Windows 8 and later keep the enabled/disabled state shown in Task Manager's Startup tab here
#define AUTOSTART_APPROVED_REG_KEY	TEXT ("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run")

// Disabled in Task Manager? The first byte of the value is even (2, 6) when enabled and odd (3, 7) when disabled.
static BOOL IsStartupDisabled (const CString& sName)
{
	HKEY	hKey		= NULL;
	BOOL	bDisabled	= FALSE;

	if (RegOpenKeyEx (HKEY_CURRENT_USER, AUTOSTART_APPROVED_REG_KEY, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		BYTE	abData[16]	= { 0 };
		DWORD	nSize		= sizeof (abData);
		DWORD	nType		= 0;

		if (RegQueryValueEx (hKey, sName, NULL, &nType, abData, &nSize) == ERROR_SUCCESS && nType == REG_BINARY && nSize >= 1)
		{
			bDisabled	= (abData[0] & 1) != 0;
		}
		RegCloseKey (hKey);
	}

	return bDisabled;
}

// Forget the Task Manager state, so an entry we add is enabled and a removed entry leaves nothing behind
static void ClearStartupApproval (const CString& sName)
{
	HKEY	hKey	= NULL;

	if (RegOpenKeyEx (HKEY_CURRENT_USER, AUTOSTART_APPROVED_REG_KEY, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
	{
		RegDeleteValue (hKey, sName);
		RegCloseKey (hKey);
	}
}

static CString GetAutoStartEntry (CString& sCommand)
{
	TCHAR	sPath[MAX_PATH]	= { 0 };

	GetModuleFileName (NULL, sPath, MAX_PATH);
	sCommand	= TEXT ("\"") + CString (sPath) + TEXT ("\"");

	return CString (AUTOSTART_VALUE);
}

// The name of the entry that versions up to 2.1 made for this executable
static CString GetLegacyAutoStartName ()
{
	TCHAR	sPath[MAX_PATH]	= { 0 };

	GetModuleFileName (NULL, sPath, MAX_PATH);

	return CString (TEXT ("DiskLED - ")) + sPath;
}

BOOL IsAutoStartEnabled ()
{
	CString		sCommand;
	CString		sName	= GetAutoStartEntry (sCommand);
	HKEY		hKey	= NULL;
	BOOL		bResult	= FALSE;

	if (RegOpenKeyEx (HKEY_CURRENT_USER, AUTOSTART_REG_KEY, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		TCHAR	sData[MAX_PATH + 3]	= { 0 };
		DWORD	nSize				= sizeof (sData) - sizeof (TCHAR);
		DWORD	nType				= 0;

		if (RegQueryValueEx (hKey, sName, NULL, &nType, (LPBYTE) sData, &nSize) == ERROR_SUCCESS && nType == REG_SZ)
		{
			bResult	= (sCommand.CompareNoCase (sData) == 0) && !IsStartupDisabled (sName);
		}
		RegCloseKey (hKey);
	}

	return bResult;
}

DWORD SetAutoStart (BOOL bEnable)
{
	CString		sCommand;
	CString		sName	= GetAutoStartEntry (sCommand);
	HKEY		hKey	= NULL;
	DWORD		nError	= ERROR_SUCCESS;

	if (bEnable)
	{
		nError	= RegCreateKeyEx (HKEY_CURRENT_USER, AUTOSTART_REG_KEY, 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);
		if (nError == ERROR_SUCCESS)
		{
			nError	= RegSetValueEx (hKey, sName, 0, REG_SZ, (const BYTE*) (LPCTSTR) sCommand, (sCommand.GetLength () + 1) * sizeof (TCHAR));
			RegCloseKey (hKey);
		}
		if (nError == ERROR_SUCCESS)
		{
			ClearStartupApproval (sName);
		}
	}
	else
	{
		nError	= RegOpenKeyEx (HKEY_CURRENT_USER, AUTOSTART_REG_KEY, 0, KEY_SET_VALUE, &hKey);
		if (nError == ERROR_SUCCESS)
		{
			nError	= RegDeleteValue (hKey, sName);
			RegCloseKey (hKey);
		}

		// Nothing to remove is fine
		if (nError == ERROR_FILE_NOT_FOUND)
		{
			nError	= ERROR_SUCCESS;
		}
		ClearStartupApproval (sName);
	}

	return nError;
}

// Versions up to 2.1 made one entry per copy of the program, named "DiskLED - <path>". Remove the one of this copy and return whether it was active.
static BOOL RemoveLegacyAutoStart ()
{
	CString	sName		= GetLegacyAutoStartName ();
	HKEY	hKey		= NULL;
	BOOL	bFound		= FALSE;
	BOOL	bEnabled	= FALSE;

	if (RegOpenKeyEx (HKEY_CURRENT_USER, AUTOSTART_REG_KEY, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
	{
		bFound	= (RegQueryValueEx (hKey, sName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS);
		if (bFound)
		{
			bEnabled	= !IsStartupDisabled (sName);
			RegDeleteValue (hKey, sName);
			ClearStartupApproval (sName);
		}
		RegCloseKey (hKey);
	}

	return bFound && bEnabled;
}

// Replace the old per-copy entry by the single entry (the running instance owns it)
void MigrateAutoStart ()
{
	if (RemoveLegacyAutoStart ())
	{
		SetAutoStart (TRUE);
	}
}

// Another instance is already running and owns the autostart: only clean up the old entry of this copy
void RemoveLegacyAutoStartEntry ()
{
	RemoveLegacyAutoStart ();
}


//////////////////////////////////////////////////////////////////////
////	Select the item in a combo box list that contains a given string
//
//////////////////////////////////////////////////////////////////////

/*void SelectComboBoxString (CComboBox& oBox, CString& sItem)
{
	for (int nIndex	= 0; nIndex < oBox.GetCount (); nIndex++)
	{
		oBox.GetItemData (nIndex);
	}
}*/