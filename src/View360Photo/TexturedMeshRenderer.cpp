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
#include "Config.h"

namespace TexturedMeshShader {
    struct Vertex {
        XrVector3f Position;
        XrVector2f Uv;
    };

    struct ModelCB {
		DirectX::XMFLOAT4X4 Model;
	};

    struct ViewProjCB {
        DirectX::XMFLOAT4X4 ViewProjection[2];
    };

	struct AlphaCB {
		XrVector4f Alpha4;
	};

#if NUM_VIEWS != 4
#  error "please fix size of ViewProjection[] below"
#endif
	
	// Separate entrypoints for the vertex and pixel shader functions.
    constexpr char VSShaderHlsl[] = R"_(
        Texture2D g_texture : register(t0);
        SamplerState g_sampler : register(s0);
        cbuffer ModelCB : register(b0) {
	        float4x4 Model;
        };
        cbuffer ViewProjCB : register(b1) {
            float4x4 ViewProjection[4];
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
        )_";

	constexpr char PSShaderHlsl[] = R"_(
        Texture2D g_texture : register(t0);
        SamplerState g_sampler : register(s0);
        cbuffer AlphaCB : register(b0) {
            float4 Alpha4;
        };

        struct VSOutput {
            float4 Pos : SV_POSITION;
            float2 Uv  : TEXCOORD;
            uint viewId : SV_RenderTargetArrayIndex;
        };

        float4 MainPS(VSOutput input) : SV_TARGET {
			float4 bgra = g_texture.Sample(g_sampler, input.Uv);
			bgra.a = Alpha4.w;
            return bgra;
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
            hr = jt.ImageFileToTexture(m_dev, m_dctx, imagePath, leftHalf, (mesh.tex).put(), (mesh.srv).put());
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
            hr = jt.ImageFileToTexture(m_dev, m_dctx, imagePath, rightHalf, (mesh.tex).put(), (mesh.srv).put());
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
		{
			const winrt::com_ptr<ID3DBlob> vertexShaderBytes = sample::dx::CompileShader(TexturedMeshShader::VSShaderHlsl, "MainVS", "vs_5_0");
			CHECK_HRCMD(m_dev->CreateVertexShader(
				vertexShaderBytes->GetBufferPointer(), vertexShaderBytes->GetBufferSize(), nullptr, m_vertexShader.put()));

			const winrt::com_ptr<ID3DBlob> pixelShaderBytes = sample::dx::CompileShader(TexturedMeshShader::PSShaderHlsl, "MainPS", "ps_5_0");
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
		}

		{
			// VS用定数バッファその1．b0
			uint32_t szMC = sizeof(TexturedMeshShader::ModelCB);
			const CD3D11_BUFFER_DESC bd(szMC, D3D11_BIND_CONSTANT_BUFFER);
			CHECK_HRCMD(m_dev->CreateBuffer(&bd, nullptr, m_modelCB.put()));
		}

		{
			// VS用定数バッファその2．b1
			const CD3D11_BUFFER_DESC viewProjectionConstantBufferDesc(sizeof(TexturedMeshShader::ViewProjCB),
																	  D3D11_BIND_CONSTANT_BUFFER);
			CHECK_HRCMD(m_dev->CreateBuffer(&viewProjectionConstantBufferDesc, nullptr, m_viewProjCB.put()));
		}

		{
			// アルファー値定数バッファは、PS用。
			uint32_t szAC = sizeof(TexturedMeshShader::AlphaCB);
			const CD3D11_BUFFER_DESC bd(szAC, D3D11_BIND_CONSTANT_BUFFER);
			CHECK_HRCMD(m_dev->CreateBuffer(&bd, nullptr, m_alphaCB.put()));
		}

		{
			D3D11_FEATURE_DATA_D3D11_OPTIONS3 options;
			m_dev->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &options, sizeof(options));
			CHECK_MSG(options.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer,
					  "This sample requires VPRT support. Adjust sample shaders on GPU without VRPT.");
		}

		{
			CD3D11_DEPTH_STENCIL_DESC dsd(CD3D11_DEFAULT{});
			dsd.DepthEnable = FALSE;
			dsd.StencilEnable = FALSE;
			CHECK_HRCMD(m_dev->CreateDepthStencilState(&dsd, m_noZtestState.put()));
		}

		{
			D3D11_SAMPLER_DESC sampDesc = {};
			sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // D3D11_FILTER_MIN_MAG_MIP_POINT; // ;
			sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			sampDesc.MipLODBias = 0; //< 4等にすると、ボケボケになり、Mipmapが行われていることを確認できる。
			sampDesc.MinLOD = 0;
			sampDesc.MaxLOD = 0; // D3D11_FLOAT32_MAX; //< 0にすると、最も精細なテクスチャーだけが使用される。
			CHECK_HRCMD(m_dev->CreateSamplerState(&sampDesc, m_sampler.put()));
		}

		{
			D3D11_RASTERIZER_DESC rasterDesc;
			rasterDesc.FillMode = D3D11_FILL_SOLID;
			rasterDesc.CullMode = D3D11_CULL_NONE;
			rasterDesc.FrontCounterClockwise = FALSE;
			rasterDesc.DepthBias = 0;
			rasterDesc.DepthBiasClamp = 0.0f;
			rasterDesc.SlopeScaledDepthBias = 0.0f;
			rasterDesc.DepthClipEnable = TRUE;
			rasterDesc.ScissorEnable = FALSE;
			rasterDesc.MultisampleEnable = FALSE;
			rasterDesc.AntialiasedLineEnable = FALSE;
			CHECK_HRCMD(m_dev->CreateRasterizerState(&rasterDesc, m_rst.put()));
		}

		{
			D3D11_BLEND_DESC bd;
			memset(&bd, 0, sizeof bd);
			bd.RenderTarget[0].BlendEnable = TRUE;
			bd.RenderTarget[0].RenderTargetWriteMask = 0x0f;
			bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
			bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;

			CHECK_HRCMD(m_dev->CreateBlendState(&bd, m_addBlend.put()));
		}
    }

    void TexturedMeshRenderer::RenderView(
            const XrRect2Di& imageRect,
			const float alpha,
            const std::vector<xr::math::ViewProjection>& viewProjections,
            DXGI_FORMAT colorSwapchainFormat,
            ID3D11Texture2D* colorTexture,
            DXGI_FORMAT depthSwapchainFormat,
            ID3D11Texture2D* depthTexture)
    {
        const uint32_t viewInstanceCount = (uint32_t)viewProjections.size();
        CHECK_MSG(viewInstanceCount <= NUM_VIEWS,
                  "TexturedMeshShader supports 4 or fewer view instances. Adjust shader to accommodate more.")

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
        m_dctx->OMSetDepthStencilState(m_noZtestState.get(), 0);

        ID3D11RenderTargetView* renderTargets[] = {renderTargetView.get()};
        m_dctx->OMSetRenderTargets((UINT)std::size(renderTargets), renderTargets, depthStencilView.get());

		m_dctx->OMSetBlendState(m_addBlend.get(), nullptr, 0xffffff);

        m_dctx->VSSetShader(m_vertexShader.get(), nullptr, 0);
        m_dctx->PSSetShader(m_pixelShader.get(), nullptr, 0);
		ID3D11SamplerState* ss = m_sampler.get();
		m_dctx->PSSetSamplers(0, 1, &ss);

        m_dctx->RSSetState(m_rst.get());
        m_dctx->IASetInputLayout(m_inputLayout.get());

		ID3D11Buffer* const vscb[] = { m_modelCB.get(), m_viewProjCB.get() };
		m_dctx->VSSetConstantBuffers(0, (UINT)std::size(vscb), vscb);
		ID3D11Buffer* const pscb[] = { m_alphaCB.get() };
		m_dctx->PSSetConstantBuffers(0, (UINT)std::size(pscb), pscb);
		{
			// ModelCBをアップロード。
			XrPosef pose;
			memset(&pose, 0, sizeof pose);
			pose.position.z = 0.0f;//-5.0f;
			pose.orientation.z = 1.0f;
			float scale = 15.0f; //  radius meters

			// 姿勢行列 modelを作成。
			TexturedMeshShader::ModelCB model;
			const DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(scale, scale, scale);
			DirectX::XMStoreFloat4x4(&model.Model, DirectX::XMMatrixTranspose(scaleMatrix * xr::math::LoadXrPose(pose)));

			m_dctx->UpdateSubresource(m_modelCB.get(), 0, nullptr, &model, 0, 0);
		}

		{
			// ViewProjCBをアップロード。
			TexturedMeshShader::ViewProjCB vpcb = {};

			for (uint32_t k = 0; k < viewInstanceCount; k++) {
				const DirectX::XMMATRIX spaceToView = xr::math::LoadInvertedXrPose(viewProjections[k].Pose);
				const DirectX::XMMATRIX projectionMatrix = ComposeProjectionMatrix(viewProjections[k].Fov, viewProjections[k].NearFar);

				// Set view projection matrix for each view, transpose for shader usage.
				DirectX::XMStoreFloat4x4(&vpcb.ViewProjection[k],
					DirectX::XMMatrixTranspose(spaceToView * projectionMatrix));
			}
			m_dctx->UpdateSubresource(m_viewProjCB.get(), 0, nullptr, &vpcb, 0, 0);
		}

		{
			// アルファー値。
			TexturedMeshShader::AlphaCB acb;
			acb.Alpha4.x = 1.0f;
			acb.Alpha4.y = 1.0f;
			acb.Alpha4.z = 1.0f;
			acb.Alpha4.w = alpha;
			m_dctx->UpdateSubresource(m_alphaCB.get(), 0, nullptr, &acb, 0, 0);
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
