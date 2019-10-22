// 日本語。
#pragma once
#include <windows.h>
#include <minmax.h>
#include <gdiplus.h>

const char* GdiplusStatusToStr(Gdiplus::Status s);

int GdiplusHousekeepingInit(void);
void GdiplusHousekeepingTerm(void);
