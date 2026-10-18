#pragma once

#include <stdexcept>
#include <string>

// Exceções que podem ocorrer no processamento de aúdio

// Erro geral ligado ao processamento de áudio
class AudioException : public std::runtime_error {
public:
    explicit AudioException(const std::string& message) 
        : std::runtime_error(message) {}
};

// Não pode abrir o arquivo com nome (filename)
class FileOpenException : public AudioException {
public:
    explicit FileOpenException(const std::string& filename)
        : AudioException("Não pode abrir arquivo: " + filename) {}
};

// Header de um dado arquivo é inválido
class InvalidHeaderException : public AudioException {
public:
    explicit InvalidHeaderException(const std::string& message)
        : AudioException("Header inválido: " + message) {}
};

// EOF encontrado em momento inesperado
class UnexpectedEOFException : public AudioException {
public:
    explicit UnexpectedEOFException()
        : AudioException("EOF inesperado encontrado.") {}
};