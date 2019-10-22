// 日本語。

#include "pch.h"
#include "JpegToTexture.h"
#include <windows.h>
#include <minmax.h>
#include <gdiplus.h>
#include "GdiplusHousekeeping.h"


struct WH 
{
    int w;
    int h;
};

static int ExtractJpegToBGRA(const wchar_t * path, D3D11_SUBRESOURCE_DATA &srd_r, WH& wh_r) {
    Gdiplus::BitmapData bd;
    Gdiplus::Bitmap* bm = new Gdiplus::Bitmap(path, TRUE);
    if (bm == nullptr || bm->GetLastStatus() != Gdiplus::Ok) {
        printf("Error: JpegToTexture::LoadFromFile(%S) failed\n", path);
        return E_FAIL;
    }

    int w = bm->GetWidth();
    int h = bm->GetHeight();

    auto r = bm->LockBits(&Gdiplus::Rect(0, 0, w, h), Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bd);
    if (r != Gdiplus::Ok) {
        printf("Error: JpegToTexture::LoadFromFile(%S) failed %s\n", path, GdiplusStatusToStr(r));
        return E_FAIL;
    }

    uint8_t* lockedMem = (uint8_t*)bd.Scan0;

    uint8_t* rawBGRA = new uint8_t[(uint64_t)bd.Stride * h];
    srd_r.pSysMem = (void*)rawBGRA;
    srd_r.SysMemPitch = bd.Stride;
    srd_r.SysMemSlicePitch = 0;

    memcpy(rawBGRA, lockedMem, bd.Stride * h);

    bm->UnlockBits(&bd);

    delete bm;

    wh_r.w = w;
    wh_r.h = h;

    return S_OK;
}

static int
CreateTextureFromMemory(
        ID3D11Device* d3dDevice,
        ID3D11DeviceContext*  dctx,
        const WH& wh_in,
        D3D11_SUBRESOURCE_DATA &srd_in,
        ID3D11Texture2D** tex_r,
        ID3D11ShaderResourceView **srv_r)
{
    assert(tex_r != nullptr);
    assert(srv_r != nullptr);

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = static_cast<UINT>(wh_in.w);
    desc.Height = static_cast<UINT>(wh_in.h);
    desc.MipLevels = static_cast<UINT>(1);
    desc.ArraySize = static_cast<UINT>(1);
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    
    HRESULT hr = d3dDevice->CreateTexture2D(&desc, &srd_in, tex_r);
    if (FAILED(hr)) {
        printf("E: CreateTextureFromMemory() d3dDevice->CreateTexture2D() failed %x\n", hr);
        return hr;
    }
    if (*tex_r == 0) {
        printf("E: CreateTextureFromMemory() d3dDevice->CreateTexture2D() tex_r == nullptr\n");
        return E_FAIL;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = desc.Format;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MipLevels = desc.MipLevels;

    hr = d3dDevice->CreateShaderResourceView(*tex_r, &SRVDesc, srv_r);
    if (FAILED(hr)) {
        (*tex_r)->Release();
        printf("E: CreateTextureFromMemory() d3dDevice->CreateShaderResourceView() failed %x\n", hr);
        return hr;
    }

    // ミップマップを作成します。
    dctx->GenerateMips(*srv_r);

    return S_OK;
}

int
JpegToTexture::LoadFromFile(
        ID3D11Device* device,
        ID3D11DeviceContext* dctx,
        const wchar_t* path,
        ID3D11Texture2D** tex_r, ID3D11ShaderResourceView** srv_r)
{
    D3D11_SUBRESOURCE_DATA srd;
    WH wh;

    int rv = ExtractJpegToBGRA(path, srd, wh);
    if (rv != S_OK) {
        return rv;
    }

    rv = CreateTextureFromMemory(device, dctx, wh, srd, tex_r, srv_r);
    if (rv != S_OK) {
        return rv;
    }

    return S_OK;
}

