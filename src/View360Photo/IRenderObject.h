// 日本語。
#pragma once

#include "pch.h"

namespace sample {
    struct IRenderObject {
        virtual ~IRenderObject() = default;

        // Render to swapchain images using stereo image array
        virtual void RenderView(const XrRect2Di& imageRect,
                                const std::vector<xr::math::ViewProjection>& viewProjections,
                                DXGI_FORMAT colorSwapchainFormat,
                                ID3D11Texture2D* colorTexture,
                                DXGI_FORMAT depthSwapchainFormat,
                                ID3D11Texture2D* depthTexture) = 0;
    };

}; // namespace sample

