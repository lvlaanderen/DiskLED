// Settings.cpp : settings structures, defaults, validation and INI file handling
//

#include "stdafx.h"
#include "Settings.h"
#include "Helpers.h"
#include "IconRenderer.h"
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
	{ VALUEFORMAT_CELSIUS_TEXT,	VALUEFORMAT_CELSIUS },
	{ VALUEFORMAT_FAHRENHEIT_TEXT,	VALUEFORMAT_FAHRENHEIT },
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
	oCounter.nUpdateInterval	= DEFAULT_UPDATE_INTERVAL_MS;
}


DWORD GetDefaultTemperatureFormat ()
{
	DWORD	nMeasure	= 0;

	if (GetLocaleInfoEx (LOCALE_NAME_USER_DEFAULT, LOCALE_IMEASURE | LOCALE_RETURN_NUMBER, (LPWSTR) &nMeasure, sizeof (nMeasure) / sizeof (WCHAR)) > 0 && nMeasure == 1)
	{
		return VALUEFORMAT_FAHRENHEIT;
	}

	return VALUEFORMAT_CELSIUS;
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
	oCounter.nUpdateInterval	= SLOW_UPDATE_INTERVAL_MS;

	if (!ParsePerfPath (sEnglishPath, sMachine, sObject, sInstance, sCounter))
	{
		return;
	}
	sObject.MakeLower ();
	sCounter.MakeLower ();

	// How often to read: the disk LED flickers with every access, a processor needs a little time to give a meaningful value, speeds and amounts change slowly
	if (sObject == TEXT ("physicaldisk") || sObject == TEXT ("logicaldisk"))
	{
		oCounter.nUpdateInterval	= DEFAULT_UPDATE_INTERVAL_MS;
	}
	else if (sObject.Left (9) == TEXT ("processor"))
	{
		oCounter.nUpdateInterval	= PROCESSOR_UPDATE_INTERVAL_MS;
	}

	// A disk's idle time is shown as its active time, like in Task Manager
	if ((sObject == TEXT ("physicaldisk") || sObject == TEXT ("logicaldisk")) && sCounter == TEXT ("% idle time"))
	{
		oCounter.bInvert	= TRUE;
	}

	if ((sObject == TEXT ("thermal zone information") && sCounter.Find (TEXT ("temperature")) >= 0) || sObject == TEXT ("diskled gpu"))
	{
		// Temperatures are kept in Celsius, in the format of the user's region. A full icon is 100 degrees Celsius. The firmware updates them slowly.
		oCounter.nValueFormat	= GetDefaultTemperatureFormat ();
		oCounter.nScaleMode		= SCALEMODE_FIXED;
		oCounter.d100Percent	= 100;
	}
	else if (sCounter.Find (TEXT ("%")) >= 0)
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


////////////////////////////////////////////////////////////////////////////////
//
//	The counter as shown in the meter list, tooltips and alerts. An inverted counter is named after what it then shows:
//	% Idle Time becomes % Active Time (like Task Manager), Available MBytes becomes Used MBytes. Others get "100% minus" in front.
//
////////////////////////////////////////////////////////////////////////////////

CString GetInvertPrefix (const CounterSettings& oCounter)
{
	CString	sFull	= FormatScaleValue (oCounter.nValueFormat, oCounter.d100Percent);

	if (oCounter.nValueFormat == VALUEFORMAT_PERCENT)
	{
		sFull	+= TEXT ("%");
	}

	return sFull + TEXT (" minus ");
}

CString GetCounterDisplayName (const CounterSettings& oCounter, BOOL bWithInstance)
{
	static const LPCTSTR	aasInverted[][2]	=
	{
		{ TEXT ("% Idle Time"),			TEXT ("% Active Time") },
		{ TEXT ("% Free Space"),		TEXT ("% Used Space") },
		{ TEXT ("Free Megabytes"),		TEXT ("Used Megabytes") },
		{ TEXT ("Available Bytes"),		TEXT ("Used Bytes") },
		{ TEXT ("Available KBytes"),	TEXT ("Used KBytes") },
		{ TEXT ("Available MBytes"),	TEXT ("Used MBytes") },
	};
	CString	sMachine;
	CString	sObject;
	CString	sInstance;
	CString	sCounter;
	CString	sLabel;

	if (!ParsePerfPath (oCounter.sPath, sMachine, sObject, sInstance, sCounter))
	{
		sCounter	= oCounter.sPath;
	}
	sLabel	= sCounter;

	if (oCounter.bInvert)
	{
		CString	sEnglishMachine;
		CString	sEnglishObject;
		CString	sEnglishInstance;
		CString	sEnglishCounter;

		// Older INI files contain paths in the system language
		if (!ParsePerfPath (LocalPathToEnglish (oCounter.sPath), sEnglishMachine, sEnglishObject, sEnglishInstance, sEnglishCounter))
		{
			sEnglishCounter	= sCounter;
		}

		sLabel	= GetInvertPrefix (oCounter) + sCounter;
		for (int i = 0; i < _countof (aasInverted); i++)
		{
			if (sEnglishCounter.CompareNoCase (aasInverted[i][0]) == 0)
			{
				sLabel	= aasInverted[i][1];
				break;
			}
		}
	}

	if (bWithInstance && !sInstance.IsEmpty ())
	{
		sLabel	+= TEXT (" (") + sInstance + TEXT (")");
	}

	return sLabel;
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
	oMeter.oCounter.bInvert	= DEFAULT_COUNTER_1_INVERT;
}

void SetDefaultSettings (DiskLEDSettings& oSettings)
{
	MeterSettings	oMeter;

	SetDefaultMeterSettings (oMeter, 1);

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
	oMeter.sName.Trim ();
	if (oMeter.sName.GetLength () > MAX_METER_NAME)
	{
		oMeter.sName.Truncate (MAX_METER_NAME);
	}
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
		oCounter.sPath		= DEFAULT_COUNTER_1_PATH;
		oCounter.bInvert	= DEFAULT_COUNTER_1_INVERT;
	}
	if (!(oCounter.d100Percent > 0.0) || !_finite (oCounter.d100Percent) || oCounter.d100Percent > MAX_100PERCENT)
	{
		oCounter.d100Percent	= DEFAULT_100PERCENT;
	}
	if (oCounter.nValueFormat < VALUEFORMAT_PERCENT || oCounter.nValueFormat > VALUEFORMAT_FAHRENHEIT)
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
	if ((int) oCounter.nUpdateInterval <= 0)
	{
		// 0 and negative numbers (read as huge unsigned values)
		oCounter.nUpdateInterval	= DEFAULT_UPDATE_INTERVAL_MS;
	}
	else if (oCounter.nUpdateInterval < MIN_UPDATE_INTERVAL_MS)
	{
		oCounter.nUpdateInterval	= MIN_UPDATE_INTERVAL_MS;
	}
	else if (oCounter.nUpdateInterval > MAX_UPDATE_INTERVAL_MS)
	{
		oCounter.nUpdateInterval	= MAX_UPDATE_INTERVAL_MS;
	}
}

void NormalizeSettings (DiskLEDSettings& oSettings)
{
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
//	Are the settings the same? Both are corrected like they would be when used. The learned maximum is left out, it changes while running.
//
////////////////////////////////////////////////////////////////////////////////

BOOL AreSettingsEqual (const DiskLEDSettings& oFirst, const DiskLEDSettings& oSecond)
{
	DiskLEDSettings	oA	= oFirst;
	DiskLEDSettings	oB	= oSecond;

	NormalizeSettings (oA);
	NormalizeSettings (oB);

	if (oA.aMeters.size () != oB.aMeters.size ())
	{
		return FALSE;
	}

	for (size_t i = 0; i < oA.aMeters.size (); i++)
	{
		const MeterSettings&	oMeterA	= oA.aMeters[i];
		const MeterSettings&	oMeterB	= oB.aMeters[i];
		const CounterSettings&	oCounterA	= oMeterA.oCounter;
		const CounterSettings&	oCounterB	= oMeterB.oCounter;

		if (oMeterA.nId != oMeterB.nId || oMeterA.sName != oMeterB.sName || oMeterA.nIconType != oMeterB.nIconType
			|| oMeterA.nThresholdMid != oMeterB.nThresholdMid || oMeterA.nThresholdHigh != oMeterB.nThresholdHigh
			|| oMeterA.anColors[0] != oMeterB.anColors[0] || oMeterA.anColors[1] != oMeterB.anColors[1] || oMeterA.anColors[2] != oMeterB.anColors[2]
			|| oCounterA.sPath.CompareNoCase (oCounterB.sPath) != 0 || oCounterA.d100Percent != oCounterB.d100Percent
			|| oCounterA.nValueFormat != oCounterB.nValueFormat || oCounterA.nScaleMode != oCounterB.nScaleMode
			|| (oCounterA.bInvert != 0) != (oCounterB.bInvert != 0) || oCounterA.nAlertPercent != oCounterB.nAlertPercent
			|| oCounterA.nAlertSeconds != oCounterB.nAlertSeconds || oCounterA.nUpdateInterval != oCounterB.nUpdateInterval)
		{
			return FALSE;
		}
	}

	return TRUE;
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
	if (ReadINIValue (sINIFile, sSection, INI_VALUE_UPDATE_INTERVAL, &sData) == ERROR_SUCCESS)
	{
		oCounter.nUpdateInterval	= (DWORD) _ttoi (sData);
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

	if (ReadINIValue (sINIFile, sSection, INI_VALUE_NAME, &sData) == ERROR_SUCCESS)
	{
		oMeter.sName	= sData;
	}
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

	if (ReadINIValue (sINIFile, INI_LEGACY_COUNTER_1_SECTION, INI_VALUE_SECOND_COUNTER, &sData) == ERROR_SUCCESS)
	{
		bSecondCounter	= (_ttoi (sData) != 0);
	}

	// The first section also holds the update interval that both counters shared
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
		oSecond.oCounter.nUpdateInterval	= oMeter.oCounter.nUpdateInterval;
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

	// Version 3.0 had one update interval for all meters: it is the interval of the meters that do not have their own
	DWORD	nGeneralInterval	= DEFAULT_UPDATE_INTERVAL_MS;

	if (ReadINIValue (sINIFile, INI_GENERAL_SECTION, INI_VALUE_UPDATE_INTERVAL, &sData) == ERROR_SUCCESS)
	{
		nGeneralInterval	= (DWORD) _ttoi (sData);
	}

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
		oMeter.oCounter.nUpdateInterval	= nGeneralInterval;
		LoadMeterStyle (sINIFile, asSections[i], oMeter);
		LoadCounterSettings (sINIFile, asSections[i], oMeter.oCounter);
		aMeters.push_back (oMeter);
	}

	if (!aMeters.empty ())
	{
		oSettings.aMeters	= aMeters;
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
	if (nError == ERROR_SUCCESS)
	{
		nError	= WriteINIValue (sINIFile, sSection, INI_VALUE_UPDATE_INTERVAL, (int) oCounter.nUpdateInterval);
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
	DWORD	nError		= WriteINIValue (sINIFile, sSection, INI_VALUE_NAME, oMeter.sName);

	if (nError == ERROR_SUCCESS)
	{
		nError	= WriteINIValue (sINIFile, sSection, INI_VALUE_LEDTYPE, CString (LookupName (g_aIconTypes, oMeter.nIconType)));
	}

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

	for (size_t i = 0; i < oSettings.aMeters.size () && nError == ERROR_SUCCESS; i++)
	{
		nError	= SaveMeterSettings (sINIFile, oSettings.aMeters[i]);
	}

	// Every meter has its own update interval now, the shared one of version 3.0 is no longer used
	if (nError == ERROR_SUCCESS)
	{
		WritePrivateProfileString (INI_GENERAL_SECTION, INI_VALUE_UPDATE_INTERVAL, NULL, sINIFile);
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
