// 日本語。
#pragma once

#include <memory>
#include "IRenderObject.h"
#include "Cube.h"
namespace sample {

    struct CubeRenderer : IRenderObject {
        virtual ~CubeRenderer() = default;

        void InitGraphcisResources(ID3D11Device* dev, ID3D11DeviceContext* dctx) {
            m_dev = dev;
            m_dctx = dctx;
            InitializeD3DResources();
        }

        void SetCubes(std::vector<const sample::Cube*>& cubes) {
            mCubes = &cubes;
        }

        // Render to swapchain images using stereo image array
        virtual void RenderView(const XrRect2Di& imageRect,
                                const std::vector<xr::math::ViewProjection>& viewProjections,
                                DXGI_FORMAT colorSwapchainFormat,
                                ID3D11Texture2D* colorTexture,
                                DXGI_FORMAT depthSwapchainFormat,
                                ID3D11Texture2D* depthTexture);

    private:
        std::vector<const sample::Cube*>* mCubes = nullptr;
        ID3D11Device* m_dev = nullptr;
        ID3D11DeviceContext* m_dctx = nullptr;
        winrt::com_ptr<ID3D11VertexShader> m_vertexShader;
        winrt::com_ptr<ID3D11PixelShader> m_pixelShader;
        winrt::com_ptr<ID3D11InputLayout> m_inputLayout;
        winrt::com_ptr<ID3D11Buffer> m_modelCBuffer;
        winrt::com_ptr<ID3D11Buffer> m_viewProjectionCBuffer;
        winrt::com_ptr<ID3D11Buffer> m_cubeVertexBuffer;
        winrt::com_ptr<ID3D11Buffer> m_cubeIndexBuffer;
        winrt::com_ptr<ID3D11DepthStencilState> m_reversedZDepthNoStencilTest;

        void InitializeD3DResources(void);
    };

    std::unique_ptr<CubeRenderer> CreateCubeRenderer(void);
};
