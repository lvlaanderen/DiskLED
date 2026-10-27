#pragma once

DWORD	ReadINIValue (CString sINIFile, CString sSection, CString sKey, CString* sValue);
DWORD	WriteINIValue (const CString &sINIFile, const CString &sSection, const CString &sKey, const CString &sValue);
DWORD	WriteINIValue (const CString &sINIFile, const CString &sSection, const CString &sKey, const int &nValue);
DWORD	WriteINIValue (const CString &sINIFile, const CString &sSection, const CString &sKey, const double &nValue);
BOOL	IsFileExistent (CString sPath);
BOOL	IsAutoStartEnabled ();
DWORD	SetAutoStart (BOOL bEnable);
BOOL	ParsePerfPath (const CString& sPath, CString& sMachine, CString& sObject, CString& sInstance, CString& sCounter);
CString	MakePerfPath (const CString& sMachine, const CString& sObject, const CString& sInstance, const CString& sCounter);
CString	LocalPathToEnglish (const CString& sPath);
CString	EnglishPathToLocal (const CString& sPath);

// The speed of the network adapter of a \Network Interface(<instance>)\Bytes ... counter (an English path), in bytes/s. Not the speed of the internet connection!
BOOL	GetNetworkLinkSpeed (const CString& sEnglishPath, double& dBytesPerSec);

// The performance counter instance name of the network adapter that carries internet traffic (the one with the best route to a public address). Empty if not found.
CString	GetDefaultNetworkInstance ();
DWORD	GetFileAttributesAPIWrapper (CString sPath, DWORD* nAttributes);
BOOL	Split (CString sDelimiter, CString sInput, CStringArray* asOutput);
