#pragma once

#include <vector>
#include "Settings.h"


// An older copy of DiskLED (versions 1.x and 2.x ran one copy per counter, each with its own INI file next to the program)
struct OldCopy
{
	CString			sIni;			// The INI file of the copy (also when it was chosen by the user)
	CString			sRunName;		// The name of its Start with Windows entry (version 2 made one per copy), empty if it has none
	DiskLEDSettings	oSettings;		// What the INI file says, in the current form
};

// Look for older copies of DiskLED that have an INI file: in the Start with Windows entries of version 2 ("DiskLED - <path>"),
// in the Startup folders and among the running programs. Nothing else on the computer is searched.
// The program itself and the INI file with sOwnINIFile are left out.
void	FindOldCopies (std::vector<OldCopy>& aCopies, const CString& sOwnINIFile);

// Read the INI file of an older copy, also one that was chosen by the user. Returns FALSE if there is no such file.
BOOL	LoadOldCopy (const CString& sINIFile, OldCopy& oCopy);

// A name for lists: the folder of the copy
CString	GetOldCopyName (const OldCopy& oCopy);

// Remove Start with Windows entries (the names of the values). The copies and their files are not touched.
void	RemoveRunEntries (const CStringArray& asNames);
