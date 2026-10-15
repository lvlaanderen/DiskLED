// Settings.cpp : settings structures, defaults, validation and INI file handling
//

#include "stdafx.h"
#include "Settings.h"
#include "Helpers.h"


struct NameValue
{
	const TCHAR*	sName;
	DWORD			nValue;
};

static const NameValue g_aIconTypes[] =
{
	{ LEDTYPE_FLICKER_TEXT,		LEDTYPE_FLICKER },
	{ LEDTYPE_COLOREDBAR_TEXT,	LEDTYPE_COLOREDBAR },
	{ LEDTYPE_CLEANBAR_TEXT,	LEDTYPE_CLEANBAR },
	{ LEDTYPE_VUMETER_TEXT,		LEDTYPE_VUMETER },
	{ LEDTYPE_GRAPH_TEXT,		LEDTYPE_GRAPH },
	{ LEDTYPE_NUMBER_TEXT,		LEDTYPE_NUMBER },
	{ NULL,						0 }
};

static const NameValue g_aValueFormats[] =
{
	{ VALUEFORMAT_PERCENT_TEXT,	VALUEFORMAT_PERCENT },
	{ VALUEFORMAT_NUMBER_TEXT,	VALUEFORMAT_NUMBER },
	{ VALUEFORMAT_BYTES_TEXT,	VALUEFORMAT_BYTES },
	{ VALUEFORMAT_BITS_TEXT,	VALUEFORMAT_BITS },
	{ VALUEFORMAT_MBYTES_TEXT,	VALUEFORMAT_MBYTES },
	{ NULL,						0 }
};

static const NameValue g_aScaleModes[] =
{
	{ SCALEMODE_FIXED_TEXT,		SCALEMODE_FIXED },
	{ SCALEMODE_PEAK_TEXT,		SCALEMODE_PEAK },
	{ SCALEMODE_DECAY_TEXT,		SCALEMODE_DECAY },
	{ NULL,						0 }
};

// Value for a name (case-insensitive), or nDefault if there is no such name
static DWORD LookupValue (const NameValue* aTable, const CString& sName, DWORD nDefault)
{
	for (; aTable->sName != NULL; aTable++)
	{
		if (sName.CompareNoCase (aTable->sName) == 0)
		{
			return aTable->nValue;
		}
	}

	return nDefault;
}

// Name for a value, or the name of the first entry if there is no such value
static const TCHAR* LookupName (const NameValue* aTable, DWORD nValue)
{
	for (const NameValue* pEntry = aTable; pEntry->sName != NULL; pEntry++)
	{
		if (pEntry->nValue == nValue)
		{
			return pEntry->sName;
		}
	}

	return aTable->sName;
}

static BOOL ParseColor (const CString& sColor, COLORREF& nColor)
{
	if (sColor.GetLength () != 6)
	{
		return FALSE;
	}

	TCHAR*			pEnd	= NULL;
	unsigned long	nRGB	= _tcstoul (sColor, &pEnd, 16);
	if (pEnd == NULL || *pEnd != 0)
	{
		return FALSE;
	}

	nColor	= RGB ((nRGB >> 16) & 0xFF, (nRGB >> 8) & 0xFF, nRGB & 0xFF);

	return TRUE;
}

static CString FormatColor (COLORREF nColor)
{
	CString	sColor;

	sColor.Format (TEXT ("%02X%02X%02X"), GetRValue (nColor), GetGValue (nColor), GetBValue (nColor));

	return sColor;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Defaults
//
////////////////////////////////////////////////////////////////////////////////

void SetDefaultCounterSettings (CounterSettings& oCounter)
{
	oCounter.sPath			= TEXT ("");
	oCounter.d100Percent	= DEFAULT_100PERCENT;
	oCounter.nValueFormat	= VALUEFORMAT_PERCENT;
	oCounter.nScaleMode		= SCALEMODE_FIXED;
	oCounter.bInvert		= FALSE;
	oCounter.nAlertPercent	= 0;
	oCounter.nAlertSeconds	= DEFAULT_ALERT_SECONDS;
}

void SetDefaultSettings (DiskLEDSettings& oSettings)
{
	oSettings.nIconType			= DEFAULT_ICONTYPE;
	oSettings.nUpdateInterval	= DEFAULT_UPDATE_INTERVAL_MS;
	oSettings.bSecondCounter	= FALSE;
	oSettings.anColors[0]		= DEFAULT_COLOR_LOW;
	oSettings.anColors[1]		= DEFAULT_COLOR_MID;
	oSettings.anColors[2]		= DEFAULT_COLOR_HIGH;
	oSettings.nThresholdMid		= DEFAULT_THRESHOLD_MID;
	oSettings.nThresholdHigh	= DEFAULT_THRESHOLD_HIGH;

	SetDefaultCounterSettings (oSettings.aCounters[0]);
	SetDefaultCounterSettings (oSettings.aCounters[1]);
	oSettings.aCounters[0].sPath	= DEFAULT_COUNTER_1_PATH;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Replace invalid values with defaults
//
////////////////////////////////////////////////////////////////////////////////

void NormalizeSettings (DiskLEDSettings& oSettings)
{
	if (oSettings.nIconType < LEDTYPE_FLICKER || oSettings.nIconType > LEDTYPE_NUMBER)
	{
		oSettings.nIconType	= DEFAULT_ICONTYPE;
	}
	if (oSettings.nUpdateInterval == 0)
	{
		oSettings.nUpdateInterval	= DEFAULT_UPDATE_INTERVAL_MS;
	}

	if (oSettings.nThresholdMid < 1 || oSettings.nThresholdMid > 100)
	{
		oSettings.nThresholdMid	= DEFAULT_THRESHOLD_MID;
	}
	if (oSettings.nThresholdHigh < oSettings.nThresholdMid || oSettings.nThresholdHigh > 100)
	{
		oSettings.nThresholdHigh	= (oSettings.nThresholdMid > DEFAULT_THRESHOLD_HIGH) ? oSettings.nThresholdMid : DEFAULT_THRESHOLD_HIGH;
	}

	for (int i = 0; i < 2; i++)
	{
		CounterSettings&	oCounter	= oSettings.aCounters[i];

		if (i == 0 && oCounter.sPath.IsEmpty ())
		{
			oCounter.sPath	= DEFAULT_COUNTER_1_PATH;
		}
		if (oCounter.d100Percent <= 0.0)
		{
			oCounter.d100Percent	= DEFAULT_100PERCENT;
		}
		if (oCounter.nValueFormat < VALUEFORMAT_PERCENT || oCounter.nValueFormat > VALUEFORMAT_MBYTES)
		{
			oCounter.nValueFormat	= VALUEFORMAT_PERCENT;
		}
		if (oCounter.nScaleMode < SCALEMODE_FIXED || oCounter.nScaleMode > SCALEMODE_DECAY)
		{
			oCounter.nScaleMode		= SCALEMODE_FIXED;
		}
		if (oCounter.nAlertPercent < 0)
		{
			oCounter.nAlertPercent	= 0;
		}
		if (oCounter.nAlertSeconds < 0)
		{
			oCounter.nAlertSeconds	= 0;
		}
	}

	// A second counter needs a path
	if (oSettings.aCounters[1].sPath.IsEmpty ())
	{
		oSettings.bSecondCounter	= FALSE;
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Read the settings of one counter from an INI section. Values not found keep their previous value.
//
////////////////////////////////////////////////////////////////////////////////

static void LoadCounterSettings (const CString& sINIFile, const CString& sSection, CounterSettings& oCounter)
{
	CString	sData;

	if (ReadINIValue (sINIFile, sSection, INI_VALUE_PATH, &sData) == ERROR_SUCCESS)
	{
		oCounter.sPath	= sData;
	}
	if (ReadINIValue (sINIFile, sSection, INI_VALUE_100PERCENT, &sData) == ERROR_SUCCESS)
	{
		oCounter.d100Percent	= _tstof (sData);
	}
	if (ReadINIValue (sINIFile, sSection, INI_VALUE_VALUEFORMAT, &sData) == ERROR_SUCCESS)
	{
		oCounter.nValueFormat	= LookupValue (g_aValueFormats, sData, oCounter.nValueFormat);
	}

	// Older versions only had AutoScale (1 = peak). ScaleMode wins if both are present.
	if (ReadINIValue (sINIFile, sSection, INI_VALUE_AUTOSCALE, &sData) == ERROR_SUCCESS)
	{
		oCounter.nScaleMode	= (_ttoi (sData) != 0) ? SCALEMODE_PEAK : SCALEMODE_FIXED;
	}
	if (ReadINIValue (sINIFile, sSection, INI_VALUE_SCALEMODE, &sData) == ERROR_SUCCESS)
	{
		oCounter.nScaleMode	= LookupValue (g_aScaleModes, sData, oCounter.nScaleMode);
	}

	if (ReadINIValue (sINIFile, sSection, INI_VALUE_INVERT, &sData) == ERROR_SUCCESS)
	{
		oCounter.bInvert	= (_ttoi (sData) != 0);
	}
	if (ReadINIValue (sINIFile, sSection, INI_VALUE_ALERT_PERCENT, &sData) == ERROR_SUCCESS)
	{
		oCounter.nAlertPercent	= _ttoi (sData);
	}
	if (ReadINIValue (sINIFile, sSection, INI_VALUE_ALERT_SECONDS, &sData) == ERROR_SUCCESS)
	{
		oCounter.nAlertSeconds	= _ttoi (sData);
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Read all settings from the INI file. Settings not found get their default value.
//
////////////////////////////////////////////////////////////////////////////////

DWORD LoadSettings (const CString& sINIFile, DiskLEDSettings& oSettings)
{
	CString	sData;

	SetDefaultSettings (oSettings);

	if (ReadINIValue (sINIFile, INI_COUNTER_1_SECTION, INI_VALUE_LEDTYPE, &sData) == ERROR_SUCCESS)
	{
		oSettings.nIconType	= LookupValue (g_aIconTypes, sData, oSettings.nIconType);
	}
	if (ReadINIValue (sINIFile, INI_COUNTER_1_SECTION, INI_VALUE_UPDATE_INTERVAL, &sData) == ERROR_SUCCESS)
	{
		oSettings.nUpdateInterval	= (DWORD) _ttoi (sData);
	}
	if (ReadINIValue (sINIFile, INI_COUNTER_1_SECTION, INI_VALUE_SECOND_COUNTER, &sData) == ERROR_SUCCESS)
	{
		oSettings.bSecondCounter	= (_ttoi (sData) != 0);
	}

	const TCHAR*	asColorKeys[3]	= { INI_VALUE_COLOR_LOW, INI_VALUE_COLOR_MID, INI_VALUE_COLOR_HIGH };
	for (int i = 0; i < 3; i++)
	{
		if (ReadINIValue (sINIFile, INI_COUNTER_1_SECTION, asColorKeys[i], &sData) == ERROR_SUCCESS)
		{
			ParseColor (sData, oSettings.anColors[i]);
		}
	}
	if (ReadINIValue (sINIFile, INI_COUNTER_1_SECTION, INI_VALUE_THRESHOLD_MID, &sData) == ERROR_SUCCESS)
	{
		oSettings.nThresholdMid	= _ttoi (sData);
	}
	if (ReadINIValue (sINIFile, INI_COUNTER_1_SECTION, INI_VALUE_THRESHOLD_HIGH, &sData) == ERROR_SUCCESS)
	{
		oSettings.nThresholdHigh	= _ttoi (sData);
	}

	LoadCounterSettings (sINIFile, INI_COUNTER_1_SECTION, oSettings.aCounters[0]);
	LoadCounterSettings (sINIFile, INI_COUNTER_2_SECTION, oSettings.aCounters[1]);

	NormalizeSettings (oSettings);

	return ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Write the settings of one counter to an INI section. Returns the first error.
//
////////////////////////////////////////////////////////////////////////////////

static DWORD SaveCounterSettings (const CString& sINIFile, const CString& sSection, const CounterSettings& oCounter)
{
	DWORD	nError	= WriteINIValue (sINIFile, sSection, INI_VALUE_PATH, oCounter.sPath);

	if (nError == ERROR_SUCCESS)
	{
		nError	= WriteINIValue (sINIFile, sSection, INI_VALUE_100PERCENT, (double) oCounter.d100Percent);
	}
	if (nError == ERROR_SUCCESS)
	{
		nError	= WriteINIValue (sINIFile, sSection, INI_VALUE_VALUEFORMAT, CString (LookupName (g_aValueFormats, oCounter.nValueFormat)));
	}
	if (nError == ERROR_SUCCESS)
	{
		nError	= WriteINIValue (sINIFile, sSection, INI_VALUE_SCALEMODE, CString (LookupName (g_aScaleModes, oCounter.nScaleMode)));
	}
	if (nError == ERROR_SUCCESS)
	{
		nError	= WriteINIValue (sINIFile, sSection, INI_VALUE_INVERT, (int) (oCounter.bInvert != 0));
	}
	if (nError == ERROR_SUCCESS)
	{
		nError	= WriteINIValue (sINIFile, sSection, INI_VALUE_ALERT_PERCENT, (int) oCounter.nAlertPercent);
	}
	if (nError == ERROR_SUCCESS)
	{
		nError	= WriteINIValue (sINIFile, sSection, INI_VALUE_ALERT_SECONDS, (int) oCounter.nAlertSeconds);
	}

	return nError;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Write all settings to the INI file. Returns the first error.
//
////////////////////////////////////////////////////////////////////////////////

DWORD SaveSettings (const CString& sINIFile, const DiskLEDSettings& oSettings)
{
	DWORD	nError	= WriteINIValue (sINIFile, INI_COUNTER_1_SECTION, INI_VALUE_LEDTYPE, CString (LookupName (g_aIconTypes, oSettings.nIconType)));

	if (nError == ERROR_SUCCESS)
	{
		nError	= WriteINIValue (sINIFile, INI_COUNTER_1_SECTION, INI_VALUE_UPDATE_INTERVAL, (int) oSettings.nUpdateInterval);
	}
	if (nError == ERROR_SUCCESS)
	{
		nError	= WriteINIValue (sINIFile, INI_COUNTER_1_SECTION, INI_VALUE_SECOND_COUNTER, (int) (oSettings.bSecondCounter != 0));
	}

	const TCHAR*	asColorKeys[3]	= { INI_VALUE_COLOR_LOW, INI_VALUE_COLOR_MID, INI_VALUE_COLOR_HIGH };
	for (int i = 0; i < 3 && nError == ERROR_SUCCESS; i++)
	{
		nError	= WriteINIValue (sINIFile, INI_COUNTER_1_SECTION, asColorKeys[i], FormatColor (oSettings.anColors[i]));
	}
	if (nError == ERROR_SUCCESS)
	{
		nError	= WriteINIValue (sINIFile, INI_COUNTER_1_SECTION, INI_VALUE_THRESHOLD_MID, oSettings.nThresholdMid);
	}
	if (nError == ERROR_SUCCESS)
	{
		nError	= WriteINIValue (sINIFile, INI_COUNTER_1_SECTION, INI_VALUE_THRESHOLD_HIGH, oSettings.nThresholdHigh);
	}

	if (nError == ERROR_SUCCESS)
	{
		nError	= SaveCounterSettings (sINIFile, INI_COUNTER_1_SECTION, oSettings.aCounters[0]);
	}

	// The second counter is only stored once it has a path
	if (nError == ERROR_SUCCESS && !oSettings.aCounters[1].sPath.IsEmpty ())
	{
		nError	= SaveCounterSettings (sINIFile, INI_COUNTER_2_SECTION, oSettings.aCounters[1]);
	}

	return nError;
}
