// IconRenderer.cpp : draws the tray icons (also used for the preview in the config dialog)
//

#include "stdafx.h"
#include "IconRenderer.h"
#include <math.h>


void MakeIconStyle (const DiskLEDSettings& oSettings, IconStyle& oStyle)
{
	oStyle.nType				= oSettings.nIconType;
	oStyle.anColors[0]			= oSettings.anColors[0];
	oStyle.anColors[1]			= oSettings.anColors[1];
	oStyle.anColors[2]			= oSettings.anColors[2];
	oStyle.adThresholds[0]		= oSettings.nThresholdMid;
	oStyle.adThresholds[1]		= oSettings.nThresholdHigh;
}


// Index into the palette (0 = low, 1 = mid, 2 = high) for a value in % of the scale
int GetColorIndex (const IconStyle& oStyle, double dNormalized)
{
	if (dNormalized >= oStyle.adThresholds[1])
	{
		return 2;
	}
	else if (dNormalized >= oStyle.adThresholds[0])
	{
		return 1;
	}

	return 0;
}


BOOL UsesSeparateIcons (DWORD nIconType, int nCounters)
{
	return nIconType == LEDTYPE_NUMBER && nCounters >= 2;
}


// Value in % of the scale
static double GetNormalized (const IconCounter& oCounter)
{
	double	dNormalized	= (oCounter.dScale > 0) ? oCounter.dValue * 100.0 / oCounter.dScale : 0;

	return _finite (dNormalized) ? dNormalized : 0;
}

// LED brightness step (0-4) for a value in % of the scale
static int GetLEDLevel (double dNormalized)
{
	if (dNormalized <= 0)
	{
		return 0;
	}
	else if (dNormalized < 25)
	{
		return 1;
	}
	else if (dNormalized < 50)
	{
		return 2;
	}
	else if (dNormalized < 75)
	{
		return 3;
	}

	return 4;
}

// Height in pixels of a fill for a value in % of the scale (at least 1 pixel for any non-zero value)
static int GetFillPixels (double dNormalized, int nHeight)
{
	if (!(dNormalized > 0.0))
	{
		return 0;
	}
	if (dNormalized >= 100.0)
	{
		return nHeight;
	}

	int	nFill	= (int) (dNormalized / 100.0 * nHeight + 0.5);

	return (nFill < 1) ? 1 : (nFill > nHeight) ? nHeight : nFill;
}

// Width of the white edge of the bar icon
static int GetBarBorder (const CRect& oRect)
{
	return (oRect.Height () / 16 > 1) ? oRect.Height () / 16 : 1;
}

// Pitch in pixels of one VU meter segment (segment plus gap)
static int GetVUPitch (int nHeight)
{
	return (nHeight / 5 > 2) ? nHeight / 5 : 2;
}

// Number of lit VU meter segments for a value in % of the scale
static int GetVULit (double dNormalized, int nHeight)
{
	int		nSegments	= nHeight / GetVUPitch (nHeight);

	if (!(dNormalized > 0.0))
	{
		return 0;
	}

	double	dLit		= ceil (dNormalized * nSegments / 100.0);

	return (dLit > nSegments) ? nSegments : (int) dLit;
}

// Height in pixels of the graph bar for history entry i, and its value in 0..1 of the scale
static int GetGraphBarHeight (const IconCounter& oCounter, int i, int nHeight, double& dFraction)
{
	dFraction	= (oCounter.dScale > 0) ? oCounter.adHistory[i] / oCounter.dScale : 0;
	if (!(dFraction > 0.0))
	{
		dFraction	= 0;
	}
	if (dFraction > 1.0)
	{
		dFraction	= 1.0;
	}

	int		nBarHeight	= (int) (dFraction * nHeight + 0.5);
	if (nBarHeight == 0 && oCounter.adHistory[i] > 0)
	{
		nBarHeight	= 1;
	}

	return nBarHeight;
}

static DWORD HashStep (DWORD nHash, DWORD nValue)
{
	return (nHash ^ nValue) * 16777619u;
}

// The number of the number icon: one decimal for small numbers, none from 10 up (rounding to 10 must not show "10.0")
static CString GetNumberText (const IconCounter& oCounter, CString& sIconUnit)
{
	CString	sUnit;
	CString	sNumber;
	double	dNumber	= 0;

	GetValueParts (oCounter.nValueFormat, oCounter.dValue, oCounter.dScale, dNumber, sUnit, sIconUnit);
	sNumber.Format ((dNumber < 9.95) ? TEXT ("%.1f") : TEXT ("%.0f"), dNumber);

	return sNumber;
}

static COLORREF ScaleColor (COLORREF nColor, double dFactor)
{
	return RGB ((int) (GetRValue (nColor) * dFactor), (int) (GetGValue (nColor) * dFactor), (int) (GetBValue (nColor) * dFactor));
}


////////////////////////////////////////////////////////////////////////////////
//
//	Split a value into a displayable number and its unit, depending on the value format
//
////////////////////////////////////////////////////////////////////////////////

void GetValueParts (DWORD nValueFormat, double dValue, double dScale, double& dNumber, CString& sUnit, CString& sIconUnit)
{
	if (nValueFormat == VALUEFORMAT_BYTES || nValueFormat == VALUEFORMAT_BITS)
	{
		bool			bBits			= (nValueFormat == VALUEFORMAT_BITS);
		double			dUnit			= bBits ? 1000.0 : 1024.0;
		const TCHAR*	asBytes[]		= { TEXT ("B/s"), TEXT ("KB/s"), TEXT ("MB/s"), TEXT ("GB/s") };
		const TCHAR*	asBits[]		= { TEXT ("bit/s"), TEXT ("kbit/s"), TEXT ("Mbit/s"), TEXT ("Gbit/s") };
		const TCHAR*	asIconBytes[]	= { TEXT ("B"), TEXT ("K"), TEXT ("M"), TEXT ("G") };
		const TCHAR*	asIconBits[]	= { TEXT ("b"), TEXT ("k"), TEXT ("m"), TEXT ("g") };
		int				nUnit			= 0;

		dNumber	= bBits ? dValue * 8.0 : dValue;
		while (dNumber >= dUnit && nUnit < 3)
		{
			dNumber	/= dUnit;
			nUnit++;
		}
		sUnit		= bBits ? asBits[nUnit] : asBytes[nUnit];
		sIconUnit	= bBits ? asIconBits[nUnit] : asIconBytes[nUnit];
	}
	else if (nValueFormat == VALUEFORMAT_MBYTES)
	{
		const TCHAR*	asMBytes[]		= { TEXT ("MB"), TEXT ("GB"), TEXT ("TB") };
		const TCHAR*	asIconMBytes[]	= { TEXT ("M"), TEXT ("G"), TEXT ("T") };
		int				nUnit			= 0;

		dNumber	= dValue;
		while (dNumber >= 1024.0 && nUnit < 2)
		{
			dNumber	/= 1024.0;
			nUnit++;
		}
		sUnit		= asMBytes[nUnit];
		sIconUnit	= asIconMBytes[nUnit];
	}
	else if (nValueFormat == VALUEFORMAT_NUMBER)
	{
		dNumber		= dValue;
		sUnit.Empty ();
		sIconUnit.Empty ();
	}
	else
	{
		dNumber		= (dScale > 0) ? dValue * 100.0 / dScale : 0;
		sUnit		= TEXT ("%");
		sIconUnit	= TEXT ("%");
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Format a value for the tooltip
//
////////////////////////////////////////////////////////////////////////////////

CString FormatValue (DWORD nValueFormat, double dValue, double dScale, BOOL bShowTotal)
{
	CString	sResult;
	CString	sUnit;
	CString	sIconUnit;
	double	dNumber	= 0;

	GetValueParts (nValueFormat, dValue, dScale, dNumber, sUnit, sIconUnit);

	if (nValueFormat == VALUEFORMAT_NUMBER)
	{
		sResult.Format (TEXT ("%.2f"), dNumber);
	}
	else if (nValueFormat == VALUEFORMAT_PERCENT)
	{
		sResult.Format (TEXT ("%.1f%%"), dNumber);
	}
	else if (nValueFormat == VALUEFORMAT_MBYTES)
	{
		sResult.Format (TEXT ("%.1f %s"), dNumber, (LPCTSTR) sUnit);

		// With a fixed scale, the scale is the total (e.g. installed memory)
		if (bShowTotal)
		{
			CString	sTotalUnit;
			CString	sTotalIconUnit;
			double	dTotal	= 0;

			GetValueParts (nValueFormat, dScale, dScale, dTotal, sTotalUnit, sTotalIconUnit);
			sResult.AppendFormat (TEXT (" / %.1f %s"), dTotal, (LPCTSTR) sTotalUnit);
		}
	}
	else
	{
		sResult.Format (TEXT ("%.1f %s"), dNumber, (LPCTSTR) sUnit);
	}

	return sResult;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Where each counter is drawn: one counter fills the icon. Two counters share it, side by side
//	for the bar types and one above the other for the graph. (The number gets an icon per counter.)
//
////////////////////////////////////////////////////////////////////////////////

static void GetCounterRects (const IconStyle& oStyle, int nCounters, int nSize, CRect* aRects)
{
	if (nCounters < 2)
	{
		aRects[0]	= CRect (0, 0, nSize, nSize);
	}
	else if (oStyle.nType == LEDTYPE_GRAPH)
	{
		aRects[0]	= CRect (0, 0, nSize, nSize / 2);
		aRects[1]	= CRect (0, nSize / 2, nSize, nSize);
	}
	else
	{
		aRects[0]	= CRect (0, 0, nSize / 2, nSize);
		aRects[1]	= CRect (nSize / 2, 0, nSize, nSize);
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Identifies what the icon looks like
//
////////////////////////////////////////////////////////////////////////////////

DWORD GetIconKey (const IconStyle& oStyle, const IconCounter* aCounters, int nCounters, int nSize)
{
	CRect	aRects[2];
	DWORD	nKey	= 2166136261u;

	if (nCounters > 2)
	{
		nCounters	= 2;
	}
	GetCounterRects (oStyle, nCounters, nSize, aRects);

	// The size is part of the key: the same values look different at another size
	nKey	= HashStep (nKey, nSize);

	for (int i = 0; i < nCounters; i++)
	{
		double	dNormalized	= GetNormalized (aCounters[i]);

		switch (oStyle.nType)
		{
		case LEDTYPE_COLOREDBAR:
			nKey	= HashStep (nKey, GetFillPixels (dNormalized, aRects[i].Height () - GetBarBorder (aRects[i])) * 4 + GetColorIndex (oStyle, dNormalized));
			break;
		case LEDTYPE_CLEANBAR:
			nKey	= HashStep (nKey, GetFillPixels (dNormalized, aRects[i].Height ()) * 4 + GetColorIndex (oStyle, dNormalized));
			break;
		case LEDTYPE_VUMETER:
			nKey	= HashStep (nKey, GetVULit (dNormalized, aRects[i].Height ()));
			break;
		case LEDTYPE_GRAPH:
			// What is drawn is the bar height in pixels and the colour of each sample
			for (int j = 0; j < HISTORY_SIZE; j++)
			{
				double	dFraction	= 0;
				int		nBarHeight	= GetGraphBarHeight (aCounters[i], j, aRects[i].Height (), dFraction);
				nKey	= HashStep (nKey, nBarHeight * 4 + GetColorIndex (oStyle, dFraction * 100.0));
			}
			break;
		case LEDTYPE_NUMBER:
			{
				// What is drawn is the text and its colour
				CString	sIconUnit;
				CString	sNumber	= GetNumberText (aCounters[i], sIconUnit);

				for (int j = 0; j < sNumber.GetLength (); j++)
				{
					nKey	= HashStep (nKey, sNumber[j]);
				}
				for (int j = 0; j < sIconUnit.GetLength (); j++)
				{
					nKey	= HashStep (nKey, sIconUnit[j]);
				}
				nKey	= HashStep (nKey, GetColorIndex (oStyle, dNormalized));
			}
			break;
		default:
			nKey	= HashStep (nKey, GetLEDLevel (dNormalized));
			break;
		}
	}

	// Keep clear of the reserved values
	return (nKey >= ICONKEY_NONE) ? 1 : nKey;
}


////////////////////////////////////////////////////////////////////////////////
//
//	LED: a rectangle in the low colour whose brightness follows the load in steps
//
////////////////////////////////////////////////////////////////////////////////

static void DrawLED (CDC* pDC, const CRect& oRect, const IconStyle& oStyle, const IconCounter& oCounter)
{
	static const double	adBrightness[]	= { 0.0, 0.27, 0.5, 0.75, 1.0 };
	CRect				oLED			= oRect;
	int					nMarginX		= (oRect.Width () / 16 > 1) ? oRect.Width () / 16 : 1;

	oLED.DeflateRect (nMarginX, oRect.Height () * 3 / 16);
	pDC->FillSolidRect (oLED, ScaleColor (oStyle.anColors[0], adBrightness[GetLEDLevel (GetNormalized (oCounter))]));
}


////////////////////////////////////////////////////////////////////////////////
//
//	Bar: Task Manager style, dotted background, white edge right and bottom, fill from the bottom
//
////////////////////////////////////////////////////////////////////////////////

static void DrawBar (CDC* pDC, const CRect& oRect, const IconStyle& oStyle, const IconCounter& oCounter)
{
	double	dNormalized	= GetNormalized (oCounter);
	int		nBorder		= GetBarBorder (oRect);
	int		nInnerRight	= oRect.right - nBorder;
	int		nInnerBottom	= oRect.bottom - nBorder;
	int		nFill		= GetFillPixels (dNormalized, oRect.Height () - nBorder);

	for (int y = oRect.top; y < nInnerBottom; y += 2)
	{
		for (int x = oRect.left; x < nInnerRight; x += 2)
		{
			pDC->SetPixelV (x, y, RGB (110, 110, 110));
		}
	}

	pDC->FillSolidRect (nInnerRight, oRect.top, nBorder, oRect.Height (), RGB (255, 255, 255));
	pDC->FillSolidRect (oRect.left, nInnerBottom, oRect.Width (), nBorder, RGB (255, 255, 255));
	pDC->FillSolidRect (oRect.left, nInnerBottom - nFill, nInnerRight - oRect.left, nFill, oStyle.anColors[GetColorIndex (oStyle, dNormalized)]);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Clean bar: a solid fill from the bottom on a black background
//
////////////////////////////////////////////////////////////////////////////////

static void DrawCleanBar (CDC* pDC, const CRect& oRect, const IconStyle& oStyle, const IconCounter& oCounter)
{
	double	dNormalized	= GetNormalized (oCounter);
	int		nMargin		= oRect.Width () / 8;
	int		nFill		= GetFillPixels (dNormalized, oRect.Height ());

	pDC->FillSolidRect (oRect.left + nMargin, oRect.bottom - nFill, oRect.Width () - 2 * nMargin, nFill, oStyle.anColors[GetColorIndex (oStyle, dNormalized)]);
}


////////////////////////////////////////////////////////////////////////////////
//
//	VU meter: stacked segments, lit up to the current value and coloured by their height
//	Unlit segments are shown dimmed
//
////////////////////////////////////////////////////////////////////////////////

static void DrawVUMeter (CDC* pDC, const CRect& oRect, const IconStyle& oStyle, const IconCounter& oCounter)
{
	int	nPitch		= GetVUPitch (oRect.Height ());
	int	nSegments	= oRect.Height () / nPitch;
	int	nLit		= GetVULit (GetNormalized (oCounter), oRect.Height ());
	int	nMargin		= oRect.Width () / 8;

	for (int i = 0; i < nSegments; i++)
	{
		COLORREF	nColor	= oStyle.anColors[GetColorIndex (oStyle, (i + 0.5) * 100.0 / nSegments)];
		if (i >= nLit)
		{
			nColor	= ScaleColor (nColor, 0.2);
		}

		// Segment i counted from the bottom, with a one pixel gap above it
		int	nBottom	= oRect.bottom - i * nPitch;
		pDC->FillSolidRect (oRect.left + nMargin, nBottom - nPitch + 1, oRect.Width () - 2 * nMargin, nPitch - 1, nColor);
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Graph: the recent values as bars, a dim line at 50% as a reference
//
////////////////////////////////////////////////////////////////////////////////

static void DrawGraph (CDC* pDC, const CRect& oRect, const IconStyle& oStyle, const IconCounter& oCounter)
{
	for (int i = 0; i < HISTORY_SIZE; i++)
	{
		double	dFraction	= 0;
		int		nHeight		= GetGraphBarHeight (oCounter, i, oRect.Height (), dFraction);

		int		nLeft	= oRect.left + i * oRect.Width () / HISTORY_SIZE;
		int		nRight	= oRect.left + (i + 1) * oRect.Width () / HISTORY_SIZE;
		pDC->FillSolidRect (nLeft, oRect.bottom - nHeight, nRight - nLeft, nHeight, oStyle.anColors[GetColorIndex (oStyle, dFraction * 100.0)]);
	}

	pDC->FillSolidRect (oRect.left, oRect.bottom - oRect.Height () / 2, oRect.Width (), 1, RGB (90, 90, 90));
}


////////////////////////////////////////////////////////////////////////////////
//
//	Number: the current value as text, coloured by level. With enough room a one-character unit goes below it.
//
////////////////////////////////////////////////////////////////////////////////

static void DrawNumber (CDC* pDC, const CRect& oRect, const IconStyle& oStyle, const IconCounter& oCounter)
{
	CString	sIconUnit;
	CString	sNumber	= GetNumberText (oCounter, sIconUnit);

	// The number gets the whole area if there is no unit or no room for it, otherwise the upper part
	bool	bUnit			= !sIconUnit.IsEmpty () && oRect.Height () >= 12 && oRect.Height () >= oRect.Width ();
	int		nNumberHeight	= bUnit ? oRect.Height () * 10 / 16 : oRect.Height ();
	int		nUnitHeight		= oRect.Height () - nNumberHeight;

	pDC->SetBkMode (TRANSPARENT);

	// Shrink the number's font until the text fits the width
	CFont	oFont;
	for (int nFontHeight = nNumberHeight; nFontHeight > 4; nFontHeight--)
	{
		oFont.DeleteObject ();
		oFont.CreateFont (-nFontHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT ("Arial"));
		CFont*	pOldFont	= pDC->SelectObject (&oFont);
		CSize	oExtent		= pDC->GetTextExtent (sNumber);
		pDC->SelectObject (pOldFont);
		if (oExtent.cx <= oRect.Width ())
		{
			break;
		}
	}

	CFont*	pOldFont	= pDC->SelectObject (&oFont);
	pDC->SetTextColor (oStyle.anColors[GetColorIndex (oStyle, GetNormalized (oCounter))]);
	CRect	oNumberRect (oRect.left, oRect.top, oRect.right, oRect.top + nNumberHeight);
	pDC->DrawText (sNumber, &oNumberRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	pDC->SelectObject (pOldFont);

	if (bUnit)
	{
		CFont	oUnitFont;
		oUnitFont.CreateFont (-nUnitHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT ("Arial"));
		pOldFont	= pDC->SelectObject (&oUnitFont);
		pDC->SetTextColor (RGB (80, 200, 255));
		CRect	oUnitRect (oRect.left, oRect.top + nNumberHeight, oRect.right, oRect.bottom);
		pDC->DrawText (sIconUnit, &oUnitRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		pDC->SelectObject (pOldFont);
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Draw the icon into a DC
//
////////////////////////////////////////////////////////////////////////////////

void DrawIconImage (CDC* pDC, int nSize, const IconStyle& oStyle, const IconCounter* aCounters, int nCounters)
{
	CRect	aRects[2];

	if (nCounters > 2)
	{
		nCounters	= 2;
	}
	GetCounterRects (oStyle, nCounters, nSize, aRects);

	pDC->FillSolidRect (0, 0, nSize, nSize, RGB (0, 0, 0));

	for (int i = 0; i < nCounters; i++)
	{
		switch (oStyle.nType)
		{
		case LEDTYPE_COLOREDBAR:
			DrawBar (pDC, aRects[i], oStyle, aCounters[i]);
			break;
		case LEDTYPE_CLEANBAR:
			DrawCleanBar (pDC, aRects[i], oStyle, aCounters[i]);
			break;
		case LEDTYPE_VUMETER:
			DrawVUMeter (pDC, aRects[i], oStyle, aCounters[i]);
			break;
		case LEDTYPE_GRAPH:
			DrawGraph (pDC, aRects[i], oStyle, aCounters[i]);
			break;
		case LEDTYPE_NUMBER:
			DrawNumber (pDC, aRects[i], oStyle, aCounters[i]);
			break;
		default:
			DrawLED (pDC, aRects[i], oStyle, aCounters[i]);
			break;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Create a tray icon (caller must DestroyIcon)
//
////////////////////////////////////////////////////////////////////////////////

HICON CreateTrayIconImage (const IconStyle& oStyle, const IconCounter* aCounters, int nCounters)
{
	int			nSize		= GetSystemMetrics (SM_CXSMICON);
	HDC			hScreenDC	= ::GetDC (NULL);
	HDC			hDC			= CreateCompatibleDC (hScreenDC);
	HBITMAP		hColor		= CreateCompatibleBitmap (hScreenDC, nSize, nSize);
	HBITMAP		hMask		= CreateBitmap (nSize, nSize, 1, 1, NULL);
	HICON		hIcon		= NULL;

	if (hDC && hColor && hMask)
	{
		CDC*	pDC		= CDC::FromHandle (hDC);

		// Opaque icon: an all-zero mask
		HGDIOBJ	hOldBitmap	= SelectObject (hDC, hMask);
		PatBlt (hDC, 0, 0, nSize, nSize, BLACKNESS);

		SelectObject (hDC, hColor);
		DrawIconImage (pDC, nSize, oStyle, aCounters, nCounters);

		SelectObject (hDC, hOldBitmap);

		ICONINFO	oInfo;
		SecureZeroMemory (&oInfo, sizeof (oInfo));
		oInfo.fIcon		= TRUE;
		oInfo.hbmMask	= hMask;
		oInfo.hbmColor	= hColor;
		hIcon			= CreateIconIndirect (&oInfo);
	}

	if (hMask)
	{
		DeleteObject (hMask);
	}
	if (hColor)
	{
		DeleteObject (hColor);
	}
	if (hDC)
	{
		DeleteDC (hDC);
	}
	::ReleaseDC (NULL, hScreenDC);

	return hIcon;
}
