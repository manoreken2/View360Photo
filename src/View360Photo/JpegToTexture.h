// 日本語。
#pragma once
#include <string>
#include <d3d11.h>

class JpegToTexture {
public:
    int ImageFileToTexture(
            ID3D11Device* device,
            ID3D11DeviceContext* dctx,
            const wchar_t* path,
            ID3D11Texture2D** tex_r,
            ID3D11ShaderResourceView** srv_r);

    /// @param portion 比率を0～1で指定。nullptrのとき全域をテクスチャーにする。
    int ImageFilePortionToTexture(
        ID3D11Device* device,
        ID3D11DeviceContext* dctx,
        const wchar_t* path,
        const XrRect2Df &portion,
        ID3D11Texture2D** tex_r,
        ID3D11ShaderResourceView** srv_r);
};