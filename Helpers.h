#pragma once

DWORD	ReadINIValue (CString sINIFile, CString sSection, CString sKey, CString* sValue);
DWORD	WriteINIValue (const CString &sINIFile, const CString &sSection, const CString &sKey, const CString &sValue);
DWORD	WriteINIValue (const CString &sINIFile, const CString &sSection, const CString &sKey, const int &nValue);
DWORD	WriteINIValue (const CString &sINIFile, const CString &sSection, const CString &sKey, const double &nValue);
void	GetINISectionNames (const CString& sINIFile, CStringArray& asNames);
DWORD	DeleteINISection (const CString& sINIFile, const CString& sSection);
BOOL	IsFileExistent (CString sPath);

// The INI file: %AppData%\DiskLED\DiskLED.ini, or next to the executable in portable mode. A file next to the executable from an older version is copied to %AppData% once.
CString	GetSettingsFile ();
BOOL	IsPortableMode ();
DWORD	CreateParentFolder (const CString& sFile);

BOOL	IsAutoStartEnabled ();
DWORD	SetAutoStart (BOOL bEnable);
void	MigrateAutoStart ();
void	RemoveLegacyAutoStartEntry ();
BOOL	ParsePerfPath (const CString& sPath, CString& sMachine, CString& sObject, CString& sInstance, CString& sCounter);
CString	MakePerfPath (const CString& sMachine, const CString& sObject, const CString& sInstance, const CString& sCounter);
CString	LocalPathToEnglish (const CString& sPath);
CString	EnglishPathToLocal (const CString& sPath);

// Is this a valid counter path when read as an English path?
BOOL	IsValidEnglishCounterPath (const CString& sPath);

// Counters that are not in degrees Celsius: value * dFactor + dOffset gives Celsius. FALSE (factor 1, offset 0) for all other counters.
BOOL	GetCelsiusConversion (const CString& sPath, double& dFactor, double& dOffset);

// The current temperature of a thermal zone (an instance of "Thermal Zone Information"). FALSE if the zone has no sensor.
BOOL	GetThermalZoneCelsius (const CString& sInstance, double& dCelsius);

// The instances of a performance object (an English object name, e.g. "Network Interface"), as the counters know them
void	GetPerfInstances (const CString& sEnglishObject, CStringArray& asInstances);

// Center a window on the monitor with the mouse pointer (where the user clicked the tray icon)
void	CenterOnPointerMonitor (CWnd* pWindow);

// The speed of the network adapter of a \Network Interface(<instance>)\Bytes ... counter (an English path), in bytes/s. Not the speed of the internet connection!
BOOL	GetNetworkLinkSpeed (const CString& sEnglishPath, double& dBytesPerSec);

// The performance counter instance name of the network adapter that carries internet traffic (the one with the best route to a public address). Empty if not found.
CString	GetDefaultNetworkInstance ();
DWORD	GetFileAttributesAPIWrapper (CString sPath, DWORD* nAttributes);
BOOL	Split (CString sDelimiter, CString sInput, CStringArray* asOutput);
