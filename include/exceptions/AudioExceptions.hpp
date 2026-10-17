#pragma once

#include <stdexcept>
#include <string>

class AudioException : public std::runtime_error {
public:
    explicit AudioException(const std::string& message) 
        : std::runtime_error(message) {}
};

class FileOpenException : public AudioException {
public:
    explicit FileOpenException(const std::string& filename)
        : AudioException("Failed to open file: " + filename) {}
};

class InvalidHeaderException : public AudioException {
public:
    explicit InvalidHeaderException(const std::string& message)
        : AudioException("Invalid header: " + message) {}
};

class UnexpectedEOFException : public AudioException {
public:
    explicit UnexpectedEOFException()
        : AudioException("Unexpected end of file reached.") {}
};