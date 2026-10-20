#define MINIMP3_IMPLEMENTATION

#include "../../include/core/MP3File.hpp"
#include "../../include/exceptions/AudioExceptions.hpp"
#include "../../include/frame/Layer3Frame.hpp"
#include "../../include/frame/PCMFrame.hpp"
#include "../../include/core/PCMFile.hpp"
#include "../../include/frame/PCMFrame.hpp"
#include "../../include/3rdparty/minimp3.h"

#include <fstream>
#include <algorithm>

MP3File::MP3File(const std::string& path) : MPEGFile(path) {}

void MP3File::parse() {
    if (!fileStream.is_open()) {
        open();
    }

    // tamanho do arquivo
    fileStream.seekg(0, std::ios::end);
    std::streamsize fileSize = fileStream.tellg();
    fileStream.seekg(0, std::ios::beg);

    id3Tag.parse(fileStream);

    // loop que extraí todos os frames
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

// Une um novo arquivo no fim do arquivo atual já aberto
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

void MP3File::addFrame(std::unique_ptr<AudioFrame> frame) {
    frames.push_back(std::move(frame));
}

// Implementação dessa biblioteca com auxílio da IA
// Usa a biblioteca minimp3.h header only para o decoder MP3 -> PCM
std::unique_ptr<PCMFile> MP3File::toPCM() const {
    auto pcmFile = std::make_unique<PCMFile>(44100, 2);

    mp3dec_t mp3d;
    mp3dec_init(&mp3d);

    for (const auto& frame : frames) {
        const auto* layer3 = dynamic_cast<const Layer3Frame*>(frame.get());
        if (!layer3) continue;

        const auto& data = layer3->getData();
        
        mp3dec_frame_info_t info;
        std::vector<int16_t> pcmSamples(MINIMP3_MAX_SAMPLES_PER_FRAME);

        int samplesDecoded = mp3dec_decode_frame(&mp3d, data.data(), static_cast<int>(data.size()), pcmSamples.data(), &info);

        if (samplesDecoded > 0) {
            pcmSamples.resize(samplesDecoded * info.channels);
            pcmFile->addFrame(std::make_unique<PCMFrame>(std::move(pcmSamples), info.hz, info.channels));
        }
    }

    return pcmFile;
}

void MP3File::applyBassBoost(float gain) {
    
    auto pcmFile = toPCM();
    
    pcmFile->applyBassBoost(gain);
    
    auto newMp3File = pcmFile->toMP3();
    
    this->frames = std::move(newMp3File->frames);
}
