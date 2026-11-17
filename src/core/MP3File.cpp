#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_STDIO

#include "../../include/core/MP3File.hpp"
#include "../../include/core/AudioFileFactory.hpp"
#include "../../include/exceptions/AudioExceptions.hpp"
#include "../../include/frame/Layer3Frame.hpp"
#include "../../include/frame/PCMFrame.hpp"
#include "../../include/core/PCMFile.hpp"
#include "../../include/strategy/FrequencyFilter.hpp"
#include "../../include/3rdparty/minimp3_ex.h"

#include <fstream>
#include <algorithm>

MP3File::MP3File(const std::string& path) : MPEGFile(path) {}

MP3File::MP3File(std::istream& encoded) : MPEGFile("") {
    // O encoder entrega um fluxo de frames MPEG em memória, sem tag ID3v2.
    while (encoded.peek() != std::char_traits<char>::eof()) {
        auto frame = std::make_unique<Layer3Frame>();
        frame->parse(encoded);
        frames.push_back(std::move(frame));
    }
    if (frames.empty()) throw InvalidHeaderException("Encoder não retornou frames MP3.");
}

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
    if (getAudioFormat(outputPath) == AudioFormat::Wav) {
        toPCM()->save(outputPath);
        return;
    }

    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile.is_open()) {
        throw FileOpenException(outputPath);
    }

    id3Tag.write(outFile);

    if (framesEdited && !frames.empty()) {
        auto* firstLayer3 = dynamic_cast<Layer3Frame*>(frames[0].get());
        if (firstLayer3 && firstLayer3->hasXingHeader()) {
            uint32_t totalBytes = 0;
            for (const auto& frame : frames) {
                totalBytes += frame->getSize();
            }
            // O frame Xing guarda metadados e fica fora da contagem de frames de áudio.
            firstLayer3->updateXingHeader(static_cast<uint32_t>(frames.size() - 1), totalBytes);
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
    framesEdited = framesEdited || extracted.size() != frames.size();
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
    framesEdited = true;

    if (frames.empty()) {
        throw InvalidHeaderException("Arquivo fica vazio após corte.");
    }
}

// Une um novo arquivo no fim do arquivo atual já aberto
void MP3File::merge(const std::string& path) {
    auto other = createAudioFile(path);
    other->parse();

    if (auto* mp3 = dynamic_cast<MP3File*>(other.get())) {
        if (frames.empty()) {
            frames = std::move(mp3->frames);
            framesEdited = false;
            return;
        }

        const auto* first = dynamic_cast<const Layer3Frame*>(frames.front().get());
        const auto* second = dynamic_cast<const Layer3Frame*>(mp3->frames.front().get());
        if (first && second && first->getSampleRate() == second->getSampleRate() && first->getChannels() == second->getChannels()) {
            // MP3s compatíveis podem ser unidos movendo os frames, como na edição direta.
            frames.reserve(frames.size() + mp3->frames.size());
            for (auto& frame : mp3->frames) {
                const auto* layer3 = dynamic_cast<const Layer3Frame*>(frame.get());
                if (layer3 && layer3->hasXingHeader()) continue;
                frames.push_back(std::move(frame));
                framesEdited = true;
            }
            return;
        }
    }

    auto pcmFile = frames.empty() ? other->toPCM() : toPCM();
    if (!frames.empty()) {
        if (const auto* pcm = dynamic_cast<const PCMFile*>(other.get())) {
            pcmFile->merge(*pcm);
        } else {
            pcmFile->merge(*other->toPCM());
        }
    }
    frames = std::move(pcmFile->toMP3()->frames);
    framesEdited = false;
}

void MP3File::addFrame(std::unique_ptr<AudioFrame> frame) {
    frames.push_back(std::move(frame));
    framesEdited = true;
}

// Implementação dessa biblioteca com auxílio da IA
// Usa o decoder do minimp3 para descontar o atraso e o padding indicados na tag Xing/Info.
std::unique_ptr<PCMFile> MP3File::toPCM() const {
    std::vector<uint8_t> encoded;
    for (const auto& frame : frames) {
        const auto* layer3 = dynamic_cast<const Layer3Frame*>(frame.get());
        if (!layer3) continue;
        // Depois de um corte ou merge direto, a contagem da tag pode estar desatualizada.
        if (framesEdited && layer3->hasXingHeader()) continue;

        const auto& data = layer3->getData();
        encoded.insert(encoded.end(), data.begin(), data.end());
    }

    mp3dec_ex_t decoder{};
    const std::unique_ptr<mp3dec_ex_t, decltype(&mp3dec_ex_close)> decoderGuard(&decoder, mp3dec_ex_close);
    if (encoded.empty() || mp3dec_ex_open_buf(&decoder, encoded.data(), encoded.size(), MP3D_DO_NOT_SCAN) != 0) {
        throw AudioException("Não foi possível iniciar a decodificação do MP3.");
    }

    std::unique_ptr<PCMFile> pcmFile;
    mp3dec_frame_info_t info{};
    int16_t* samples = nullptr;
    while (const size_t count = mp3dec_ex_read_frame(&decoder, &samples, &info, MINIMP3_MAX_SAMPLES_PER_FRAME)) {
        if (!pcmFile) pcmFile = std::make_unique<PCMFile>(info.hz, info.channels);
        std::vector<int16_t> pcmSamples(samples, samples + count);
        pcmFile->addFrame(std::make_unique<PCMFrame>(std::move(pcmSamples), info.hz, info.channels));
    }

    if (decoder.last_error) throw AudioException("Erro ao decodificar os frames do MP3.");
    if (!pcmFile) throw AudioException("Nenhuma amostra pôde ser decodificada do MP3.");
    return pcmFile;
}

void MP3File::applyBassBoost(float gain) {
    applyFrequencyEffect(FrequencyFilter(FrequencyFilter::Mode::BassBoost, FrequencyFilter::bassCutoffHz, gain));
}

void MP3File::applyFrequencyEffect(const FrequencyEffect& effect) {
    auto pcmFile = toPCM();
    pcmFile->applyFrequencyEffect(effect);
    frames = std::move(pcmFile->toMP3()->frames);
    framesEdited = false;
}
