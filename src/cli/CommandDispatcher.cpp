#include "../../include/cli/CommandDispatcher.hpp"
#include "../../include/core/MP3File.hpp"
#include "../../include/strategy/FrameRangeStrategy.hpp"
#include "../../include/strategy/SecondRangeStrategy.hpp"
#include "../../include/strategy/CutStrategy.hpp"
#include <iostream>
#include <stdexcept>
#include <memory>

CommandDispatcher::CommandDispatcher() {
    commands["parse"] = [](int c, char* v[]) {
        if (c < 3) throw std::invalid_argument("Usage: parse <input>");
        MP3File mp3(v[2]);
        mp3.parse();
        std::cout << "Parsed frames: " << mp3.getFrameCount() << "\n";
    };

    commands["trim-start"] = [](int c, char* v[]) {
        if (c < 5) throw std::invalid_argument("Usage: trim-start <input> <output> <value> [frames|seconds]");
        MP3File mp3(v[2]);
        mp3.parse();
        
        std::string unit = (c > 5) ? v[5] : "frames";
        double val = std::stod(v[4]);

        std::unique_ptr<RangeStrategy> strategy;
        if (unit == "seconds") {
            strategy = std::make_unique<SecondRangeStrategy>(SecondRangeStrategy::Mode::TrimStart, val);
        } else {
            strategy = std::make_unique<FrameRangeStrategy>(FrameRangeStrategy::Mode::TrimStart, static_cast<size_t>(val));
        }

        mp3.applyRange(*strategy);
        mp3.save(v[3]);
        std::cout << "Trimmed start and saved to " << v[3] << "\n";
    };

    commands["trim-end"] = [](int c, char* v[]) {
        if (c < 5) throw std::invalid_argument("Usage: trim-end <input> <output> <value> [frames|seconds]");
        MP3File mp3(v[2]);
        mp3.parse();
        
        std::string unit = (c > 5) ? v[5] : "frames";
        double val = std::stod(v[4]);

        std::unique_ptr<RangeStrategy> strategy;
        if (unit == "seconds") {
            strategy = std::make_unique<SecondRangeStrategy>(SecondRangeStrategy::Mode::TrimEnd, val);
        } else {
            strategy = std::make_unique<FrameRangeStrategy>(FrameRangeStrategy::Mode::TrimEnd, static_cast<size_t>(val));
        }

        mp3.applyRange(*strategy);
        mp3.save(v[3]);
        std::cout << "Trimmed end and saved to " << v[3] << "\n";
    };

    commands["extract"] = [](int c, char* v[]) {
        if (c < 6) throw std::invalid_argument("Usage: extract <input> <output> <start> <duration> [frames|seconds]");
        MP3File mp3(v[2]);
        mp3.parse();
        
        std::string unit = (c > 6) ? v[6] : "frames";
        double start = std::stod(v[4]);
        double duration = std::stod(v[5]);

        std::unique_ptr<RangeStrategy> strategy;
        if (unit == "seconds") {
            strategy = std::make_unique<SecondRangeStrategy>(SecondRangeStrategy::Mode::Extract, start, duration);
        } else {
            strategy = std::make_unique<FrameRangeStrategy>(FrameRangeStrategy::Mode::Extract, static_cast<size_t>(start), static_cast<size_t>(duration));
        }

        mp3.applyRange(*strategy);
        mp3.save(v[3]);
        std::cout << "Extracted range and saved to " << v[3] << "\n";
    };

    commands["merge"] = [](int c, char* v[]) {
        if (c < 5) throw std::invalid_argument("Usage: merge <input1> <input2> <output>");
        MP3File mp3(v[2]);
        mp3.parse();
        mp3.merge(v[3]);
        mp3.save(v[4]);
        std::cout << "Merged files and saved to " << v[4] << "\n";
    };

    commands["cut"] = [](int c, char* v[]) {
        if (c < 6) throw std::invalid_argument("Usage: cut <input> <output> <start> <duration> [frames|seconds]");
        MP3File mp3(v[2]);
        mp3.parse();
        
        std::string unit = (c > 6) ? v[6] : "frames";
        double start = std::stod(v[4]);
        double duration = std::stod(v[5]);

        CutStrategy strategy(start, duration, unit == "seconds");
        mp3.applyCut(strategy);
        mp3.save(v[3]);
        std::cout << "Cut range and saved to " << v[3] << "\n";
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