// libtype.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <string>
#include <vector>

std::vector<std::vector<POINT>> g_vecBezierPts;
std::vector<std::vector<POINT>> g_vecPolylinePts;
std::vector<POINT> g_veclinePts;


std::string WcsToMbs(const std::wstring& wcs) 
{
	int lengthOfMbs = WideCharToMultiByte(CP_ACP, 0, wcs.c_str(), -1, NULL, 0, NULL, NULL);
	char* mbs = new char[lengthOfMbs];
	WideCharToMultiByte(CP_ACP, 0, wcs.c_str(), -1, mbs, lengthOfMbs, NULL, NULL);
	std::string result = mbs;
	delete mbs;
	mbs = NULL;
	return result;
}

std::wstring MbsToWcs(const std::string& mbs) 
{
	int lengthOfWcs = MultiByteToWideChar(CP_ACP, 0, mbs.c_str(), -1, NULL, 0);
	wchar_t* wcs = new wchar_t[lengthOfWcs];
	MultiByteToWideChar(CP_ACP, 0, mbs.c_str(), -1, wcs, lengthOfWcs);
	std::wstring result = wcs;
	delete wcs;
	wcs = NULL;
	return result;
}

FIXED FixedFromDouble(double d)
{
	long l;
	l = (long)(d * 65536L);
	return *(FIXED *)&l;
}

double DoubleFromFixed(FIXED f)
{
	return f.value + (double)f.fract / 65536L;
}

POINT GetPoint(POINTFX point, TEXTMETRIC& tm, POINT Origin)
{
	POINT p;
	p.x = DoubleFromFixed(point.x) + Origin.x;
	p.y = DoubleFromFixed(point.y) + Origin.y + tm.tmAscent;
	return p;
}

void AddBezierPoints(std::vector<POINT>& vec, POINT* pts, int nCount)
{
	for (int i = 0; i < nCount; i++)
	{
		vec.push_back(pts[i]);
	}
}

void AddPolylinePoints(std::vector<POINT>& vec, POINT* pts, int nCount)
{
	for (int i = 0; i < nCount; i++)
	{
		vec.push_back(pts[i]);
	}
}

void AddLlinePoints(POINT p1, POINT p2)
{
	g_veclinePts.push_back(p1);
	g_veclinePts.push_back(p2);
}

void GetCurve(int nCount, unsigned char* pBuffer, POINT ptOrigin, TEXTMETRIC& tm)
{
	while (nCount > 0)
	{
		TTPOLYGONHEADER* Header = (TTPOLYGONHEADER*)pBuffer;
		DWORD Start = (DWORD)pBuffer + sizeof(TTPOLYGONHEADER);
		DWORD dwRemine = Header->cb - sizeof(TTPOLYGONHEADER);
		POINTFX ptStart = Header->pfxStart;
		POINTFX ptInit = ptStart;
		while (dwRemine > 0)
		{
			std::vector<POINT> vecBezierPts;
			std::vector<POINT> vecPolylinePts;
			TTPOLYCURVE* pCurve = (TTPOLYCURVE*)Start;

			POINT* pts = new POINT[pCurve->cpfx + 1];
			pts[0] = GetPoint(ptStart, tm, ptOrigin);
			for (size_t i = 0; i < pCurve->cpfx; i++)
			{
				pts[i + 1] = GetPoint(pCurve->apfx[i], tm, ptOrigin);
			}

			switch (pCurve->wType)
			{
			case TT_PRIM_LINE:
				AddPolylinePoints(vecPolylinePts, pts, pCurve->cpfx + 1);
				break;
			case TT_PRIM_CSPLINE:
				AddBezierPoints(vecBezierPts, pts, pCurve->cpfx + 1);
				break;
			default:
				break;
			}

			delete[] pts;

			if (vecPolylinePts.size() > 0)
			{
				g_vecPolylinePts.push_back(vecPolylinePts);
			}
			if (vecBezierPts.size() > 0)
			{
				g_vecBezierPts.push_back(vecBezierPts);
			}

			ptStart = pCurve->apfx[pCurve->cpfx - 1];
			Start += sizeof(TTPOLYCURVE) + (pCurve->cpfx - 1) * sizeof(POINTFX);
			dwRemine -= sizeof(TTPOLYCURVE) + (pCurve->cpfx - 1) * sizeof(POINTFX);
		}
		if ((ptStart.x.fract != ptInit.x.fract) ||
			(ptStart.x.value != ptInit.x.value) ||
			(ptStart.y.fract != ptInit.y.fract) ||
			(ptStart.y.value != ptInit.y.value))
		{
			POINT p1 = GetPoint(ptStart, tm, ptOrigin);
			POINT p2 = GetPoint(ptInit, tm, ptOrigin);
			AddLlinePoints(p1, p2);
		}

		ptInit = ptStart;
		pBuffer += Header->cb;
		nCount -= Header->cb;
	}
}


void __stdcall SetText(HDC hdc, int x, int y, const char* str, const char* szFont, int nHeight, bool bBold, bool bItalic, bool bUnderline)
{
	g_vecBezierPts.clear();
	g_vecPolylinePts.clear();
	g_veclinePts.clear();

	HFONT hFont = CreateFontA(
		nHeight,
		0,                                          //   字体的宽度  
		0,                                          //  nEscapement 
		0,                                          //  nOrientation   
		(bBold ? FW_BOLD : FW_NORMAL),              //   nWeight   
		bItalic,                                    //   bItalic   
		bUnderline,                                 //   bUnderline   
		0,                                          //   cStrikeOut   
		ANSI_CHARSET,                               //   nCharSet   
		OUT_DEFAULT_PRECIS,						    //   nOutPrecision   
		CLIP_DEFAULT_PRECIS,						//   nClipPrecision   
		DEFAULT_QUALITY,							//   nQuality   
		DEFAULT_PITCH | FF_DONTCARE,				//   nPitchAndFamily     
		szFont);

	HGDIOBJ hOldFont = SelectObject(hdc, (HGDIOBJ)hFont);
	std::wstring wstr = MbsToWcs(str);
	POINT ptOrigin = { x, y };

	TEXTMETRIC tm;
	MAT2 mat;

	mat.eM11 = FixedFromDouble(1);
	mat.eM12 = FixedFromDouble(0);
	mat.eM21 = FixedFromDouble(0);
	mat.eM22 = FixedFromDouble(-1);

	GetTextMetrics(hdc, &tm);

	for (size_t i = 0; i < wstr.length(); i++)
	{
		GLYPHMETRICS gm;

		int nCount = GetGlyphOutline(hdc, wstr[i], GGO_BEZIER, &gm, 0, NULL, &mat);
		if (nCount != -1)
		{
			unsigned char* pBuf = new unsigned char[nCount];
			GetGlyphOutline(hdc, wstr[i], GGO_BEZIER, &gm, nCount, pBuf, &mat);
			GetCurve(nCount, pBuf, ptOrigin, tm);
			ptOrigin.x += gm.gmCellIncX;
			ptOrigin.y += gm.gmCellIncY;
			delete[] pBuf;
		}
	}


	SelectObject(hdc, hOldFont);
	DeleteObject(hFont);
}

int __stdcall GetBezierCount()
{
	return g_vecBezierPts.size();
}

int __stdcall GetBezierPointsCount(int bezierIndex)
{
	if ((bezierIndex < 0) || (bezierIndex > g_vecBezierPts.size() - 1))
	{
		return -1;
	}
	return g_vecBezierPts[bezierIndex].size();
}

int __stdcall GetPolylineCount()
{
	return g_vecPolylinePts.size();
}

int __stdcall GetPolylinePointsCount(int polylineIndex)
{
	if ((polylineIndex < 0) || (polylineIndex > g_vecPolylinePts.size() - 1))
	{
		return -1;
	}
	return g_vecPolylinePts[polylineIndex].size();
}

int __stdcall GetLinePointsCount()
{
	return (g_veclinePts.size() / 2);
}

void __stdcall GetBezierPoint(int bezierIndex, int ptIndex, int* x, int* y)
{
	if ((bezierIndex < 0) || (bezierIndex > g_vecBezierPts.size() - 1))
	{
		return;
	}
	if ((ptIndex < 0) || (ptIndex > g_vecBezierPts[bezierIndex].size() - 1))
	{
		return;
	}

	*x = g_vecBezierPts[bezierIndex][ptIndex].x;
	*y = g_vecBezierPts[bezierIndex][ptIndex].y;
}

void __stdcall GetPolylinePoint(int polylineIndex, int ptIndex, int* x, int* y)
{
	if ((polylineIndex < 0) || (polylineIndex > g_vecPolylinePts.size() - 1))
	{
		return;
	}
	if ((ptIndex < 0) || (ptIndex > g_vecPolylinePts[polylineIndex].size() - 1))
	{
		return;
	}

	*x = g_vecPolylinePts[polylineIndex][ptIndex].x;
	*y = g_vecPolylinePts[polylineIndex][ptIndex].y;
}

void __stdcall GetlinePoint(int idx, int* x1, int* y1, int* x2, int* y2)
{
	if ((idx < 0) || (idx > (g_vecPolylinePts.size() / 2) - 1))
	{
		return;
	}

	*x1 = g_veclinePts[idx * 2].x;
	*y1 = g_veclinePts[idx * 2].y;
	*x2 = g_veclinePts[idx * 2 + 1].x;
	*y2 = g_veclinePts[idx * 2 + 1].y;
}