#pragma once

void __stdcall SetText(int x, int y, const char* str, const char* szFont, int nHeight, bool bBold, bool bItalic, bool bUnderline);
int __stdcall GetBezierCount();
int __stdcall GetBezierPointsCount(int bezierIndex);
int __stdcall GetPolylineCount();
int __stdcall GetPolylinePointsCount(int polylineIndex);
int __stdcall GetLinePointsCount();
void __stdcall GetBezierPoint(int bezierIndex, int ptIndex, int* x, int* y);
void __stdcall GetPolylinePoint(int polylineIndex, int ptIndex, int* x, int* y);
void __stdcall GetlinePoint(int idx, int* x1, int* y1, int* x2, int* y2);