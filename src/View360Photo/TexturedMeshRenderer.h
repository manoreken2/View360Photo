// 日本語。
#pragma once

#include <memory>
#include "IRenderObject.h"
#include "TexturedMesh.h"

namespace sample {

    struct TexturedMeshRenderer : IRenderObject {
        TexturedMeshRenderer() = default;
        
        virtual ~TexturedMeshRenderer() = default;

        void InitGraphcisResources(ID3D11Device * device, ID3D11DeviceContext * dctx) {
            m_dev = device;
            m_dctx = dctx;
            InitializeD3DResources();
        }

        void InitializeD3DResources(void);

        // List of color pixel formats supported by this app.
        const std::vector<DXGI_FORMAT>& SupportedColorFormats() const;
        const std::vector<DXGI_FORMAT>& SupportedDepthFormats() const;

        int LoadPlyMesh(const std::wstring& plyPath);

        int LoadTextureFromFile(const wchar_t* path);

        // Render to swapchain images using stereo image array
        void RenderView(
            const XrRect2Di& imageRect,
            const std::vector<xr::math::ViewProjection>& viewProjections,
            DXGI_FORMAT colorSwapchainFormat,
            ID3D11Texture2D* colorTexture,
            DXGI_FORMAT depthSwapchainFormat,
            ID3D11Texture2D* depthTexture) override;

    private:
        TexturedMesh m_mesh;
        ID3D11Device * m_dev = nullptr;
        ID3D11DeviceContext * m_dctx = nullptr;
        winrt::com_ptr<ID3D11VertexShader> m_vertexShader;
        winrt::com_ptr<ID3D11PixelShader> m_pixelShader;
        winrt::com_ptr<ID3D11InputLayout> m_inputLayout;
        winrt::com_ptr<ID3D11Buffer> m_modelCBuffer;
        winrt::com_ptr<ID3D11Buffer> m_viewProjectionCBuffer;
        winrt::com_ptr<ID3D11Buffer> m_vb;
        winrt::com_ptr<ID3D11Buffer> m_ib;
        winrt::com_ptr<ID3D11DepthStencilState> m_reversedZDepthNoStencilTest;
        winrt::com_ptr<ID3D11Texture2D> m_tex;
        winrt::com_ptr<ID3D11ShaderResourceView> m_srv;
        winrt::com_ptr<ID3D11SamplerState> m_sampler;
        winrt::com_ptr<ID3D11RasterizerState> m_rst;
    };

}; // namespace sample
