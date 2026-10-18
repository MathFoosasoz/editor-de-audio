#pragma once
#include <memory>
#include <string>
#include <stdexcept>
#include <algorithm>
#include "../core/AudioFile.hpp"
#include "../core/MP3File.hpp"
#include "../core/PCMFile.hpp"

// Factory (sem classe) para instanciar um tipo específico de AudioFile dependendo do tipo do arquivo
// Vale notar que o método createAudioFile propaga as exceções dos construtores dos outros arquivos.
inline bool endsWith(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) return false;
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

inline std::unique_ptr<AudioFile> createAudioFile(const std::string& path) {
    std::string lowerPath = path;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
    
    if (endsWith(lowerPath, ".wav")) {
        return std::make_unique<PCMFile>(path);
    } else if (endsWith(lowerPath, ".mp3")) {
        return std::make_unique<MP3File>(path);
    }
    throw std::invalid_argument("Unsupported audio file format (supported: .mp3, .wav).");
}