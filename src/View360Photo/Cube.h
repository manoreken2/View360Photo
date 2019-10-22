// 日本語。
#pragma once
#include "pch.h" 

namespace sample {
    struct Cube {
        xr::SpaceHandle Space;
        XrPosef Pose = xr::math::Pose::Identity();
        XrVector3f Scale{0.02f, 0.02f, 0.02f};
    };
};
