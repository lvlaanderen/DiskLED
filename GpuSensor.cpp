// GpuSensor.cpp : the temperature of graphics adapters, read through the graphics kernel (D3DKMT, the same source the Task Manager uses)
//
// No driver or administrator rights are needed. The functions are loaded from gdi32.dll when needed. The structures are declared here with the layouts of
// d3dkmthk.h (which cannot be included together with windows.h and the MFC headers without redefining the status codes).

#include "stdafx.h"
#include "GpuSensor.h"
#include "Helpers.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// KMTQUERYADAPTERINFOTYPE values
#define	KMT_QAI_ADAPTERREGISTRYINFO		8
#define	KMT_QAI_ADAPTERTYPE				15
#define	KMT_QAI_ADAPTERPERFDATA			62

// D3DKMT_ADAPTERTYPE.Value: bit 2 is "software device"
#define	KMT_ADAPTERTYPE_SOFTWAREDEVICE	0x4

// Plausible readings in tenths of a degree Celsius, anything else is no reading
#define	KMT_TEMPERATURE_MIN				1
#define	KMT_TEMPERATURE_MAX				2500

struct KmtAdapterInfo
{
	UINT	hAdapter;
	LUID	AdapterLuid;
	ULONG	NumOfSources;
	BOOL	bPrecisePresentRegionsPreferred;
};

struct KmtEnumAdapters2
{
	ULONG			NumAdapters;
	KmtAdapterInfo*	pAdapters;
};

struct KmtQueryAdapterInfo
{
	UINT	hAdapter;
	UINT	Type;
	void*	pPrivateDriverData;
	UINT	PrivateDriverDataSize;
};

struct KmtOpenAdapterFromLuid
{
	LUID	AdapterLuid;
	UINT	hAdapter;
};

struct KmtCloseAdapter
{
	UINT	hAdapter;
};

struct KmtAdapterRegistryInfo
{
	WCHAR	AdapterString[MAX_PATH];
	WCHAR	BiosString[MAX_PATH];
	WCHAR	DacType[MAX_PATH];
	WCHAR	ChipType[MAX_PATH];
};

struct KmtAdapterType
{
	UINT	Value;
};

struct KmtAdapterPerfData
{
	UINT32		PhysicalAdapterIndex;
	ULONGLONG	MemoryFrequency;
	ULONGLONG	MaxMemoryFrequency;
	ULONGLONG	MaxMemoryFrequencyOC;
	ULONGLONG	MemoryBandwidth;
	ULONGLONG	PCIEBandwidth;
	ULONG		FanRPM;
	ULONG		Power;
	ULONG		Temperature;		// In tenths of a degree Celsius
	UCHAR		PowerStateOverride;
};

static_assert (sizeof (KmtAdapterPerfData) == 64, "D3DKMT_ADAPTER_PERFDATA has another layout");

typedef LONG (WINAPI* PFN_KmtEnumAdapters2) (KmtEnumAdapters2*);
typedef LONG (WINAPI* PFN_KmtQueryAdapterInfo) (KmtQueryAdapterInfo*);
typedef LONG (WINAPI* PFN_KmtOpenAdapterFromLuid) (KmtOpenAdapterFromLuid*);
typedef LONG (WINAPI* PFN_KmtCloseAdapter) (KmtCloseAdapter*);

static PFN_KmtEnumAdapters2			g_pfnEnumAdapters2		= NULL;
static PFN_KmtQueryAdapterInfo		g_pfnQueryAdapterInfo	= NULL;
static PFN_KmtOpenAdapterFromLuid	g_pfnOpenAdapterFromLuid	= NULL;
static PFN_KmtCloseAdapter			g_pfnCloseAdapter		= NULL;
static BOOL							g_bApiTried				= FALSE;

static BOOL LoadApi ()
{
	if (!g_bApiTried)
	{
		HMODULE	hGdi	= LoadLibrary (TEXT ("gdi32.dll"));

		g_bApiTried	= TRUE;
		if (hGdi)
		{
			g_pfnEnumAdapters2			= (PFN_KmtEnumAdapters2) GetProcAddress (hGdi, "D3DKMTEnumAdapters2");
			g_pfnQueryAdapterInfo		= (PFN_KmtQueryAdapterInfo) GetProcAddress (hGdi, "D3DKMTQueryAdapterInfo");
			g_pfnOpenAdapterFromLuid	= (PFN_KmtOpenAdapterFromLuid) GetProcAddress (hGdi, "D3DKMTOpenAdapterFromLuid");
			g_pfnCloseAdapter			= (PFN_KmtCloseAdapter) GetProcAddress (hGdi, "D3DKMTCloseAdapter");
		}
	}

	return g_pfnEnumAdapters2 && g_pfnQueryAdapterInfo && g_pfnOpenAdapterFromLuid && g_pfnCloseAdapter;
}

static BOOL QueryAdapter (UINT hAdapter, UINT nType, void* pData, UINT nSize)
{
	KmtQueryAdapterInfo	oQuery;

	oQuery.hAdapter				= hAdapter;
	oQuery.Type					= nType;
	oQuery.pPrivateDriverData	= pData;
	oQuery.PrivateDriverDataSize	= nSize;

	// NTSTATUS: success and informational values are not negative
	return g_pfnQueryAdapterInfo (&oQuery) >= 0;
}

static void CloseAdapter (UINT hAdapter)
{
	KmtCloseAdapter	oClose	= { hAdapter };

	g_pfnCloseAdapter (&oClose);
}


////////////////////////////////////////////////////////////////////////////////
//
//	The adapters that exist: name and LUID. The LUID is only valid until the next restart (or driver reset), the name is what the meter stores.
//
////////////////////////////////////////////////////////////////////////////////

struct GpuEntry
{
	CString	sBase;				// The name the driver reports
	CString	sName;				// The name with " #2" for an identical adapter
	LUID	oLuid;
};

static std::vector<GpuEntry>	g_aEntries;

static void RefreshEntries ()
{
	KmtEnumAdapters2	oEnum	= { 0, NULL };

	g_aEntries.clear ();
	if (!LoadApi ())
	{
		return;
	}

	// The first call returns the number of adapters (it can be more than there are)
	if (g_pfnEnumAdapters2 (&oEnum) < 0 || oEnum.NumAdapters == 0)
	{
		return;
	}

	std::vector<KmtAdapterInfo>	aInfo (oEnum.NumAdapters);

	oEnum.pAdapters	= aInfo.data ();
	if (g_pfnEnumAdapters2 (&oEnum) < 0)
	{
		return;
	}

	for (ULONG i = 0; i < oEnum.NumAdapters && i < aInfo.size (); i++)
	{
		UINT					hAdapter	= aInfo[i].hAdapter;
		KmtAdapterType			oType		= { 0 };
		KmtAdapterRegistryInfo	oRegistry;

		SecureZeroMemory (&oRegistry, sizeof (oRegistry));
		if (QueryAdapter (hAdapter, KMT_QAI_ADAPTERTYPE, &oType, sizeof (oType)) && (oType.Value & KMT_ADAPTERTYPE_SOFTWAREDEVICE) == 0
			&& QueryAdapter (hAdapter, KMT_QAI_ADAPTERREGISTRYINFO, &oRegistry, sizeof (oRegistry)) && oRegistry.AdapterString[0] != 0)
		{
			GpuEntry	oEntry;
			int			nSame		= 0;

			oRegistry.AdapterString[MAX_PATH - 1]	= 0;
			oEntry.sBase	= oRegistry.AdapterString;
			oEntry.oLuid	= aInfo[i].AdapterLuid;
			for (size_t j = 0; j < g_aEntries.size (); j++)
			{
				if (g_aEntries[j].sBase == oEntry.sBase)
				{
					nSame++;
				}
			}
			oEntry.sName	= oEntry.sBase;
			if (nSame > 0)
			{
				oEntry.sName.AppendFormat (TEXT (" #%d"), nSame + 1);
			}
			g_aEntries.push_back (oEntry);
		}
		CloseAdapter (hAdapter);
	}
}

static const GpuEntry* FindEntry (const CString& sAdapter)
{
	for (size_t i = 0; i < g_aEntries.size (); i++)
	{
		if (g_aEntries[i].sName.CompareNoCase (sAdapter) == 0)
		{
			return &g_aEntries[i];
		}
	}

	return NULL;
}

static BOOL ReadEntry (const GpuEntry& oEntry, double& dCelsius)
{
	KmtOpenAdapterFromLuid	oOpen;
	KmtAdapterPerfData		oPerf;
	BOOL					bResult	= FALSE;

	SecureZeroMemory (&oOpen, sizeof (oOpen));
	SecureZeroMemory (&oPerf, sizeof (oPerf));
	oOpen.AdapterLuid	= oEntry.oLuid;
	if (g_pfnOpenAdapterFromLuid (&oOpen) < 0)
	{
		return FALSE;
	}

	if (QueryAdapter (oOpen.hAdapter, KMT_QAI_ADAPTERPERFDATA, &oPerf, sizeof (oPerf)) && oPerf.Temperature >= KMT_TEMPERATURE_MIN && oPerf.Temperature <= KMT_TEMPERATURE_MAX)
	{
		dCelsius	= oPerf.Temperature / 10.0;
		bResult		= TRUE;
	}
	CloseAdapter (oOpen.hAdapter);

	return bResult;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Public functions
//
////////////////////////////////////////////////////////////////////////////////

CString MakeGpuPath (const CString& sAdapter)
{
	return MakePerfPath (TEXT (""), GPU_PATH_OBJECT, sAdapter, GPU_PATH_COUNTER);
}

BOOL IsGpuPath (const CString& sPath, CString* psAdapter)
{
	CString	sMachine;
	CString	sObject;
	CString	sInstance;
	CString	sCounter;

	if (!ParsePerfPath (sPath, sMachine, sObject, sInstance, sCounter) || sObject.CompareNoCase (GPU_PATH_OBJECT) != 0 || sCounter.CompareNoCase (GPU_PATH_COUNTER) != 0)
	{
		return FALSE;
	}
	if (psAdapter)
	{
		*psAdapter	= sInstance;
	}

	return TRUE;
}

void GetGpuAdapters (std::vector<GpuAdapter>& aAdapters)
{
	aAdapters.clear ();
	RefreshEntries ();

	for (size_t i = 0; i < g_aEntries.size (); i++)
	{
		GpuAdapter	oAdapter;

		oAdapter.sName		= g_aEntries[i].sName;
		oAdapter.dCelsius	= 0;
		if (ReadEntry (g_aEntries[i], oAdapter.dCelsius))
		{
			aAdapters.push_back (oAdapter);
		}
	}
}

BOOL ReadGpuTemperature (const CString& sAdapter, double& dCelsius)
{
	const GpuEntry*	pEntry	= FindEntry (sAdapter);

	dCelsius	= 0;

	// The adapters are only listed again when this one is not known or cannot be read (the LUID changes after a driver reset)
	if (pEntry && ReadEntry (*pEntry, dCelsius))
	{
		return TRUE;
	}

	RefreshEntries ();
	pEntry	= FindEntry (sAdapter);

	return pEntry && ReadEntry (*pEntry, dCelsius);
}
