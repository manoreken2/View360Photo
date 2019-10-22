// 日本語。
#include "pch.h"
#include "GdiplusHousekeeping.h"


const char* GdiplusStatusToStr(Gdiplus::Status s) {
    switch (s) {
    case Gdiplus::Ok:
        return "Ok";
    case Gdiplus::GenericError:
        return "GenericError";
    case Gdiplus::InvalidParameter:
        return "InvalidParameter";
    case Gdiplus::OutOfMemory:
        return "OutOfMemory";
    case Gdiplus::ObjectBusy:
        return "ObjectBusy";
    case Gdiplus::InsufficientBuffer:
        return "InsufficientBuffer";
    case Gdiplus::NotImplemented:
        return "NotImplemented";
    case Gdiplus::Win32Error:
        return "Win32Error";
    case Gdiplus::WrongState:
        return "WrongState";
    case Gdiplus::Aborted:
        return "Aborted";
    case Gdiplus::FileNotFound:
        return "FileNotFound";
    case Gdiplus::ValueOverflow:
        return "ValueOverflow";
    case Gdiplus::AccessDenied:
        return "AccessDenied";
    case Gdiplus::UnknownImageFormat:
        return "UnknownImageFormat";
    case Gdiplus::FontFamilyNotFound:
        return "FontFamilyNotFound";
    case Gdiplus::FontStyleNotFound:
        return "FontStyleNotFound";
    case Gdiplus::NotTrueTypeFont:
        return "NotTrueTypeFont";
    case Gdiplus::UnsupportedGdiplusVersion:
        return "UnsupportedGdiplusVersion";
    case Gdiplus::GdiplusNotInitialized:
        return "GdiplusNotInitialized";
    case Gdiplus::PropertyNotFound:
        return "PropertyNotFound";
    case Gdiplus::PropertyNotSupported:
        return "PropertyNotSupported";
    default:
        return "Unknown error";
    }
}

static ULONG_PTR gdiplusToken = 0;

int GdiplusHousekeepingInit(void) {
    Gdiplus::GdiplusStartupInput gpsi;
    gpsi.GdiplusVersion = 1;
    gpsi.DebugEventCallback = nullptr;
    gpsi.SuppressBackgroundThread = FALSE;
    gpsi.SuppressExternalCodecs = FALSE;
    Gdiplus::Status s = Gdiplus::GdiplusStartup(&gdiplusToken, &gpsi, nullptr);
    if (s != Gdiplus::Ok) {
        printf("Error: GdiplusHousekeepingInit() GdiplusStartup() failed %s\n", GdiplusStatusToStr(s));
        return E_FAIL;
    }

    return S_OK;
}

void GdiplusHousekeepingTerm(void)
{
    if (gdiplusToken != 0) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        gdiplusToken = 0;
    }
}

