// 日本語。
#pragma once

#include <memory>
#include <string>

namespace sample {
    struct IOpenXrProgram {
        virtual ~IOpenXrProgram() = default;
        virtual int Run() = 0;
    };

    std::unique_ptr<IOpenXrProgram> CreateOpenXrProgram(
            std::string applicationName);

}; // namespace sample
