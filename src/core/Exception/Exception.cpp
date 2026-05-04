#include "Exception/Exception.hpp"

namespace t3
{

    AppException::AppException(ExceptionType type, const std::string &message)
        : std::runtime_error(message), type_(type)
    {
    }

    ExceptionType AppException::type() const
    {
        return type_;
    }

    ParseException::ParseException(const std::string &message)
        : AppException(ExceptionType::Parse, message)
    {
    }

    ValidationException::ValidationException(const std::string &message)
        : AppException(ExceptionType::Validation, message)
    {
    }

    IOException::IOException(const std::string &message)
        : AppException(ExceptionType::IO, message)
    {
    }

} // namespace t3
