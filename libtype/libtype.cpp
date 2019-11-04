// libtype.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <string>
#include <vector>

std::vector<POINT> g_vecBezierPts;
std::vector<POINT> g_vecPolylinePts;
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

void AddBezierPoints(POINT* pts, int nCount)
{
	for (int i = 0; i < nCount; i++)
	{
		g_vecBezierPts.push_back(pts[i]);
	}
}

void AddPolylinePoints(POINT* pts, int nCount)
{
	for (int i = 0; i < nCount; i++)
	{
		g_vecPolylinePts.push_back(pts[i]);
	}
}

void AddLlinePoints(POINT p1, POINT p2)
{
	g_veclinePts.push_back(p1);
	g_veclinePts.push_back(p2);
}

void GetCurve(wchar_t ch, int nCount, unsigned char* pBuffer, POINT ptOrigin, TEXTMETRIC& tm)
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
				AddPolylinePoints(pts, pCurve->cpfx + 1);
				break;
			case TT_PRIM_CSPLINE:
			{
				AddBezierPoints(pts, pCurve->cpfx + 1);
				break;
			}
			default:
				break;
			}

			delete[] pts;

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
			ptOrigin.x += gm.gmCellIncX;
			ptOrigin.y += gm.gmBlackBoxY;
			GetCurve(wstr[i], nCount, pBuf, ptOrigin, tm);
			delete[] pBuf;
		}
	}


	SelectObject(hdc, hOldFont);
	DeleteObject(hFont);
}

int __stdcall GetBezierPointsCount()
{
	return g_vecBezierPts.size();
}

int __stdcall GetPolylinePointsCount()
{
	return g_vecPolylinePts.size();
}

int __stdcall GetLinePointsCount()
{
	return (g_veclinePts.size() / 2);
}

void __stdcall GetBezierPoint(int idx, int* x, int* y)
{
	if ((idx < 0) || (idx > g_vecBezierPts.size() - 1))
	{
		return;
	}

	*x = g_vecBezierPts[idx].x;
	*y = g_vecBezierPts[idx].y;
}

void __stdcall GetPolylinePoint(int idx, int* x, int* y)
{
	if ((idx < 0) || (idx > g_vecPolylinePts.size() - 1))
	{
		return;
	}

	*x = g_vecPolylinePts[idx].x;
	*y = g_vecPolylinePts[idx].y;
}

void __stdcall GetlinePoint(int idx, int* x1, int* y1, int* x2, int* y2)
{
	if ((idx < 0) || (idx > (g_vecPolylinePts.size() / 2) - 1))
	{
		return;
	}

	*x1 = g_veclinePts[idx * 2].x;
	*y1 = g_vecPolylinePts[idx * 2].y;
	*x2 = g_veclinePts[idx * 2 + 1].x;
	*y2 = g_vecPolylinePts[idx * 2 + 1].y;
}