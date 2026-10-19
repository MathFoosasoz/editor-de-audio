#include "../../include/core/PCMFile.hpp"
#include "../../include/exceptions/AudioExceptions.hpp"
#include "../../include/core/MP3File.hpp"
#include "../../include/frame/Layer3Frame.hpp"
#include "../../include/core/MP3File.hpp"
#include <fstream>
#include <algorithm>

PCMFile::PCMFile(const std::string& path) : AudioFile(path), sampleRate(44100), channels(2) {}
PCMFile::PCMFile(int sr, int ch) : AudioFile(""), sampleRate(sr), channels(ch) {}

void PCMFile::parse() {
    if (!fileStream.is_open()) {
        open();
    }

    char header[44];
    fileStream.read(header, 44);
    if (!fileStream || fileStream.gcount() < 44) {
        throw InvalidHeaderException("Header de WAV inválido ou incompleto.");
    }

    if (std::string(header, 4) != "RIFF" || std::string(header + 8, 4) != "WAVE") {
        throw InvalidHeaderException("Não é um arquivo WAV válido.");
    }

    channels = *reinterpret_cast<uint16_t*>(header + 22);
    sampleRate = *reinterpret_cast<uint32_t*>(header + 24);
    uint16_t bitsPerSample = *reinterpret_cast<uint16_t*>(header + 34);

    if (bitsPerSample != 16) {
        throw InvalidHeaderException("Somente arquivos PCM de 16 bits são suportados.");
    }

    fileStream.seekg(0, std::ios::end);
    std::streamsize fileSize = fileStream.tellg();
    std::streamsize dataSize = fileSize - 44;
    
    fileStream.seekg(44, std::ios::beg);
    std::vector<int16_t> rawSamples(dataSize / sizeof(int16_t));
    fileStream.read(reinterpret_cast<char*>(rawSamples.data()), dataSize);

    size_t samplesPerFrame = 1024 * channels;
    for (size_t i = 0; i < rawSamples.size(); i += samplesPerFrame) {
        size_t count = std::min(samplesPerFrame, rawSamples.size() - i);
        std::vector<int16_t> frameSamples(rawSamples.begin() + i, rawSamples.begin() + i + count);
        frames.push_back(std::make_unique<PCMFrame>(std::move(frameSamples), sampleRate, channels));
    }
}

void PCMFile::save(const std::string& outputPath) const {
    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile.is_open()) {
        throw FileOpenException(outputPath);
    }

    uint32_t totalDataBytes = 0;
    for (const auto& frame : frames) {
        totalDataBytes += frame->getSamples().size() * sizeof(int16_t);
    }

    outFile.write("RIFF", 4);
    uint32_t chunkSize = 36 + totalDataBytes;
    outFile.write(reinterpret_cast<const char*>(&chunkSize), 4);
    outFile.write("WAVEfmt ", 8);
    uint32_t subchunk1Size = 16;
    outFile.write(reinterpret_cast<const char*>(&subchunk1Size), 4);
    uint16_t audioFormat = 1;
    outFile.write(reinterpret_cast<const char*>(&audioFormat), 2);
    outFile.write(reinterpret_cast<const char*>(&channels), 2);
    outFile.write(reinterpret_cast<const char*>(&sampleRate), 4);
    uint32_t byteRate = sampleRate * channels * 2;
    outFile.write(reinterpret_cast<const char*>(&byteRate), 4);
    uint16_t blockAlign = channels * 2;
    outFile.write(reinterpret_cast<const char*>(&blockAlign), 2);
    uint16_t bitsPerSample = 16;
    outFile.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
    outFile.write("data", 4);
    outFile.write(reinterpret_cast<const char*>(&totalDataBytes), 4);

    for (const auto& frame : frames) {
        const auto& samples = frame->getSamples();
        outFile.write(reinterpret_cast<const char*>(samples.data()), samples.size() * sizeof(int16_t));
    }
}

size_t PCMFile::getFrameCount() const {
    return frames.size();
}

const std::vector<std::unique_ptr<PCMFrame>>& PCMFile::getFrames() const {
    return frames;
}

void PCMFile::addFrame(std::unique_ptr<PCMFrame> frame) {
    frames.push_back(std::move(frame));
}

void PCMFile::applyRange(const RangeStrategy& strategy) {
    auto [start, end] = strategy.calculateRange(reinterpret_cast<const std::vector<std::unique_ptr<AudioFrame>>&>(frames));
    
    if (start >= frames.size() || start >= end) {
        throw InvalidHeaderException("Operação resulta em um arquivo PCM inválido (o seu intervalo pode ser inválido).");
    }

    auto startIt = frames.begin() + start;
    auto endIt = frames.begin() + end;

    std::vector<std::unique_ptr<PCMFrame>> extracted(std::make_move_iterator(startIt), std::make_move_iterator(endIt));
    frames = std::move(extracted);
}

void PCMFile::applyCut(const RangeStrategy& strategy) {
    auto [removeStart, removeEnd] = strategy.calculateRange(reinterpret_cast<const std::vector<std::unique_ptr<AudioFrame>>&>(frames));

    if (removeStart >= removeEnd || removeStart >= frames.size()) {
        return; 
    }

    std::vector<std::unique_ptr<PCMFrame>> newFrames;
    newFrames.reserve(frames.size() - (removeEnd - removeStart));

    for (size_t i = 0; i < removeStart; ++i) {
        newFrames.push_back(std::move(frames[i]));
    }
    for (size_t i = removeEnd; i < frames.size(); ++i) {
        newFrames.push_back(std::move(frames[i]));
    }

    frames = std::move(newFrames);

    if (frames.empty()) {
        throw InvalidHeaderException("Corte resulta em um arquivo PCM vazio.");
    }
}

void PCMFile::merge(const std::string& path) {
    PCMFile other(path);
    other.parse();
    
    for (const auto& frame : other.getFrames()) {
        std::vector<int16_t> sampleCopy = frame->getSamples();
        frames.push_back(std::make_unique<PCMFrame>(sampleCopy, frame->getSampleRate(), frame->getChannels()));
    }
}

void PCMFile::applyBassBoost(float gainFactor) {
    for (auto& frame : frames) {
        for (auto& sample : frame->getSamples()) {
            int32_t boosted = static_cast<int32_t>(sample * gainFactor);
            if (boosted > 32767) boosted = 32767;
            if (boosted < -32768) boosted = -32768;
            sample = static_cast<int16_t>(boosted);
        }
    }
}

std::unique_ptr<MP3File> PCMFile::toMP3() const {
    auto mp3File = std::make_unique<MP3File>("");
    for (const auto& frame : frames) {
        const auto& pcmSamples = frame->getSamples();

        // PLACEHOLDER

        std::vector<uint8_t> encodedLayer3Data;
        
        // PLACEHOLDER
    }

    return mp3File;
}
