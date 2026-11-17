#pragma once
#include <memory>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include "../core/AudioFile.hpp"
#include "../core/MP3File.hpp"
#include "../core/PCMFile.hpp"

enum class AudioFormat { Wav, Mp3 };

inline bool endsWith(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) return false;
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

// A entrada e a saída usam a mesma regra para identificar a extensão.
inline AudioFormat getAudioFormat(const std::string& path) {
    std::string lowerPath = path;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    if (endsWith(lowerPath, ".wav")) return AudioFormat::Wav;
    if (endsWith(lowerPath, ".mp3")) return AudioFormat::Mp3;
    throw std::invalid_argument("Formato de áudio não suportado (suportados: .mp3, .wav).");
}

// Factory (sem classe) para instanciar um tipo específico de AudioFile pela extensão.
inline std::unique_ptr<AudioFile> createAudioFile(const std::string& path) {
    if (getAudioFormat(path) == AudioFormat::Wav) {
        return std::make_unique<PCMFile>(path);
    }
    return std::make_unique<MP3File>(path);
}
