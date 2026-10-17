#include "../../include/cli/CommandDispatcher.hpp"
#include "../../include/core/AudioFile.hpp"
#include "../../include/core/AudioFileFactory.hpp"
#include "../../include/strategy/FrameRangeStrategy.hpp"
#include "../../include/strategy/SecondRangeStrategy.hpp"
#include "../../include/strategy/CutStrategy.hpp"
#include <iostream>
#include <stdexcept>
#include <memory>

CommandDispatcher::CommandDispatcher() {
    commands["parse"] = [](int c, char* v[]) {
        if (c < 3) throw std::invalid_argument("Usage: parse <input>");
        auto audio = createAudioFile(v[2]);
        audio->parse();
        std::cout << "Parsed frames: " << audio->getFrameCount() << "\n";
    };

    commands["trim-start"] = [](int c, char* v[]) {
        if (c < 5) throw std::invalid_argument("Usage: trim-start <input> <output> <value> [frames|seconds]");
        auto audio = createAudioFile(v[2]);
        audio->parse();
        
        std::string unit = (c > 5) ? v[5] : "frames";
        double val = std::stod(v[4]);

        std::unique_ptr<RangeStrategy> strategy;
        if (unit == "seconds") {
            strategy = std::make_unique<SecondRangeStrategy>(SecondRangeStrategy::Mode::TrimStart, val);
        } else {
            strategy = std::make_unique<FrameRangeStrategy>(FrameRangeStrategy::Mode::TrimStart, static_cast<size_t>(val));
        }

        audio->applyRange(*strategy);
        audio->save(v[3]);
        std::cout << "Trimmed start and saved to " << v[3] << "\n";
    };

    commands["trim-end"] = [](int c, char* v[]) {
        if (c < 5) throw std::invalid_argument("Usage: trim-end <input> <output> <value> [frames|seconds]");
        auto audio = createAudioFile(v[2]);
        audio->parse();
        
        std::string unit = (c > 5) ? v[5] : "frames";
        double val = std::stod(v[4]);

        std::unique_ptr<RangeStrategy> strategy;
        if (unit == "seconds") {
            strategy = std::make_unique<SecondRangeStrategy>(SecondRangeStrategy::Mode::TrimEnd, val);
        } else {
            strategy = std::make_unique<FrameRangeStrategy>(FrameRangeStrategy::Mode::TrimEnd, static_cast<size_t>(val));
        }

        audio->applyRange(*strategy);
        audio->save(v[3]);
        std::cout << "Trimmed end and saved to " << v[3] << "\n";
    };

    commands["extract"] = [](int c, char* v[]) {
        if (c < 6) throw std::invalid_argument("Usage: extract <input> <output> <start> <duration> [frames|seconds]");
        auto audio = createAudioFile(v[2]);
        audio->parse();
        
        std::string unit = (c > 6) ? v[6] : "frames";
        double start = std::stod(v[4]);
        double duration = std::stod(v[5]);

        std::unique_ptr<RangeStrategy> strategy;
        if (unit == "seconds") {
            strategy = std::make_unique<SecondRangeStrategy>(SecondRangeStrategy::Mode::Extract, start, duration);
        } else {
            strategy = std::make_unique<FrameRangeStrategy>(FrameRangeStrategy::Mode::Extract, static_cast<size_t>(start), static_cast<size_t>(duration));
        }

        audio->applyRange(*strategy);
        audio->save(v[3]);
        std::cout << "Extracted range and saved to " << v[3] << "\n";
    };

    commands["merge"] = [](int c, char* v[]) {
        if (c < 5) throw std::invalid_argument("Usage: merge <input1> <input2> <output>");
        auto audio = createAudioFile(v[2]);
        audio->parse();
        audio->merge(v[3]);
        audio->save(v[4]);
        std::cout << "Merged files and saved to " << v[4] << "\n";
    };

    commands["cut"] = [](int c, char* v[]) {
        if (c < 6) throw std::invalid_argument("Usage: cut <input> <output> <start> <duration> [frames|seconds]");
        auto audio = createAudioFile(v[2]);
        audio->parse();
        
        std::string unit = (c > 6) ? v[6] : "frames";
        double start = std::stod(v[4]);
        double duration = std::stod(v[5]);

        CutStrategy strategy(start, duration, unit == "seconds");
        audio->applyCut(strategy);
        audio->save(v[3]);
        std::cout << "Cut range and saved to " << v[3] << "\n";
    };

    commands["bass-boost"] = [](int c, char* v[]) {
        if (c < 5) throw std::invalid_argument("Usage: bass-boost <input> <output> <gain_factor>");
        
        auto audio = createAudioFile(v[2]);
        audio->parse();
        
        PCMFile* pcmFile = dynamic_cast<PCMFile*>(audio.get());
        if (!pcmFile) {
            throw std::invalid_argument("bass-boost is currently only supported for uncompressed PCM (.wav) files.");
        }
        
        float gain = std::stof(v[4]);
        pcmFile->applyBassBoost(gain);
        
        audio->save(v[3]);
        std::cout << "Applied bass boost (gain " << gain << ") and saved to " << v[3] << "\n";
    };
}

void CommandDispatcher::execute(int argc, char* argv[]) {
    if (argc < 2) {
        throw std::invalid_argument("No command provided.");
    }

    std::string command = argv[1];
    auto it = commands.find(command);
    if (it != commands.end()) {
        it->second(argc, argv);
    } else {
        throw std::invalid_argument("Unknown command: " + command);
    }
}