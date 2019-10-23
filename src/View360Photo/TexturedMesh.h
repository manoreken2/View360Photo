// 日本語。
#pragma once

#include "pch.h"
#include "XyzUv.h"
#include <vector>
#include <stdint.h>

struct TexturedMesh {
    std::vector<XyzUv> vertexList;
    std::vector<uint32_t> triangleIdxList;
    winrt::com_ptr<ID3D11Texture2D> tex;
    winrt::com_ptr<ID3D11ShaderResourceView> srv;
    winrt::com_ptr<ID3D11Buffer> vb;
    winrt::com_ptr<ID3D11Buffer> ib;

    uint32_t NumTriangles(void) {
        return (uint32_t)(triangleIdxList.size() / 3);
    }

    void Clear(void) {
        vertexList.clear();
        triangleIdxList.clear();
        if (tex.get() != nullptr) {
            tex->Release();
        }
        if (srv.get() != nullptr) {
            srv->Release();
        }
		if (vb.get() != nullptr) {
			vb->Release();
		}
		if (ib.get() != nullptr) {
			ib->Release();
		}
	}
};

