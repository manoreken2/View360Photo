// 日本語。
#pragma once

#include <memory>
#include "IRenderObject.h"
#include "TexturedMesh.h"

namespace sample {

    struct TexturedMeshRenderer {
        TexturedMeshRenderer() = default;
        
        virtual ~TexturedMeshRenderer() = default;

		void InitGraphcisResources(ID3D11Device * device, ID3D11DeviceContext * dctx) {
            m_dev = device;
            m_dctx = dctx;
            InitializeD3DResources();
        }

		int Load(const wchar_t *imagePath);

        // Render to swapchain images using stereo image array
        void RenderView(
            const XrRect2Di& imageRect,
			const float alpha,
            const std::vector<xr::math::ViewProjection>& viewProjections,
            DXGI_FORMAT colorSwapchainFormat,
            ID3D11Texture2D* colorTexture,
            DXGI_FORMAT depthSwapchainFormat,
            ID3D11Texture2D* depthTexture);

    private:
        static const int N_MESH = 2;
        TexturedMesh m_meshes[N_MESH];
        ID3D11Device * m_dev = nullptr;
        ID3D11DeviceContext * m_dctx = nullptr;
        winrt::com_ptr<ID3D11VertexShader> m_vertexShader;
        winrt::com_ptr<ID3D11PixelShader> m_pixelShader;
        winrt::com_ptr<ID3D11InputLayout> m_inputLayout;
        winrt::com_ptr<ID3D11Buffer> m_modelCB;
        winrt::com_ptr<ID3D11Buffer> m_viewProjCB;
		winrt::com_ptr<ID3D11Buffer> m_alphaCB;
		winrt::com_ptr<ID3D11DepthStencilState> m_noZtestState;
        winrt::com_ptr<ID3D11SamplerState> m_sampler;
        winrt::com_ptr<ID3D11RasterizerState> m_rst;
		winrt::com_ptr<ID3D11BlendState> m_addBlend;
		void InitializeD3DResources(void);
	};

}; // namespace sample
