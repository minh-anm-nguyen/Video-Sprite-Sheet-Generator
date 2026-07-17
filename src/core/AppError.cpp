#include "core/AppError.h"

namespace vss {

AppError::AppError(ErrorCategory category, const std::string& message)
    : std::runtime_error(message), category_(category) {}

ErrorCategory AppError::category() const noexcept {
    return category_;
}

ExitCode AppError::exitCode() const noexcept {
    switch (category_) {
    case ErrorCategory::Argument:
        return ExitCode::ArgumentError;
    case ErrorCategory::Input:
        return ExitCode::InputError;
    default:
        return ExitCode::ProcessingError;
    }
}

}
