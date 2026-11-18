#pragma once

#include <vector>

// The "counter path" of a graphics adapter temperature looks like a performance counter path, so everything that only displays or compares paths keeps working:
// \DiskLED GPU(<adapter name>)\Temperature. It is not a real performance counter, the meter reads it through the graphics kernel (D3DKMT) instead of PDH.
#define	GPU_PATH_OBJECT				TEXT ("DiskLED GPU")
#define	GPU_PATH_COUNTER			TEXT ("Temperature")

struct GpuAdapter
{
	CString		sName;				// The name of the adapter. Identical adapters get " #2", " #3", ...
	double		dCelsius;			// Current temperature
};

CString	MakeGpuPath (const CString& sAdapter);

// Is this a graphics adapter temperature path? The adapter name is returned in psAdapter.
BOOL	IsGpuPath (const CString& sPath, CString* psAdapter = NULL);

// The graphics adapters that report a temperature, with the current value. Software adapters and adapters whose driver reports no temperature
// (for example most integrated graphics, which share the temperature of the processor) are left out.
void	GetGpuAdapters (std::vector<GpuAdapter>& aAdapters);

// The current temperature of an adapter in degrees Celsius. FALSE if the adapter is gone or its driver reports no temperature.
BOOL	ReadGpuTemperature (const CString& sAdapter, double& dCelsius);
