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

/// @param portion 比率を0～1で指定。nullptrのとき全域をテクスチャーにする。
static int ExtractJpegToBGRA(const wchar_t * path, const XrRect2Df *portion, const uint8_t alpha, D3D11_SUBRESOURCE_DATA &srd_r, WH& wh_r) {
    Gdiplus::BitmapData bd;
    Gdiplus::Bitmap* bm = new Gdiplus::Bitmap(path, TRUE);
    if (bm == nullptr || bm->GetLastStatus() != Gdiplus::Ok) {
        printf("Error: JpegToTexture::LoadFromFile(%S) failed\n", path);
        return E_FAIL;
    }

    int origW = bm->GetWidth();
    int origH = bm->GetHeight();

    auto origR = Gdiplus::Rect(0, 0, origW, origH);
	auto cropR = origR;
	if (portion != nullptr) {
		cropR = Gdiplus::Rect(
			(int)(portion->offset.x * origW),
			(int)(portion->offset.y * origH),
			(int)(portion->extent.width * origW),
			(int)(portion->extent.height * origH)
			);
	}
	int cropW = cropR.Width;
	int cropH = cropR.Height;

    auto r = bm->LockBits(&origR, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bd);
    if (r != Gdiplus::Ok) {
        printf("Error: JpegToTexture::LoadFromFile(%S) failed %s\n", path, GdiplusStatusToStr(r));
        return E_FAIL;
    }

    uint8_t* lockedMem = (uint8_t*)bd.Scan0;

    uint8_t* rawBGRA = new uint8_t[4 * cropW * cropH];
    srd_r.pSysMem = (void*)rawBGRA;
    srd_r.SysMemPitch = 4 * cropW;
    srd_r.SysMemSlicePitch = 0;

	for (int y = 0; y < cropH; ++y) {
		uint8_t *fromPos = lockedMem + cropR.X * 4 + bd.Stride * (y + cropR.Y);
		memcpy(&rawBGRA[4 * cropW * y], fromPos, 4 * cropW);
	}

	if (alpha != 0xff) {
		const uint32_t alpha32 = alpha << 24;
		uint32_t *p = (uint32_t*)rawBGRA;
		for (int y = 0; y < cropH; ++y) {
			for (int x = 0; x < cropW; ++x) {
				*p = (*p & 0x00ffffff) | alpha32;
				++p;
			}
		}
	}

    bm->UnlockBits(&bd);

    delete bm;

    wh_r.w = cropW;
    wh_r.h = cropH;

    return S_OK;
}

static int
CreateTextureFromMemory(
        ID3D11Device* dev,
        ID3D11DeviceContext*  dctx,
        const WH& wh_in,
        D3D11_SUBRESOURCE_DATA &srd_in,
        ID3D11Texture2D** tex_r,
        ID3D11ShaderResourceView **srv_r)
{
	HRESULT hr;
    assert(tex_r != nullptr);
    assert(srv_r != nullptr);

	D3D11_TEXTURE2D_DESC desc = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_B8G8R8A8_UNORM, wh_in.w, wh_in.h);
	desc.MipLevels = 0;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    hr = dev->CreateTexture2D(&desc, nullptr, tex_r);
    if (FAILED(hr)) {
        printf("E: CreateTextureFromMemory() d3dDevice->CreateTexture2D() failed %x\n", hr);
        return hr;
    }
    if (*tex_r == 0) {
        printf("E: CreateTextureFromMemory() d3dDevice->CreateTexture2D() tex_r == nullptr\n");
        return E_FAIL;
    }

	dctx->UpdateSubresource(*tex_r, 0, nullptr, srd_in.pSysMem, srd_in.SysMemPitch, 0);

    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format = desc.Format;
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	sd.Texture2D.MipLevels = -1;
	sd.Texture2D.MostDetailedMip = 0;

    hr = dev->CreateShaderResourceView(*tex_r, &sd, srv_r);
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
JpegToTexture::ImageFileToTexture(
        ID3D11Device* device,
        ID3D11DeviceContext* dctx,
        const wchar_t* path,
        const XrRect2Df &portion,
        ID3D11Texture2D** tex_r,
        ID3D11ShaderResourceView** srv_r,
		uint8_t alpha)
{
    D3D11_SUBRESOURCE_DATA srd;
    WH wh;

	assert(0.0f <= portion.offset.x);
	assert(0.0f <= portion.offset.y);
	assert(portion.offset.x + portion.extent.width <= 1.0f);
	assert(portion.offset.y + portion.extent.height <= 1.0f);

    int hr = ExtractJpegToBGRA(path, &portion, alpha, srd, wh);
    if (FAILED(hr)) {
        return hr;
    }

    hr = CreateTextureFromMemory(device, dctx, wh, srd, tex_r, srv_r);
	if (FAILED(hr)) {
		return hr;
    }

    return S_OK;
}
