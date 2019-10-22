// 日本語。
#pragma once

#include "pch.h"
#include "XyzUv.h"
#include <vector>
#include <stdint.h>

struct TexturedMesh {
    std::vector<XyzUv> vertexList;
    std::vector<uint32_t> triangleIdxList;

    uint32_t NumTriangles(void) {
        return (uint32_t)(triangleIdxList.size() / 3);
    }

    void Clear(void) {
        vertexList.clear();
        triangleIdxList.clear();
    }
};

