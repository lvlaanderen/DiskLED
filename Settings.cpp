// Settings.cpp : settings structures, defaults, validation and INI file handling
//

#include "stdafx.h"
#include "Settings.h"
#include "Helpers.h"
#include <float.h>
#include <math.h>


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
	{ SCALEMODE_AUTO_TEXT,		SCALEMODE_AUTO },
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
	for (int i = 0; i < 6; i++)
	{
		if (!_istxdigit (sColor[i]))
		{
			return FALSE;
		}
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
	oCounter.dLearnedMax	= 0;
	oCounter.bInvert		= FALSE;
	oCounter.nAlertPercent	= 0;
	oCounter.nAlertSeconds	= DEFAULT_ALERT_SECONDS;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Sensible value format, scale mode and 100% value for a counter, so users do not have to know them
//	The alert settings and the path are left alone
//
////////////////////////////////////////////////////////////////////////////////

void GetCounterDefaults (const CString& sEnglishPath, CounterSettings& oCounter)
{
	CString	sMachine;
	CString	sObject;
	CString	sInstance;
	CString	sCounter;

	// Unknown counters: a plain number that learns its maximum
	oCounter.bInvert		= FALSE;
	oCounter.dLearnedMax	= 0;
	oCounter.nValueFormat	= VALUEFORMAT_NUMBER;
	oCounter.nScaleMode		= SCALEMODE_AUTO;
	oCounter.d100Percent	= 1;

	if (!ParsePerfPath (sEnglishPath, sMachine, sObject, sInstance, sCounter))
	{
		return;
	}
	sObject.MakeLower ();
	sCounter.MakeLower ();

	if (sCounter.Find (TEXT ("%")) >= 0)
	{
		// Percentages: 100 is a full icon
		oCounter.nValueFormat	= VALUEFORMAT_PERCENT;
		oCounter.nScaleMode		= SCALEMODE_FIXED;
		oCounter.d100Percent	= 100;
	}
	else if (sObject == TEXT ("network interface") && sCounter.Left (5) == TEXT ("bytes"))
	{
		// Network speed: the adapter speed is not the internet speed, so learn the maximum. The minimum is 1 Mbit/s.
		oCounter.nValueFormat	= VALUEFORMAT_BITS;
		oCounter.d100Percent	= 125000;
	}
	else if (sCounter.Find (TEXT ("mbytes")) >= 0)
	{
		oCounter.nValueFormat	= VALUEFORMAT_MBYTES;
		if (sObject == TEXT ("memory"))
		{
			// Memory: the installed memory is a full icon
			MEMORYSTATUSEX	oMemory;

			SecureZeroMemory (&oMemory, sizeof (oMemory));
			oMemory.dwLength	= sizeof (oMemory);
			if (GlobalMemoryStatusEx (&oMemory) && oMemory.ullTotalPhys > 0)
			{
				oCounter.nScaleMode		= SCALEMODE_FIXED;
				oCounter.d100Percent	= (double) (oMemory.ullTotalPhys / (1024 * 1024));
			}
		}
	}
	else if (sCounter.Find (TEXT ("bytes/sec")) >= 0)
	{
		// Disk and other throughput: the minimum is 1 MB/s
		oCounter.nValueFormat	= VALUEFORMAT_BYTES;
		oCounter.d100Percent	= 1048576;
	}
}

void SetDefaultMeterSettings (MeterSettings& oMeter, int nId)
{
	oMeter.nId				= nId;
	oMeter.nIconType		= DEFAULT_ICONTYPE;
	oMeter.anColors[0]		= DEFAULT_COLOR_LOW;
	oMeter.anColors[1]		= DEFAULT_COLOR_MID;
	oMeter.anColors[2]		= DEFAULT_COLOR_HIGH;
	oMeter.nThresholdMid	= DEFAULT_THRESHOLD_MID;
	oMeter.nThresholdHigh	= DEFAULT_THRESHOLD_HIGH;

	SetDefaultCounterSettings (oMeter.oCounter);
	oMeter.oCounter.sPath	= DEFAULT_COUNTER_1_PATH;
}

void SetDefaultSettings (DiskLEDSettings& oSettings)
{
	MeterSettings	oMeter;

	SetDefaultMeterSettings (oMeter, 1);

	oSettings.nUpdateInterval	= DEFAULT_UPDATE_INTERVAL_MS;
	oSettings.aMeters.clear ();
	oSettings.aMeters.push_back (oMeter);
}

int GetFreeMeterId (const DiskLEDSettings& oSettings)
{
	for (int nId = 1; nId <= MAX_METERS; nId++)
	{
		BOOL	bUsed	= FALSE;

		for (size_t i = 0; i < oSettings.aMeters.size (); i++)
		{
			if (oSettings.aMeters[i].nId == nId)
			{
				bUsed	= TRUE;
			}
		}
		if (!bUsed)
		{
			return nId;
		}
	}

	return 0;
}

CString GetMeterSection (int nId)
{
	CString	sSection;

	sSection.Format (TEXT ("%s%d"), INI_METER_SECTION_PREFIX, nId);

	return sSection;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Replace invalid values with defaults
//
////////////////////////////////////////////////////////////////////////////////

void NormalizeMeterSettings (MeterSettings& oMeter)
{
	if (oMeter.nIconType < LEDTYPE_FLICKER || oMeter.nIconType > LEDTYPE_NUMBER)
	{
		oMeter.nIconType	= DEFAULT_ICONTYPE;
	}
	if (oMeter.nThresholdMid < 1 || oMeter.nThresholdMid > 100)
	{
		oMeter.nThresholdMid	= DEFAULT_THRESHOLD_MID;
	}
	if (oMeter.nThresholdHigh < oMeter.nThresholdMid || oMeter.nThresholdHigh > 100)
	{
		oMeter.nThresholdHigh	= (oMeter.nThresholdMid > DEFAULT_THRESHOLD_HIGH) ? oMeter.nThresholdMid : DEFAULT_THRESHOLD_HIGH;
	}

	CounterSettings&	oCounter	= oMeter.oCounter;

	if (oCounter.sPath.IsEmpty ())
	{
		oCounter.sPath	= DEFAULT_COUNTER_1_PATH;
	}
	if (!(oCounter.d100Percent > 0.0) || !_finite (oCounter.d100Percent) || oCounter.d100Percent > MAX_100PERCENT)
	{
		oCounter.d100Percent	= DEFAULT_100PERCENT;
	}
	if (oCounter.nValueFormat < VALUEFORMAT_PERCENT || oCounter.nValueFormat > VALUEFORMAT_MBYTES)
	{
		oCounter.nValueFormat	= VALUEFORMAT_PERCENT;
	}
	if (oCounter.nScaleMode < SCALEMODE_FIXED || oCounter.nScaleMode > SCALEMODE_AUTO)
	{
		oCounter.nScaleMode		= SCALEMODE_FIXED;
	}
	if (!(oCounter.dLearnedMax >= 0.0) || !_finite (oCounter.dLearnedMax) || oCounter.dLearnedMax > MAX_100PERCENT)
	{
		oCounter.dLearnedMax	= 0;
	}
	if (oCounter.nAlertPercent < 0)
	{
		oCounter.nAlertPercent	= 0;
	}
	if (oCounter.nAlertPercent > MAX_ALERT_PERCENT)
	{
		oCounter.nAlertPercent	= MAX_ALERT_PERCENT;
	}
	if (oCounter.nAlertSeconds < 0)
	{
		oCounter.nAlertSeconds	= 0;
	}
	if (oCounter.nAlertSeconds > MAX_ALERT_SECONDS)
	{
		oCounter.nAlertSeconds	= MAX_ALERT_SECONDS;
	}
}

void NormalizeSettings (DiskLEDSettings& oSettings)
{
	if ((int) oSettings.nUpdateInterval <= 0)
	{
		// 0 and negative numbers (read as huge unsigned values)
		oSettings.nUpdateInterval	= DEFAULT_UPDATE_INTERVAL_MS;
	}
	else if (oSettings.nUpdateInterval < MIN_UPDATE_INTERVAL_MS)
	{
		oSettings.nUpdateInterval	= MIN_UPDATE_INTERVAL_MS;
	}
	else if (oSettings.nUpdateInterval > MAX_UPDATE_INTERVAL_MS)
	{
		oSettings.nUpdateInterval	= MAX_UPDATE_INTERVAL_MS;
	}

	// Meters with a number that is out of range or already taken get the next free one, and too many meters are dropped
	std::vector<MeterSettings>	aMeters;

	for (size_t i = 0; i < oSettings.aMeters.size () && aMeters.size () < MAX_METERS; i++)
	{
		MeterSettings	oMeter	= oSettings.aMeters[i];
		BOOL			bTaken	= FALSE;

		for (size_t j = 0; j < aMeters.size (); j++)
		{
			if (aMeters[j].nId == oMeter.nId)
			{
				bTaken	= TRUE;
			}
		}

		if (oMeter.nId < 1 || oMeter.nId > MAX_METERS || bTaken)
		{
			DiskLEDSettings	oTemp;

			oTemp.aMeters	= aMeters;
			oMeter.nId		= GetFreeMeterId (oTemp);
		}
		NormalizeMeterSettings (oMeter);
		aMeters.push_back (oMeter);
	}

	// Sorted by number, so the order does not depend on the order in the file
	for (size_t i = 1; i < aMeters.size (); i++)
	{
		for (size_t j = i; j > 0 && aMeters[j].nId < aMeters[j - 1].nId; j--)
		{
			MeterSettings	oSwap	= aMeters[j];

			aMeters[j]		= aMeters[j - 1];
			aMeters[j - 1]	= oSwap;
		}
	}

	if (aMeters.empty ())
	{
		MeterSettings	oMeter;

		SetDefaultMeterSettings (oMeter, 1);
		aMeters.push_back (oMeter);
	}

	oSettings.aMeters	= aMeters;
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
	if (ReadINIValue (sINIFile, sSection, INI_VALUE_LEARNEDMAX, &sData) == ERROR_SUCCESS)
	{
		oCounter.dLearnedMax	= _tstof (sData);
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
//	Read the look of a meter (icon type, colours, thresholds) from an INI section. Values not found keep their previous value.
//
////////////////////////////////////////////////////////////////////////////////

static void LoadMeterStyle (const CString& sINIFile, const CString& sSection, MeterSettings& oMeter)
{
	CString	sData;

	if (ReadINIValue (sINIFile, sSection, INI_VALUE_LEDTYPE, &sData) == ERROR_SUCCESS)
	{
		oMeter.nIconType	= LookupValue (g_aIconTypes, sData, oMeter.nIconType);
	}

	const TCHAR*	asColorKeys[3]	= { INI_VALUE_COLOR_LOW, INI_VALUE_COLOR_MID, INI_VALUE_COLOR_HIGH };
	for (int i = 0; i < 3; i++)
	{
		if (ReadINIValue (sINIFile, sSection, asColorKeys[i], &sData) == ERROR_SUCCESS)
		{
			ParseColor (sData, oMeter.anColors[i]);
		}
	}
	if (ReadINIValue (sINIFile, sSection, INI_VALUE_THRESHOLD_MID, &sData) == ERROR_SUCCESS)
	{
		oMeter.nThresholdMid	= _ttoi (sData);
	}
	if (ReadINIValue (sINIFile, sSection, INI_VALUE_THRESHOLD_HIGH, &sData) == ERROR_SUCCESS)
	{
		oMeter.nThresholdHigh	= _ttoi (sData);
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Files of the versions up to 2.1: the first section also holds what both counters share, a second counter is a second section.
//	Every counter becomes a meter with the shared look.
//
////////////////////////////////////////////////////////////////////////////////

static void LoadLegacySettings (const CString& sINIFile, DiskLEDSettings& oSettings)
{
	CString			sData;
	MeterSettings	oMeter;
	BOOL			bSecondCounter	= FALSE;

	SetDefaultMeterSettings (oMeter, 1);

	if (ReadINIValue (sINIFile, INI_LEGACY_COUNTER_1_SECTION, INI_VALUE_UPDATE_INTERVAL, &sData) == ERROR_SUCCESS)
	{
		oSettings.nUpdateInterval	= (DWORD) _ttoi (sData);
	}
	if (ReadINIValue (sINIFile, INI_LEGACY_COUNTER_1_SECTION, INI_VALUE_SECOND_COUNTER, &sData) == ERROR_SUCCESS)
	{
		bSecondCounter	= (_ttoi (sData) != 0);
	}

	LoadMeterStyle (sINIFile, INI_LEGACY_COUNTER_1_SECTION, oMeter);
	LoadCounterSettings (sINIFile, INI_LEGACY_COUNTER_1_SECTION, oMeter.oCounter);
	oSettings.aMeters.clear ();
	oSettings.aMeters.push_back (oMeter);

	if (bSecondCounter)
	{
		MeterSettings	oSecond	= oMeter;

		SetDefaultCounterSettings (oSecond.oCounter);
		oSecond.nId				= 2;
		oSecond.oCounter.sPath	= TEXT ("");
		LoadCounterSettings (sINIFile, INI_LEGACY_COUNTER_2_SECTION, oSecond.oCounter);
		if (!oSecond.oCounter.sPath.IsEmpty ())
		{
			oSettings.aMeters.push_back (oSecond);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	The number of a "Meter <number>" section name, 0 if it is another section
//
////////////////////////////////////////////////////////////////////////////////

static int GetMeterIdOfSection (const CString& sSection)
{
	CString	sPrefix	= INI_METER_SECTION_PREFIX;

	if (sSection.GetLength () <= sPrefix.GetLength () || sSection.Left (sPrefix.GetLength ()).CompareNoCase (sPrefix) != 0)
	{
		return 0;
	}

	CString	sNumber	= sSection.Mid (sPrefix.GetLength ());
	int		nId		= _ttoi (sNumber);

	// Only plain numbers: "Meter 2 old" is not a meter
	CString	sCheck;
	sCheck.Format (TEXT ("%d"), nId);

	return (sCheck == sNumber && nId >= 1 && nId <= MAX_METERS) ? nId : 0;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Read all settings from the INI file. Settings not found get their default value.
//
////////////////////////////////////////////////////////////////////////////////

DWORD LoadSettings (const CString& sINIFile, DiskLEDSettings& oSettings)
{
	CString			sData;
	CStringArray	asSections;

	SetDefaultSettings (oSettings);

	GetINISectionNames (sINIFile, asSections);

	std::vector<MeterSettings>	aMeters;

	for (int i = 0; i < asSections.GetSize (); i++)
	{
		int	nId	= GetMeterIdOfSection (asSections[i]);

		// A section without a counter path is not a meter (e.g. only a learned maximum written for a meter that was never saved)
		if (nId == 0 || ReadINIValue (sINIFile, asSections[i], INI_VALUE_PATH, &sData) != ERROR_SUCCESS)
		{
			continue;
		}

		MeterSettings	oMeter;

		SetDefaultMeterSettings (oMeter, nId);
		LoadMeterStyle (sINIFile, asSections[i], oMeter);
		LoadCounterSettings (sINIFile, asSections[i], oMeter.oCounter);
		aMeters.push_back (oMeter);
	}

	if (!aMeters.empty ())
	{
		oSettings.aMeters	= aMeters;
		if (ReadINIValue (sINIFile, INI_GENERAL_SECTION, INI_VALUE_UPDATE_INTERVAL, &sData) == ERROR_SUCCESS)
		{
			oSettings.nUpdateInterval	= (DWORD) _ttoi (sData);
		}
	}
	else if (ReadINIValue (sINIFile, INI_LEGACY_COUNTER_1_SECTION, INI_VALUE_PATH, &sData) == ERROR_SUCCESS)
	{
		LoadLegacySettings (sINIFile, oSettings);
	}

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
	if (nError == ERROR_SUCCESS && oCounter.nScaleMode == SCALEMODE_AUTO)
	{
		nError	= WriteINIValue (sINIFile, sSection, INI_VALUE_LEARNEDMAX, (double) oCounter.dLearnedMax);
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
//	Write the look of a meter and its counter to an INI section. Returns the first error.
//
////////////////////////////////////////////////////////////////////////////////

static DWORD SaveMeterSettings (const CString& sINIFile, const MeterSettings& oMeter)
{
	CString	sSection	= GetMeterSection (oMeter.nId);
	DWORD	nError		= WriteINIValue (sINIFile, sSection, INI_VALUE_LEDTYPE, CString (LookupName (g_aIconTypes, oMeter.nIconType)));

	const TCHAR*	asColorKeys[3]	= { INI_VALUE_COLOR_LOW, INI_VALUE_COLOR_MID, INI_VALUE_COLOR_HIGH };
	for (int i = 0; i < 3 && nError == ERROR_SUCCESS; i++)
	{
		nError	= WriteINIValue (sINIFile, sSection, asColorKeys[i], FormatColor (oMeter.anColors[i]));
	}
	if (nError == ERROR_SUCCESS)
	{
		nError	= WriteINIValue (sINIFile, sSection, INI_VALUE_THRESHOLD_MID, oMeter.nThresholdMid);
	}
	if (nError == ERROR_SUCCESS)
	{
		nError	= WriteINIValue (sINIFile, sSection, INI_VALUE_THRESHOLD_HIGH, oMeter.nThresholdHigh);
	}
	if (nError == ERROR_SUCCESS)
	{
		nError	= SaveCounterSettings (sINIFile, sSection, oMeter.oCounter);
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
	DWORD	nError	= CreateParentFolder (sINIFile);

	if (nError == ERROR_SUCCESS)
	{
		nError	= WriteINIValue (sINIFile, INI_GENERAL_SECTION, INI_VALUE_UPDATE_INTERVAL, (int) oSettings.nUpdateInterval);
	}

	for (size_t i = 0; i < oSettings.aMeters.size () && nError == ERROR_SUCCESS; i++)
	{
		nError	= SaveMeterSettings (sINIFile, oSettings.aMeters[i]);
	}

	// Remove what is no longer there: meters that were deleted, and the sections of the older versions (they would be read again if the new sections were gone)
	if (nError == ERROR_SUCCESS)
	{
		CStringArray	asSections;

		GetINISectionNames (sINIFile, asSections);
		for (int i = 0; i < asSections.GetSize () && nError == ERROR_SUCCESS; i++)
		{
			int		nId		= GetMeterIdOfSection (asSections[i]);
			BOOL	bKeep	= FALSE;

			for (size_t j = 0; j < oSettings.aMeters.size (); j++)
			{
				if (oSettings.aMeters[j].nId == nId)
				{
					bKeep	= TRUE;
				}
			}

			BOOL	bLegacy	= (asSections[i].CompareNoCase (INI_LEGACY_COUNTER_1_SECTION) == 0 || asSections[i].CompareNoCase (INI_LEGACY_COUNTER_2_SECTION) == 0);

			if ((nId != 0 && !bKeep) || bLegacy)
			{
				nError	= DeleteINISection (sINIFile, asSections[i]);
			}
		}
	}

	return nError;
}
