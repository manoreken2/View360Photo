// 日本語。

#include "pch.h"
#include "PlyReader.h"
#include <stdio.h>
#include <assert.h>

int PlyReader::Read(const std::wstring& path, TexturedMesh& tm_r) {
    tm_r.Clear();
    mTM = &tm_r;
    mVtxProp = 0;
    mFaceProp = 0;
    mNumVtx = 0;
    mNumFace = 0;

    mFp = nullptr;
    errno_t er = _wfopen_s(&mFp, path.c_str(), L"rb");
    if (er != 0) {
        printf("E: PlyReader::Read(%S) failed.\n", path.c_str());
        return E_FAIL;
    }

    int rv = Read1();

    fclose(mFp);
    mFp = nullptr;

    return rv;
}

int
PlyReader::Read1(void)
{
    int rv = 0;
    bool bDone = false;
    assert(mFp);
    assert(mTM);

    mState = ST_ReadSignature;
    while (!bDone) {
        switch (mState) {
        case ST_ReadSignature:
            rv = ReadSignature();
            break;
        case ST_ReadFormat:
            rv = ReadFormat();
            break;
        case ST_ReadHeader:
        case ST_ReadHeaderVtx:
        case ST_ReadHeaderFace:
            rv = ReadHeader();
            break;
        case ST_ReadVertex:
            rv = ReadVertex();
            break;
        case ST_ReadFace:
            rv = ReadFace();
            break;
        case ST_Finish:
            bDone = true;
            break;
        default:
            assert(0);
            break;
        }

        if (rv != S_OK) {
            printf("E: PlyReader::Read1() failed.\n");
            break;
        }
    }

    return rv;
}

static char *
FGetS(char *s, int sizeof_s, FILE *fp)
{
    memset(s, 0, sizeof_s);
    char *r = fgets(s, sizeof_s-1, fp);
    if (r == nullptr) {
        return r;
    }

    // trim last LF
    int len = (int)strlen(s);
    if (s[len - 1] == '\n') {
        s[len - 1] = 0;
    }

    // trim last CR
    len = (int)strlen(s);
    if (s[len - 1] == '\r') {
        s[len - 1] = 0;
    }
    return r;
}

int
PlyReader::ReadSignature(void)
{
    char s[256];

    char *r = FGetS(s, sizeof s, mFp);
    if (r == nullptr) {
        printf("E: PlyReader::ReadSignature() failed.\n");
        return E_FAIL;
    }

    if (0 != strcmp("ply", r)) {
        printf("E: PlyReader::ReadSignature() Not PLY file.\n");
        return E_FAIL;
    }

    mState = ST_ReadFormat;
    return S_OK;
}

int
PlyReader::ReadFormat(void)
{
    char seps[] = " \n";
    char* tknFmt     = nullptr;
    char* tknType    = nullptr;
    char* tknVersion = nullptr;
    char* nextTkn    = nullptr;
    char s[256];

    char* r = FGetS(s, sizeof s, mFp);
    if (r == nullptr) {
        printf("E: PlyReader::ReadFormat() failed.\n");
        return E_FAIL;
    }

    tknFmt     = strtok_s(s, seps, &nextTkn);
    tknType    = strtok_s(nullptr, seps, &nextTkn);
    tknVersion = strtok_s(nullptr, seps, &nextTkn);

    if (0 != strcmp("format", tknFmt)) {
        printf("E: PlyReader::ReadFormat() Not PLY file.\n");
        return E_FAIL;
    }
    if (0 == strcmp("binary", tknType)) {
        mIsBinary = true;
    } else if (0 == strcmp("ascii", tknType)) {
        mIsBinary = false;
    } else {
        printf("E: PlyReader::ReadFormat() unknown format type.\n");
        return E_FAIL;
    }
    if (0 != strcmp("1.0", tknVersion)) {
        printf("E: PlyReader::ReadFormat() unknown PLY version %s.\n", tknVersion);
        return E_FAIL;
    }

    mState = ST_ReadHeader;
    return S_OK;
}

int
PlyReader::ReadHeader(void)
{
    int rv = 0;
    char seps[] = " \r\n";
    char* tkn = nullptr;
    char* nextTkn = nullptr;
    char s[256];

    char* r = FGetS(s, sizeof s, mFp);
    if (r == nullptr) {
        printf("E: PlyReader::ReadHeader() failed.\n");
        return E_FAIL;
    }

    tkn = strtok_s(s, seps, &nextTkn);
    if (nullptr == tkn) {
        printf("E: PlyReader::ReadHeader() no token found error\n");
        return E_FAIL;
    }

    if (0 == strcmp("end_header", tkn)) {
        // ヘッダー終わり。
        // ヘッダーデータをチェックする。
        if (mNumVtx <= 0) {
            printf("E: PlyReader::ReadHeader() header does not contain \"element vertex\" error\n");
            return E_FAIL;
        }
        if (mVtxProp == 0) {
            printf("E: PlyReader::ReadHeader() header does not contain vertex property error\n");
            return E_FAIL;
        }

        if (mVtxProp == (PP_X | PP_Y | PP_Z | PP_NX | PP_NY | PP_NZ | PP_S | PP_T)) {
            mVtxPropType = VPT_XYZ_NXNYNZ_ST;
        } else if (mVtxProp == (PP_X | PP_Y | PP_Z | PP_S | PP_T)) {
            mVtxPropType = VPT_XYZ_ST;
        } else {
            mVtxPropType = VPT_Unknown;
            printf("E: PlyReader::ReadHeader() unknown property type\n");
            return E_FAIL;
        }

        mState = ST_ReadVertex;
        return S_OK;
    }

    if (0 == strcmp("comment", tkn)) {
        // コメント。スルーする。
        return S_OK;
    }

    if (0 == strcmp("element", tkn)) {
        // element vertex nVtx
        // element face nFace
        tkn = strtok_s(nullptr, seps, &nextTkn);
        if (nullptr == tkn) {
            printf("E: PlyReader::ReadHeader() element token error\n");
            return E_FAIL;
        }

        if (0 == strcmp("vertex", tkn)) {
            tkn = strtok_s(nullptr, seps, &nextTkn);
            if (nullptr == tkn) {
                printf("E: PlyReader::ReadHeader() element vertex token error\n");
                return E_FAIL;
            }
            
            rv = sscanf_s(tkn, "%d", &mNumVtx);
            if (rv != 1 || mNumVtx <= 0) {
                printf("E: PlyReader::ReadHeader() element vertex number error\n");
                return E_FAIL;
            }
            mState = ST_ReadHeaderVtx;
        }

        if (0 == strcmp("face", tkn)) {
            tkn = strtok_s(nullptr, seps, &nextTkn);
            if (nullptr == tkn) {
                printf("E: PlyReader::ReadHeader() element face token error\n");
                return E_FAIL;
            }

            rv = sscanf_s(tkn, "%d", &mNumFace);
            if (rv != 1 || mNumFace <= 0) {
                printf("E: PlyReader::ReadHeader() element face number error\n");
                return E_FAIL;
            }
            mState = ST_ReadHeaderFace;
        }
        return S_OK;
    }

    if (0 == strcmp("property", tkn)) {
        tkn = strtok_s(nullptr, seps, &nextTkn);
        if (nullptr == tkn) {
            printf("E: PlyReader::ReadHeader() property read error\n");
            return E_FAIL;
        }

        if (0 == strcmp("float", tkn)) {
            tkn = strtok_s(nullptr, seps, &nextTkn);
            if (nullptr == tkn) {
                printf("E: PlyReader::ReadHeader() property float read error\n");
                return E_FAIL;
            }

            if (0 == strcmp("x", tkn)) {
                mVtxProp |= PP_X;
            } else if (0 == strcmp("y", tkn)) {
                mVtxProp |= PP_Y;
            } else if (0 == strcmp("z", tkn)) {
                mVtxProp |= PP_Z;
            } else if (0 == strcmp("nx", tkn)) {
                mVtxProp |= PP_NX;
            } else if (0 == strcmp("ny", tkn)) {
                mVtxProp |= PP_NY;
            } else if (0 == strcmp("nz", tkn)) {
                mVtxProp |= PP_NZ;
            } else if (0 == strcmp("s", tkn)) {
                mVtxProp |= PP_S;
            } else if (0 == strcmp("t", tkn)) {
                mVtxProp |= PP_T;
            } else {
                printf("D: PlyReader::ReadHeader() unknown float plop %s\n", tkn);
                // スルーする。
            }
            return S_OK;
        }

        if (0 == strcmp("list", tkn)) {
            // トライアングルリスト。
            mFaceProp |= PP_VTX_IDX;
            return S_OK;
        }
    }

    return S_OK;
}

int
PlyReader::ReadVertex(void) {
    int rv = 0;
    char s[256];

    for (int i = 0; i < mNumVtx; ++i) {
        char* r = FGetS(s, sizeof s, mFp);
        if (r == nullptr) {
            printf("E: PlyReader::ReadVertex() failed.\n");
            return E_FAIL;
        }
        XyzUv xyzUv;
        float nx, ny, nz;
        switch (mVtxPropType) {
        case VPT_XYZ_NXNYNZ_ST:
            rv = sscanf_s(s, "%f %f %f %f %f %f %f %f",
                &xyzUv.xyz.x, &xyzUv.xyz.y, &xyzUv.xyz.z, &nx, &ny, &nz, &xyzUv.uv.x, &xyzUv.uv.y);
            if (rv != 8) {
                printf("E: PlyReader::ReadVertex() vertex item read failed 1 %d.\n", rv);
                return E_FAIL;
            }
            break;
        case VPT_XYZ_ST:
            rv = sscanf_s(s, "%f %f %f %f %f",
                &xyzUv.xyz.x, &xyzUv.xyz.y, &xyzUv.xyz.z, &xyzUv.uv.x, &xyzUv.uv.y);
            if (rv != 5) {
                printf("E: PlyReader::ReadVertex() vertex item read failed 2 %d.\n", rv);
                return E_FAIL;
            }
            break;
        default:
            assert(0);
            break;
        }

        mTM->vertexList.push_back(xyzUv);
    }

    mState = ST_ReadFace;
    return S_OK;
}

int
PlyReader::ReadFace(void)
{
    int rv = 0;
    char s[256];

    if (mFaceProp == 0) {
        // 面が無い場合。
        // OK.
        mState = ST_Finish;
        return S_OK;
    }

    if (mFaceProp != PP_VTX_IDX) {
        printf("E: PlyReader::ReadFace() element face property error\n");
        return E_FAIL;
    }

    for (int i = 0; i < mNumFace; ++i) {
        char* r = FGetS(s, sizeof s, mFp);
        if (r == nullptr) {
            printf("E: PlyReader::ReadFace() failed.\n");
            return E_FAIL;
        }
        int nIdx;
        int idx0;
        int idx1;
        int idx2;
        rv = sscanf_s(s, "%d %d %d %d", &nIdx, &idx0, &idx1, &idx2);
        if (rv != 4) {
            printf("E: PlyReader::ReadFace() face item read failed %d.\n", rv);
            return E_FAIL;
        }

        if (nIdx != 3) {
            printf("E: PlyReader::ReadFace() only triangle index supported error. %d.\n", nIdx);
            return E_FAIL;
        }

        mTM->triangleIdxList.push_back(idx0);
        mTM->triangleIdxList.push_back(idx1);
        mTM->triangleIdxList.push_back(idx2);
    }

    mState = ST_Finish;
    return S_OK;
}
