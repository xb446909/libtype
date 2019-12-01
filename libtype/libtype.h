#pragma once


///
// bPointType: true: 输出点阵，false：输出线段 dx：X方向输出点间隔 dy: Y方向输出间隔
// 如果bPointType为true 用GetPointsCount获得所有点数，用GetPoints获取所有点坐标
///
void __stdcall SetText(int x, int y, const char* str, const char* szFont, int nHeight, bool bBold, bool bItalic, bool bPointType, int dx, int dy);
int __stdcall GetBezierCount();
int __stdcall GetBezierPointsCount(int bezierIndex);
int __stdcall GetPolylineCount();
int __stdcall GetPolylinePointsCount(int polylineIndex);
int __stdcall GetLinePointsCount();
void __stdcall GetBezierPoint(int bezierIndex, int ptIndex, int* x, int* y);
void __stdcall GetPolylinePoint(int polylineIndex, int ptIndex, int* x, int* y);
void __stdcall GetlinePoint(int idx, int* x1, int* y1, int* x2, int* y2);
int __stdcall GetPointsCount();
void __stdcall GetPoints(int idx, int* x, int* y);