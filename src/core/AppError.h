#pragma once

#include <stdexcept>
#include <string>

namespace vss {

enum class ExitCode {
    Success = 0,
    ArgumentError = 1,
    InputError = 2,
    ProcessingError = 3
};

enum class ErrorCategory {
    Argument,
    Input,
    Processing
};

class AppError : public std::runtime_error {
public:
    AppError(ErrorCategory category, const std::string& message);

    ErrorCategory category() const noexcept;
    ExitCode exitCode() const noexcept;

private:
    ErrorCategory category_;
};

}
