// 日本語。

//*********************************************************
//    Copyright (c) Microsoft. All rights reserved.
//
//    Apache 2.0 License
//
//    You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
//
//*********************************************************

#include "pch.h"
#include "TexturedMeshRenderer.h"
#include "DxUtility.h"
#include "PlyReader.h"
#include "JpegToTexture.h"

namespace TexturedMeshShader {
    struct Vertex {
        XrVector3f Position;
        XrVector2f Uv;
    };

    struct ModelConstantBuffer {
        DirectX::XMFLOAT4X4 Model;
    };

    struct ViewProjectionConstantBuffer {
        DirectX::XMFLOAT4X4 ViewProjection[2];
    };

    constexpr uint32_t MaxViewInstance = 2;

    // Separate entrypoints for the vertex and pixel shader functions.
    constexpr char ShaderHlsl[] = R"_(
        Texture2D g_texture : register(t0);
        SamplerState g_sampler : register(s0);
        cbuffer ModelConstantBuffer : register(b0) {
            float4x4 Model;
        };
        cbuffer ViewProjectionConstantBuffer : register(b1) {
            float4x4 ViewProjection[2];
        };

        struct VSOutput {
            float4 Pos : SV_POSITION;
            float2 Uv  : TEXCOORD;
            uint viewId : SV_RenderTargetArrayIndex;
        };
        struct VSInput {
            float3 Pos : POSITION;
            float2 Uv  : TEXCOORD;
            uint instId : SV_InstanceID;
        };

        VSOutput MainVS(VSInput input) {
            VSOutput output;
            output.Pos = mul(mul(float4(input.Pos, 1), Model), ViewProjection[input.instId]);
            output.Uv = input.Uv;
            output.viewId = input.instId;
            return output;
        }

        float4 MainPS(VSOutput input) : SV_TARGET {
            return g_texture.Sample(g_sampler, input.Uv);
        }
        )_";

} // namespace TexturedMeshShader

namespace sample {
    int TexturedMeshRenderer::Load(const wchar_t *imagePath) {
        int hr;
        {
            TexturedMesh &mesh = m_meshes[0];
            mesh.Clear();

            PlyReader pr;
            hr = pr.Read(L"sphereL.ply", mesh);
            if (FAILED(hr)) {
                return hr;
            }

            JpegToTexture jt;
            XrRect2Df leftHalf{ 0, 0, 0.5f, 1.0f };
            hr = jt.ImageFilePortionToTexture(m_dev, m_dctx, imagePath, leftHalf, (mesh.tex).put(), (mesh.srv).put());
            if (FAILED(hr)) {
                return hr;
            }
        }
        {
            TexturedMesh &mesh = m_meshes[1];
            mesh.Clear();

            PlyReader pr;
            hr = pr.Read(L"sphereR.ply", mesh);
            if (FAILED(hr)) {
                return hr;
            }

            JpegToTexture jt;
            XrRect2Df rightHalf{ 0.5f, 0, 0.5f, 1.0f };
            hr = jt.ImageFilePortionToTexture(m_dev, m_dctx, imagePath, rightHalf, (mesh.tex).put(), (mesh.srv).put());
            if (FAILED(hr)) {
                return hr;
            }
        }

		for (int i = 0; i < N_MESH; ++i) {
			TexturedMesh &tm = m_meshes[i];
			const D3D11_SUBRESOURCE_DATA vertexBufferData{ &tm.vertexList[0] };
			const CD3D11_BUFFER_DESC vertexBufferDesc((uint32_t)(sizeof(XyzUv) * tm.vertexList.size()), D3D11_BIND_VERTEX_BUFFER);
			CHECK_HRCMD(m_dev->CreateBuffer(&vertexBufferDesc, &vertexBufferData, tm.vb.put()));

			// triangle index要素のサイズ(4バイト)は後でIASetIndexBuffer()で指定する。
			const D3D11_SUBRESOURCE_DATA indexBufferData{ &tm.triangleIdxList[0] };
			const CD3D11_BUFFER_DESC indexBufferDesc((uint32_t)(sizeof(uint32_t) * tm.triangleIdxList.size()), D3D11_BIND_INDEX_BUFFER);
			CHECK_HRCMD(m_dev->CreateBuffer(&indexBufferDesc, &indexBufferData, tm.ib.put()));
		}

        return hr;
    }

    void TexturedMeshRenderer::InitializeD3DResources(void) {
        const winrt::com_ptr<ID3DBlob> vertexShaderBytes = sample::dx::CompileShader(TexturedMeshShader::ShaderHlsl, "MainVS", "vs_5_0");
        CHECK_HRCMD(m_dev->CreateVertexShader(
            vertexShaderBytes->GetBufferPointer(), vertexShaderBytes->GetBufferSize(), nullptr, m_vertexShader.put()));

        const winrt::com_ptr<ID3DBlob> pixelShaderBytes = sample::dx::CompileShader(TexturedMeshShader::ShaderHlsl, "MainPS", "ps_5_0");
        CHECK_HRCMD(m_dev->CreatePixelShader(
            pixelShaderBytes->GetBufferPointer(), pixelShaderBytes->GetBufferSize(), nullptr, m_pixelShader.put()));

        const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };

        CHECK_HRCMD(m_dev->CreateInputLayout(vertexDesc,
                                                (UINT)std::size(vertexDesc),
                                                vertexShaderBytes->GetBufferPointer(),
                                                vertexShaderBytes->GetBufferSize(),
                                                m_inputLayout.put()));

        const CD3D11_BUFFER_DESC modelConstantBufferDesc(sizeof(TexturedMeshShader::ModelConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
        CHECK_HRCMD(m_dev->CreateBuffer(&modelConstantBufferDesc, nullptr, m_modelCBuffer.put()));

        const CD3D11_BUFFER_DESC viewProjectionConstantBufferDesc(sizeof(TexturedMeshShader::ViewProjectionConstantBuffer),
                                                                  D3D11_BIND_CONSTANT_BUFFER);
        CHECK_HRCMD(m_dev->CreateBuffer(&viewProjectionConstantBufferDesc, nullptr, m_viewProjectionCBuffer.put()));

        D3D11_FEATURE_DATA_D3D11_OPTIONS3 options;
        m_dev->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &options, sizeof(options));
        CHECK_MSG(options.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer,
                  "This sample requires VPRT support. Adjust sample shaders on GPU without VRPT.");

        CD3D11_DEPTH_STENCIL_DESC depthStencilDesc(CD3D11_DEFAULT{});
        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER;
        CHECK_HRCMD(m_dev->CreateDepthStencilState(&depthStencilDesc, m_reversedZDepthNoStencilTest.put()));

        D3D11_SAMPLER_DESC sampDesc = {};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MinLOD = 0;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
        CHECK_HRCMD(m_dev->CreateSamplerState(&sampDesc, m_sampler.put()));

        D3D11_RASTERIZER_DESC rasterDesc;
        rasterDesc.AntialiasedLineEnable = FALSE;
        rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.DepthBias = 0;
        rasterDesc.DepthBiasClamp = 0.0f;
        rasterDesc.DepthClipEnable = TRUE;
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.FrontCounterClockwise = FALSE;
        rasterDesc.MultisampleEnable = FALSE;
        rasterDesc.ScissorEnable = FALSE;
        rasterDesc.SlopeScaledDepthBias = 0.0f;
        CHECK_HRCMD(m_dev->CreateRasterizerState(&rasterDesc, m_rst.put()));
    }

    void TexturedMeshRenderer::RenderView(
            const XrRect2Di& imageRect,
            const std::vector<xr::math::ViewProjection>& viewProjections,
            DXGI_FORMAT colorSwapchainFormat,
            ID3D11Texture2D* colorTexture,
            DXGI_FORMAT depthSwapchainFormat,
            ID3D11Texture2D* depthTexture)
    {
        const uint32_t viewInstanceCount = (uint32_t)viewProjections.size();
        CHECK_MSG(viewInstanceCount <= TexturedMeshShader::MaxViewInstance,
                  "TexturedMeshShader supports 2 or fewer view instances. Adjust shader to accommodate more.")

        CD3D11_VIEWPORT viewport(
            (float)imageRect.offset.x, (float)imageRect.offset.y, (float)imageRect.extent.width, (float)imageRect.extent.height);
        m_dctx->RSSetViewports(1, &viewport);

        // Create RenderTargetView with the original swapchain format (swapchain image is typeless).
        winrt::com_ptr<ID3D11RenderTargetView> renderTargetView;
        const CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(D3D11_RTV_DIMENSION_TEXTURE2DARRAY, colorSwapchainFormat);
        CHECK_HRCMD(m_dev->CreateRenderTargetView(colorTexture, &renderTargetViewDesc, renderTargetView.put()));

        // Create a DepthStencilView with the original swapchain format (swapchain image is typeless)
        winrt::com_ptr<ID3D11DepthStencilView> depthStencilView;
        CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2DARRAY, depthSwapchainFormat);
        CHECK_HRCMD(m_dev->CreateDepthStencilView(depthTexture, &depthStencilViewDesc, depthStencilView.put()));

        const bool reversedZ = viewProjections[0].NearFar.Near > viewProjections[0].NearFar.Far;
        m_dctx->OMSetDepthStencilState(reversedZ ? m_reversedZDepthNoStencilTest.get() : nullptr, 0);

        ID3D11RenderTargetView* renderTargets[] = {renderTargetView.get()};
        m_dctx->OMSetRenderTargets((UINT)std::size(renderTargets), renderTargets, depthStencilView.get());

        ID3D11Buffer* const constantBuffers[] = {m_modelCBuffer.get(), m_viewProjectionCBuffer.get()};
        m_dctx->VSSetConstantBuffers(0, (UINT)std::size(constantBuffers), constantBuffers);
        m_dctx->VSSetShader(m_vertexShader.get(), nullptr, 0);
        m_dctx->PSSetShader(m_pixelShader.get(), nullptr, 0);

        TexturedMeshShader::ViewProjectionConstantBuffer vpcb;

        for (uint32_t k = 0; k < viewInstanceCount; k++) {
            const DirectX::XMMATRIX spaceToView = xr::math::LoadInvertedXrPose(viewProjections[k].Pose);
            const DirectX::XMMATRIX projectionMatrix = ComposeProjectionMatrix(viewProjections[k].Fov, viewProjections[k].NearFar);

            // Set view projection matrix for each view, transpose for shader usage.
            DirectX::XMStoreFloat4x4(&vpcb.ViewProjection[k],
                                     DirectX::XMMatrixTranspose(spaceToView * projectionMatrix));
        }
        m_dctx->UpdateSubresource(m_viewProjectionCBuffer.get(), 0, nullptr, &vpcb, 0, 0);

        m_dctx->RSSetState(m_rst.get());
        ID3D11SamplerState* ss = m_sampler.get();
        m_dctx->PSSetSamplers(0, 1, &ss);
        m_dctx->IASetInputLayout(m_inputLayout.get());

        {
            XrPosef pose;
            memset(&pose, 0, sizeof pose);
            pose.position.z = 0.0f;//-5.0f;
            pose.orientation.z = 1.0f;
            float scale = 15.0f; //  radius meters

            // 姿勢行列 modelを作成。
            TexturedMeshShader::ModelConstantBuffer model;
            const DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(scale, scale, scale);
            DirectX::XMStoreFloat4x4(&model.Model, DirectX::XMMatrixTranspose(scaleMatrix * xr::math::LoadXrPose(pose)));
            m_dctx->UpdateSubresource(m_modelCBuffer.get(), 0, nullptr, &model, 0, 0);
        }

        for (int i = 0; i < N_MESH; ++i) {
            TexturedMesh &tm = m_meshes[i];

            // Set primitive data.
            ID3D11ShaderResourceView* srv = tm.srv.get();
            m_dctx->PSSetShaderResources(0, 1, &srv);

            const UINT strides[] = { sizeof(TexturedMeshShader::Vertex) };
            const UINT offsets[] = { 0 };
            ID3D11Buffer* vertexBuffers[] = { tm.vb.get() };
            m_dctx->IASetVertexBuffers(0, (UINT)std::size(vertexBuffers), vertexBuffers, strides, offsets);
            m_dctx->IASetIndexBuffer(tm.ib.get(), DXGI_FORMAT_R32_UINT, 0);
            m_dctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // Draw.
            m_dctx->DrawIndexedInstanced((uint32_t)(tm.triangleIdxList.size()), viewInstanceCount, 0, 0, 0);
        }
    }

} // namespace sample
