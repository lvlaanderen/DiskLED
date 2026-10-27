#pragma once

// Icon type constants (the numeric values are internal, the INI file uses the text)
#define	LEDTYPE_FLICKER				1
#define	LEDTYPE_FLICKER_TEXT		TEXT ("Flicker")
#define	LEDTYPE_COLOREDBAR			2
#define	LEDTYPE_COLOREDBAR_TEXT		TEXT ("ColoredBar")
#define	LEDTYPE_CLEANBAR			3
#define	LEDTYPE_CLEANBAR_TEXT		TEXT ("CleanBar")
#define	LEDTYPE_VUMETER				4
#define	LEDTYPE_VUMETER_TEXT		TEXT ("VUMeter")
#define	LEDTYPE_GRAPH				5
#define	LEDTYPE_GRAPH_TEXT			TEXT ("Graph")
#define	LEDTYPE_NUMBER				6
#define	LEDTYPE_NUMBER_TEXT			TEXT ("Number")

// Value format constants (how the value is shown in the tooltip and in the number icon)
#define	VALUEFORMAT_PERCENT			1		// Value relative to the scale, e.g. "42.0%"
#define	VALUEFORMAT_NUMBER			2		// Raw value, e.g. "12.34"
#define	VALUEFORMAT_BYTES			3		// Raw value is bytes/s, e.g. "1.2 MB/s"
#define	VALUEFORMAT_BITS			4		// Raw value is bytes/s, shown as bits/s, e.g. "9.6 Mbit/s"
#define	VALUEFORMAT_MBYTES			5		// Raw value is megabytes, e.g. "34.5 GB / 64.0 GB"
#define	VALUEFORMAT_PERCENT_TEXT	TEXT ("Percent")
#define	VALUEFORMAT_NUMBER_TEXT		TEXT ("Number")
#define	VALUEFORMAT_BYTES_TEXT		TEXT ("Bytes")
#define	VALUEFORMAT_BITS_TEXT		TEXT ("Bits")
#define	VALUEFORMAT_MBYTES_TEXT		TEXT ("MBytes")

// Scale mode constants: what corresponds to the full icon
#define	SCALEMODE_FIXED				1		// The 100% value
#define	SCALEMODE_PEAK				2		// The peak of the recent history (at least the 100% value)
#define	SCALEMODE_DECAY				3		// A peak that slowly decays (at least the 100% value)
#define	SCALEMODE_AUTO				4		// The highest value seen so far, learned over time and kept in the INI (at least the 100% value)
#define	SCALEMODE_FIXED_TEXT		TEXT ("Fixed")
#define	SCALEMODE_PEAK_TEXT			TEXT ("Peak")
#define	SCALEMODE_DECAY_TEXT		TEXT ("Decay")
#define	SCALEMODE_AUTO_TEXT			TEXT ("Auto")

// Time after which the decaying peak has dropped to half
#define	DECAY_HALFLIFE_MS			30000

// Time after which the learned maximum (auto scale) has dropped to half. Very slow, so a quiet day does not forget the maximum.
#define	LEARN_HALFLIFE_MS			86400000

// The learned maximum is written to the INI file at most this often
#define	LEARN_SAVE_MS				300000

// Number of samples kept for the graph icon and for auto-scaling
#define	HISTORY_SIZE				16

// Default colour palette and the thresholds (in % of the scale) at which the colour changes
#define	DEFAULT_COLOR_LOW			RGB (0, 220, 0)
#define	DEFAULT_COLOR_MID			RGB (255, 200, 0)
#define	DEFAULT_COLOR_HIGH			RGB (255, 40, 0)
#define	DEFAULT_THRESHOLD_MID		50
#define	DEFAULT_THRESHOLD_HIGH		75

// Default values
#define DEFAULT_ICONTYPE			LEDTYPE_FLICKER
#define DEFAULT_COUNTER_1_PATH		TEXT ("\\PhysicalDisk(_Total)\\% Disk Time")
#define DEFAULT_UPDATE_INTERVAL_MS	30
#define MIN_UPDATE_INTERVAL_MS		10
#define MAX_UPDATE_INTERVAL_MS		3600000
#define MAX_100PERCENT				1e15
#define MAX_ALERT_PERCENT			10000
#define MAX_ALERT_SECONDS			86400
#define DEFAULT_100PERCENT			100
#define DEFAULT_ALERT_SECONDS		10

// Name of the INI (settings) file
#define INI_SETTINGS_FILE			TEXT ("DiskLED.ini")

// INI sections. Settings that are not specific to a counter live in the first section.
#define INI_COUNTER_1_SECTION		TEXT ("Performance Counter 1")
#define INI_COUNTER_2_SECTION		TEXT ("Performance Counter 2")

// INI value names
#define INI_VALUE_LEDTYPE			TEXT ("LEDType")
#define INI_VALUE_UPDATE_INTERVAL	TEXT ("UpdateInterval")
#define INI_VALUE_SECOND_COUNTER	TEXT ("SecondCounter")
#define INI_VALUE_COLOR_LOW			TEXT ("ColorLow")
#define INI_VALUE_COLOR_MID			TEXT ("ColorMid")
#define INI_VALUE_COLOR_HIGH		TEXT ("ColorHigh")
#define INI_VALUE_THRESHOLD_MID		TEXT ("ThresholdMid")
#define INI_VALUE_THRESHOLD_HIGH	TEXT ("ThresholdHigh")
#define INI_VALUE_PATH				TEXT ("Path")
#define INI_VALUE_100PERCENT		TEXT ("100% Utilization")
#define INI_VALUE_VALUEFORMAT		TEXT ("ValueFormat")
#define INI_VALUE_SCALEMODE			TEXT ("ScaleMode")
#define INI_VALUE_LEARNEDMAX		TEXT ("LearnedMax")
#define INI_VALUE_AUTOSCALE			TEXT ("AutoScale")				// Older versions: 1 = peak scaling. Only read.
#define INI_VALUE_INVERT			TEXT ("Invert")
#define INI_VALUE_ALERT_PERCENT		TEXT ("AlertAbove")
#define INI_VALUE_ALERT_SECONDS		TEXT ("AlertSeconds")


// Settings of one performance counter
struct CounterSettings
{
	CString		sPath;				// Counter path, in English if possible
	double		d100Percent;		// Counter value that corresponds to 100%
	DWORD		nValueFormat;		// VALUEFORMAT_*
	DWORD		nScaleMode;			// SCALEMODE_*
	double		dLearnedMax;		// The highest value seen (SCALEMODE_AUTO), 0 = nothing learned yet
	BOOL		bInvert;			// Use the 100% value minus the counter value
	int			nAlertPercent;		// Warn when the value is above this % of the 100% value, 0 = no warning
	int			nAlertSeconds;		// ... for at least this many seconds
};

// All settings
struct DiskLEDSettings
{
	DWORD			nIconType;			// LEDTYPE_*
	DWORD			nUpdateInterval;	// ms
	BOOL			bSecondCounter;		// Display a second counter as well
	CounterSettings	aCounters[2];
	COLORREF		anColors[3];		// Low, mid and high colour
	int				nThresholdMid;		// % of the scale where the mid colour starts
	int				nThresholdHigh;		// % of the scale where the high colour starts
};

void	SetDefaultCounterSettings (CounterSettings& oCounter);

// Sensible value format, scale mode and 100% value for a counter (an English path), e.g. 100 for percent counters, the auto scale for network speeds
void	GetCounterDefaults (const CString& sEnglishPath, CounterSettings& oCounter);
void	SetDefaultSettings (DiskLEDSettings& oSettings);
void	NormalizeSettings (DiskLEDSettings& oSettings);
DWORD	LoadSettings (const CString& sINIFile, DiskLEDSettings& oSettings);
DWORD	SaveSettings (const CString& sINIFile, const DiskLEDSettings& oSettings);
