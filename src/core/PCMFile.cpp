#include "../../include/core/PCMFile.hpp"
#include "../../include/core/AudioFileFactory.hpp"
#include "../../include/exceptions/AudioExceptions.hpp"
#include "../../include/core/MP3File.hpp"
#include "../../include/dsp/FrequencyProcessor.hpp"
#include "../../include/strategy/FrequencyFilter.hpp"
#include "../3rdparty/lame/lame.h"

#include <fstream>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <sstream>

static uint16_t readUint16(const char* bytes) {
    return static_cast<uint8_t>(bytes[0]) | (static_cast<uint8_t>(bytes[1]) << 8);
}

static uint32_t readUint32(const char* bytes) {
    return readUint16(bytes) | (static_cast<uint32_t>(readUint16(bytes + 2)) << 16);
}

PCMFile::PCMFile(const std::string& path) : AudioFile(path), sampleRate(44100), channels(2) {}
PCMFile::PCMFile(int sr, int ch) : AudioFile(""), sampleRate(sr), channels(ch) {}

void PCMFile::parse() {
    if (!fileStream.is_open()) {
        open();
    }

    if (!fileStream.is_open()) {
        throw FileOpenException(filePath);
    }

    char header[12];
    if (!fileStream.read(header, sizeof(header))) {
        throw InvalidHeaderException("Header de WAV inválido ou incompleto.");
    }

    if (std::string(header, 4) != "RIFF" || std::string(header + 8, 4) != "WAVE") {
        throw InvalidHeaderException("Não é um arquivo WAV válido.");
    }

    fileStream.seekg(0, std::ios::end);
    const std::streamoff fileSize = fileStream.tellg();
    fileStream.seekg(sizeof(header), std::ios::beg);

    bool hasFormat = false;
    std::streamoff dataOffset = -1;
    uint32_t dataSize = 0;
    char chunkHeader[8];

    // Chunks como LIST podem aparecer entre o formato e as amostras.
    while ((!hasFormat || dataOffset < 0) && fileStream.read(chunkHeader, sizeof(chunkHeader))) {
        const std::streamoff chunkOffset = fileStream.tellg();
        const uint32_t chunkSize = readUint32(chunkHeader + 4);
        if (chunkSize > fileSize - chunkOffset) throw UnexpectedEOFException();

        const std::string chunkId(chunkHeader, 4);
        if (chunkId == "fmt " && !hasFormat) {
            constexpr uint32_t pcmFormatSize = 16;
            constexpr uint16_t pcmFormat = 1;
            constexpr uint16_t extensibleFormat = 0xfffe;
            const std::string pcmSubtype("\x01\x00\x00\x00\x00\x00\x10\x00\x80\x00\x00\xaa\x00\x38\x9b\x71", 16);
            char format[40];
            if (chunkSize < pcmFormatSize || !fileStream.read(format, std::min<uint32_t>(chunkSize, sizeof(format)))) {
                throw InvalidHeaderException("Formato WAV inválido ou incompleto.");
            }

            const uint16_t encoding = readUint16(format);
            const bool isExtensiblePCM = encoding == extensibleFormat && chunkSize >= sizeof(format) &&
                std::string(format + 24, 16) == pcmSubtype;
            if ((encoding != pcmFormat && !isExtensiblePCM) || readUint16(format + 14) != 16) {
                throw InvalidHeaderException("Somente arquivos PCM de 16 bits são suportados.");
            }

            channels = readUint16(format + 2);
            const uint32_t rate = readUint32(format + 4);
            if (channels == 0 || rate == 0 || rate > std::numeric_limits<int>::max()) {
                throw InvalidHeaderException("Taxa de amostragem ou número de canais inválido.");
            }
            sampleRate = static_cast<int>(rate);
            hasFormat = true;
        } else if (chunkId == "data" && dataOffset < 0) {
            dataOffset = chunkOffset;
            dataSize = chunkSize;
        }

        fileStream.seekg(chunkOffset + chunkSize + (chunkSize % 2));
    }

    if (!hasFormat || dataOffset < 0 || dataSize % (channels * sizeof(int16_t)) != 0) {
        throw InvalidHeaderException("WAV sem formato ou dados PCM válidos.");
    }

    fileStream.seekg(dataOffset);
    std::vector<int16_t> rawSamples(dataSize / sizeof(int16_t));
    if (!fileStream.read(reinterpret_cast<char*>(rawSamples.data()), dataSize)) throw UnexpectedEOFException();
    addSamples(rawSamples);
}

void PCMFile::addSamples(const std::vector<int16_t>& samples) {
    const size_t samplesPerFrame = 1024 * channels;
    for (size_t i = 0; i < samples.size(); i += samplesPerFrame) {
        const size_t count = std::min(samplesPerFrame, samples.size() - i);
        std::vector<int16_t> frameSamples(samples.begin() + i, samples.begin() + i + count);
        frames.push_back(std::make_unique<PCMFrame>(std::move(frameSamples), sampleRate, channels));
    }
}

// Uso ruim de magic numbers do arquivo WAV

void PCMFile::save(const std::string& outputPath) const {
    if (getAudioFormat(outputPath) == AudioFormat::Mp3) {
        toMP3()->save(outputPath);
        return;
    }

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

void PCMFile::addFrame(std::unique_ptr<AudioFrame> frame) {
    frames.push_back(std::move(frame));
}
void PCMFile::applyRange(const RangeStrategy& strategy) {
    auto [start, end] = strategy.calculateRange(frames);
    
    if (start >= frames.size() || start >= end) {
        throw InvalidHeaderException("Operação resulta em um arquivo PCM inválido (o seu intervalo pode ser inválido).");
    }

    auto startIt = frames.begin() + start;
    auto endIt = frames.begin() + end;

    std::vector<std::unique_ptr<AudioFrame>> extracted(std::make_move_iterator(startIt), std::make_move_iterator(endIt));
    frames = std::move(extracted);
}

void PCMFile::applyCut(const RangeStrategy& strategy) {
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
        throw InvalidHeaderException("Corte resulta em um arquivo PCM vazio.");
    }
}

void PCMFile::merge(const std::string& path) {
    auto other = createAudioFile(path);
    other->parse();

    if (const auto* pcm = dynamic_cast<const PCMFile*>(other.get())) {
        merge(*pcm);
    } else {
        merge(*other->toPCM());
    }
}

void PCMFile::merge(const PCMFile& other) {
    if (sampleRate != other.sampleRate || channels != other.channels) {
        addSamples(other.convertSamples(sampleRate, channels));
        return;
    }

    const size_t frameCount = other.frames.size();
    frames.reserve(frames.size() + frameCount);
    for (size_t i = 0; i < frameCount; ++i) {
        frames.push_back(std::make_unique<PCMFrame>(other.frames[i]->getSamples(), sampleRate, channels));
    }
}

std::unique_ptr<PCMFile> PCMFile::toPCM() const {
    auto copy = std::make_unique<PCMFile>(sampleRate, channels);
    copy->merge(*this);
    return copy;
}

std::vector<int16_t> PCMFile::convertSamples(int targetSampleRate, int targetChannels) const {
    std::vector<int16_t> samples;
    for (const auto& frame : frames) {
        const auto& frameSamples = frame->getSamples();
        if (targetChannels == channels) {
            samples.insert(samples.end(), frameSamples.begin(), frameSamples.end());
            continue;
        }

        // A saída mantém os canais em ordem e mistura os canais excedentes.
        for (size_t i = 0; i < frameSamples.size(); i += channels) {
            const auto first = frameSamples.begin() + i;
            if (targetChannels == 1) {
                const int sum = std::accumulate(first, first + channels, 0);
                samples.push_back(static_cast<int16_t>(sum / channels));
            } else if (channels == 1) {
                samples.insert(samples.end(), targetChannels, *first);
            } else if (targetChannels < channels) {
                const int sum = std::accumulate(first + targetChannels, first + channels, 0);
                for (int channel = 0; channel < targetChannels; ++channel) {
                    samples.push_back(static_cast<int16_t>((first[channel] + sum) / (channels - targetChannels + 1)));
                }
            } else {
                samples.insert(samples.end(), first, first + channels);
                samples.insert(samples.end(), targetChannels - channels, 0);
            }
        }
    }

    if (samples.empty() || sampleRate == targetSampleRate) return samples;

    // Interpola no áudio inteiro para manter a duração entre os frames.
    const size_t samplesPerChannel = samples.size() / targetChannels;
    const double step = static_cast<double>(sampleRate) / targetSampleRate;
    const size_t targetCount = std::max(1LL, std::llround(samplesPerChannel / step));
    std::vector<int16_t> resampled(targetCount * targetChannels);
    for (size_t i = 0; i < targetCount; ++i) {
        const double position = std::min(i * step, static_cast<double>(samplesPerChannel - 1));
        const size_t left = static_cast<size_t>(position);
        const size_t right = std::min(left + 1, samplesPerChannel - 1);
        const double fraction = position - left;
        for (int channel = 0; channel < targetChannels; ++channel) {
            const int16_t start = samples[left * targetChannels + channel];
            const int16_t end = samples[right * targetChannels + channel];
            resampled[i * targetChannels + channel] = static_cast<int16_t>(std::lround(start + (end - start) * fraction));
        }
    }

    return resampled;
}

void PCMFile::applyBassBoost(float gainFactor) {
    applyFrequencyEffect(FrequencyFilter(FrequencyFilter::Mode::BassBoost, FrequencyFilter::bassCutoffHz, gainFactor));
}

void PCMFile::applyFrequencyEffect(const FrequencyEffect& effect) {
    const FrequencyProcessor processor(sampleRate, effect);
    auto samples = convertSamples(sampleRate, channels);
    processor.apply(samples, channels);

    // A janela da FFT atravessa frames. A saída mantém a divisão original em frames.
    auto nextSample = samples.cbegin();
    for (auto& frame : frames) {
        auto& frameSamples = frame->getSamples();
        std::copy_n(nextSample, frameSamples.size(), frameSamples.begin());
        nextSample += frameSamples.size();
    }
}
    
// Integração dessa biblíoteca com auxílio da IA
// Usa a biblioteca LAME para codificar PCM -> MP3
// Essa conversão é não-trivial e seria muito fora do escopo do trabalho
std::unique_ptr<MP3File> PCMFile::toMP3() const {
    const int mp3Channels = std::min(channels, 2);
    auto allSamples = convertSamples(sampleRate, mp3Channels);
    if (allSamples.empty()) throw AudioException("Arquivo sem amostras para codificar em MP3.");

    std::unique_ptr<lame_global_flags, decltype(&lame_close)> encoder(lame_init(), lame_close);
    if (!encoder || lame_set_in_samplerate(encoder.get(), sampleRate) < 0 ||
        lame_set_num_channels(encoder.get(), mp3Channels) < 0 ||
        lame_set_VBR(encoder.get(), vbr_default) < 0 || lame_init_params(encoder.get()) < 0) {
        throw AudioException("Não foi possível inicializar o encoder MP3.");
    }

    const size_t samplesPerChannel = allSamples.size() / mp3Channels;
    // Limite de buffer recomendado pelo LAME para uma chamada de codificação.
    const size_t bufferSize = 7200 + samplesPerChannel + (samplesPerChannel + 3) / 4;
    if (bufferSize > std::numeric_limits<int>::max()) {
        throw AudioException("Áudio grande demais para codificar em MP3.");
    }
    const int maxMp3Size = static_cast<int>(bufferSize);
    const int numSamples = static_cast<int>(samplesPerChannel);
    std::vector<uint8_t> mp3Buffer(maxMp3Size);

    const int bytesEncoded = mp3Channels == 1
        ? lame_encode_buffer(encoder.get(), allSamples.data(), allSamples.data(), numSamples, mp3Buffer.data(), maxMp3Size)
        : lame_encode_buffer_interleaved(encoder.get(), allSamples.data(), numSamples, mp3Buffer.data(), maxMp3Size);
    if (bytesEncoded < 0) throw AudioException("Erro ao codificar amostras em MP3.");

    const int flushBytes = lame_encode_flush(encoder.get(), mp3Buffer.data() + bytesEncoded, maxMp3Size - bytesEncoded);
    if (flushBytes < 0) throw AudioException("Erro ao finalizar o áudio MP3.");

    const size_t totalBytes = bytesEncoded + flushBytes;
    // O LAME reserva o primeiro frame para a tag até a codificação terminar.
    if (lame_get_lametag_frame(encoder.get(), mp3Buffer.data(), totalBytes) > totalBytes) {
        throw AudioException("Erro ao gravar a tag do encoder MP3.");
    }

    std::istringstream encoded(std::string(reinterpret_cast<const char*>(mp3Buffer.data()), totalBytes), std::ios::binary);
    return std::make_unique<MP3File>(encoded);
}
