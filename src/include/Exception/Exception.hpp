#pragma once

#include <stdexcept>
#include <string>

namespace t3
{

    enum class ExceptionType
    {
        Parse,
        Validation,
        IO
    };

    class AppException : public std::runtime_error
    {
    public:
        AppException(ExceptionType type, const std::string &message);

        ExceptionType type() const;

    private:
        ExceptionType type_;
    };

    class ParseException : public AppException
    {
    public:
        explicit ParseException(const std::string &message);
    };

    class ValidationException : public AppException
    {
    public:
        explicit ValidationException(const std::string &message);
    };

    class IOException : public AppException
    {
    public:
        explicit IOException(const std::string &message);
    };

} // namespace t3
