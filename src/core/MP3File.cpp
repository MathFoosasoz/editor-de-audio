#include "../../include/core/MP3File.hpp"
#include "../../include/exceptions/AudioExceptions.hpp"
#include "../../include/frame/Layer3Frame.hpp"
#include "../../include/frame/PCMFrame.hpp"
#include "../../include/core/PCMFile.hpp"
#include "../../include/frame/PCMFrame.hpp"

#include <fstream>
#include <algorithm>

MP3File::MP3File(const std::string& path) : MPEGFile(path) {}

void MP3File::parse() {
    if (!fileStream.is_open()) {
        open();
    }

    fileStream.seekg(0, std::ios::end);
    std::streamsize fileSize = fileStream.tellg();
    fileStream.seekg(0, std::ios::beg);

    id3Tag.parse(fileStream);

    while (fileStream.tellg() < fileSize) {
        if (fileSize - fileStream.tellg() < 4) {
            break;
        }
        auto frame = std::make_unique<Layer3Frame>();
        try {
            frame->parse(fileStream);
            frames.push_back(std::move(frame));
        } catch (const AudioException&) {
            break;
        }
    }

    if (frames.empty()) {
        throw InvalidHeaderException("Nenhum frame valido MPEG encontrado no arquivo");
    }
}

void MP3File::save(const std::string& outputPath) const {
    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile.is_open()) {
        throw FileOpenException(outputPath);
    }

    id3Tag.write(outFile);

    uint32_t totalBytes = 0;
    for (const auto& frame : frames) {
        const auto* layer3 = dynamic_cast<const Layer3Frame*>(frame.get());
        if (layer3) {
            totalBytes += layer3->getData().size();
        }
    }

    if (!frames.empty()) {
        auto* firstLayer3 = dynamic_cast<Layer3Frame*>(frames[0].get());
        if (firstLayer3 && firstLayer3->hasXingHeader()) {
            firstLayer3->updateXingHeader(static_cast<uint32_t>(frames.size()), totalBytes);
        }
    }

    for (const auto& frame : frames) {
        const auto* layer3 = dynamic_cast<const Layer3Frame*>(frame.get());
        if (layer3) {
            layer3->write(outFile);
        }
    }
}

size_t MP3File::getFrameCount() const {
    return frames.size();
}

void MP3File::applyRange(const RangeStrategy& strategy) {
    auto [start, end] = strategy.calculateRange(frames);
    
    if (start >= frames.size() || start >= end) {
        throw InvalidHeaderException("Arquivo fica vazio após corte em intervalo (O intervalo pode ser invalido).");
    }

    auto startIt = frames.begin() + start;
    auto endIt = frames.begin() + end;

    std::vector<std::unique_ptr<AudioFrame>> extracted(std::make_move_iterator(startIt), std::make_move_iterator(endIt));
    frames = std::move(extracted);
}

void MP3File::applyCut(const RangeStrategy& strategy) {
    auto [removeStart, removeEnd] = strategy.calculateRange(frames);

    if (removeStart >= removeEnd || removeStart >= frames.size()) {
        return; 
    }

    std::vector<std::unique_ptr<AudioFrame>> newFrames;
    newFrames.reserve(frames.size() - (removeEnd - removeStart));

    for (size_t i = 0; i < removeStart; ++i) {
        newFrames.push_back(std::move(frames[i]));
    }
    for (size_t i = removeEnd; i < frames.size(); ++i) {
        newFrames.push_back(std::move(frames[i]));
    }

    frames = std::move(newFrames);

    if (frames.empty()) {
        throw InvalidHeaderException("Arquivo fica vazio após corte.");
    }
}

void MP3File::merge(const std::string& path) {
    MP3File other(path);
    other.parse();
    
    for (auto& frame : other.frames) {
        const auto* layer3 = dynamic_cast<const Layer3Frame*>(frame.get());
        if (layer3 && layer3->hasXingHeader()) {
            continue; 
        }
        frames.push_back(std::move(frame));
    }
    other.frames.clear();
}

std::unique_ptr<PCMFile> MP3File::toPCM() const {

    auto pcmFile = std::make_unique<PCMFile>(44100, 2);

    for (const auto& frame : frames) {
        const auto* layer3 = dynamic_cast<const Layer3Frame*>(frame.get());
        if (!layer3) continue;

        const auto& compressedData = layer3->getData();

        // PLACEHOLDER DE MP3 -> PCM
        // A gente precisa de uma biblioteca externa para fazer isso
        // TODO: Adicionar dependências do FFMPEG
        
        std::vector<int16_t> decodedSamples; 
        
        // PLACEHOLDER

        pcmFile->addFrame(std::make_unique<PCMFrame>(std::move(decodedSamples), 44100, 2));
    }

    return pcmFile;
}