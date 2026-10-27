#pragma once

#include "Settings.h"

// Icon redraw key: nothing drawn yet
#define	ICONKEY_NONE				0xFFFFFFFE

// How the icon looks
struct IconStyle
{
	DWORD		nType;				// LEDTYPE_*
	COLORREF	anColors[3];		// Low, mid and high colour
	double		adThresholds[2];	// % of the scale where the mid and the high colour start
};

// What one counter shows
struct IconCounter
{
	double		dValue;						// Current value
	double		dScale;						// Value that corresponds to the full icon
	DWORD		nValueFormat;				// VALUEFORMAT_*
	double		adHistory[HISTORY_SIZE];	// Recent values, oldest first
};

void	MakeIconStyle (const DiskLEDSettings& oSettings, IconStyle& oStyle);
int		GetColorIndex (const IconStyle& oStyle, double dNormalized);

// The number icon is too small to share between two counters: each counter gets its own tray icon
BOOL	UsesSeparateIcons (DWORD nIconType, int nCounters);

// Split a value into a displayable number and its unit (full unit for the tooltip, one character for the icon)
void	GetValueParts (DWORD nValueFormat, double dValue, double dScale, double& dNumber, CString& sUnit, CString& sIconUnit);

// Format a value for the tooltip. With bShowTotal, megabyte values also show the scale (e.g. "34.5 GB / 64.0 GB").
CString	FormatValue (DWORD nValueFormat, double dValue, double dScale, BOOL bShowTotal);

// The 100% value as the user sees and enters it, e.g. "350 Mbit/s" for bits/s or "100" for percent. A plain number is always the raw counter value.
CString	FormatScaleValue (DWORD nValueFormat, double dValue);
BOOL	ParseScaleValue (DWORD nValueFormat, const CString& sText, double& dValue);

// Identifies what the icon looks like (a hash of what would be drawn), so it only needs redrawing when this changes
DWORD	GetIconKey (const IconStyle& oStyle, const IconCounter* aCounters, int nCounters, int nSize);

// Draw the icon into a DC (nSize x nSize pixels at the origin). One counter fills the icon, two counters share it.
void	DrawIconImage (CDC* pDC, int nSize, const IconStyle& oStyle, const IconCounter* aCounters, int nCounters);

// Create a tray icon. The caller must call DestroyIcon.
HICON	CreateTrayIconImage (const IconStyle& oStyle, const IconCounter* aCounters, int nCounters);
