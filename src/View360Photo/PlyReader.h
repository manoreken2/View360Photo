// 日本語。
#pragma once

#include "pch.h"
#include "TexturedMesh.h"
#include <stdio.h>
#include <stdint.h>

class PlyReader {
public:
    int Read(const std::wstring& path, TexturedMesh& tm_return);

private:
    FILE* mFp = nullptr;
    TexturedMesh* mTM = nullptr;
    bool mIsBinary = false;
    int mNumVtx = 0;
    int mNumFace = 0;

    enum State {
        ST_ReadSignature,
        ST_ReadFormat,
        ST_ReadHeader,
        ST_ReadHeaderVtx,
        ST_ReadHeaderFace,
        ST_ReadVertex,
        ST_ReadFace,
        ST_Finish,
    };
    State mState = ST_ReadSignature;

    enum Property {
        PP_X = 1,
        PP_Y = 2,
        PP_Z = 4,
        PP_NX = 8,
        PP_NY = 16,

        PP_NZ = 32,
        PP_S = 64,
        PP_T = 128,
        PP_VTX_IDX = 256,
    };
    uint32_t mVtxProp = 0;
    uint32_t mFaceProp = 0;

    enum VtxPropType {
        VPT_Unknown,
        VPT_XYZ_NXNYNZ_ST,
        VPT_XYZ_ST,
    };
    VtxPropType mVtxPropType = VPT_Unknown;


    int Read1(void);
    int ReadSignature(void);
    int ReadFormat(void);
    int ReadHeader(void);
    int ReadVertex(void);
    int ReadFace(void);
};
