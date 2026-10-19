#include "../../include/cli/CommandDispatcher.hpp"
#include "../../include/core/AudioFile.hpp"
#include "../../include/core/AudioFileFactory.hpp"
#include "../../include/strategy/FrameRangeStrategy.hpp"
#include "../../include/strategy/SecondRangeStrategy.hpp"
#include "../../include/strategy/CutStrategy.hpp"
#include <iostream>
#include <stdexcept>
#include <memory>
#include <sstream>

CommandDispatcher::CommandDispatcher() {
    commands["help"] = [this](int c, char* v[]) {
        std::cout << "\nComandos:\n"
                  << "  parse <input>\n"
                  << "  trim-start <input> <output> <val> [frames|seconds]\n"
                  << "  trim-end <input> <output> <val> [frames|seconds]\n"
                  << "  extract <input> <output> <start> <duration> [frames|seconds]\n"
                  << "  cut <input> <output> <start> <duration> [frames|seconds]\n"
                  << "  merge <input1> <input2> <output>\n"
                  << "  bass-boost <input> <output> <gain>\n"
                  << "  help\n"
                  << "  exit / quit\n\n";
    };

    commands["exit"] = [this](int c, char* v[]) {
        running = false;
        std::cout << "Encerrando o editor de áudio...\n";
    };

    commands["quit"] = commands["exit"];

    commands["parse"] = [](int c, char* v[]) {
        if (c < 2) throw std::invalid_argument("Uso: parse <input>");
        auto audio = createAudioFile(v[1]);
        audio->parse();
        std::cout << "Feito parse em frames: " << audio->getFrameCount() << "\n";
    };

    commands["trim-start"] = [](int c, char* v[]) {
        if (c < 4) throw std::invalid_argument("Uso: trim-start <input> <output> <value> [frames|seconds]");
        auto audio = createAudioFile(v[1]);
        audio->parse();
        
        std::string unit = (c > 4) ? v[4] : "frames";
        double val = std::stod(v[3]);

        std::unique_ptr<RangeStrategy> strategy;
        if (unit == "seconds") {
            strategy = std::make_unique<SecondRangeStrategy>(SecondRangeStrategy::Mode::TrimStart, val);
        } else {
            strategy = std::make_unique<FrameRangeStrategy>(FrameRangeStrategy::Mode::TrimStart, static_cast<size_t>(val));
        }

        audio->applyRange(*strategy);
        audio->save(v[2]);
        std::cout << "Cortado começo e salvo para: " << v[2] << "\n";
    };

    commands["trim-end"] = [](int c, char* v[]) {
        if (c < 4) throw std::invalid_argument("Uso: trim-end <input> <output> <value> [frames|seconds]");
        auto audio = createAudioFile(v[1]);
        audio->parse();
        
        std::string unit = (c > 4) ? v[4] : "frames";
        double val = std::stod(v[3]);

        std::unique_ptr<RangeStrategy> strategy;
        if (unit == "seconds") {
            strategy = std::make_unique<SecondRangeStrategy>(SecondRangeStrategy::Mode::TrimEnd, val);
        } else {
            strategy = std::make_unique<FrameRangeStrategy>(FrameRangeStrategy::Mode::TrimEnd, static_cast<size_t>(val));
        }

        audio->applyRange(*strategy);
        audio->save(v[2]);
        std::cout << "Cortado e salvo para: " << v[2] << "\n";
    };

    commands["extract"] = [](int c, char* v[]) {
        if (c < 5) throw std::invalid_argument("Uso: extract <input> <output> <start> <duration> [frames|seconds]");
        auto audio = createAudioFile(v[1]);
        audio->parse();
        
        std::string unit = (c > 5) ? v[5] : "frames";
        double start = std::stod(v[3]);
        double duration = std::stod(v[4]);

        std::unique_ptr<RangeStrategy> strategy;
        if (unit == "seconds") {
            strategy = std::make_unique<SecondRangeStrategy>(SecondRangeStrategy::Mode::Extract, start, duration);
        } else {
            strategy = std::make_unique<FrameRangeStrategy>(FrameRangeStrategy::Mode::Extract, static_cast<size_t>(start), static_cast<size_t>(duration));
        }

        audio->applyRange(*strategy);
        audio->save(v[2]);
        std::cout << "Extraído intervalo e salvo para: " << v[2] << "\n";
    };

    commands["merge"] = [](int c, char* v[]) {
        if (c < 4) throw std::invalid_argument("Uso: merge <input1> <input2> <output>");
        auto audio = createAudioFile(v[1]);
        audio->parse();
        audio->merge(v[2]);
        audio->save(v[3]);
        std::cout << "Uniu arquivos e salvou para " << v[3] << "\n";
    };

    commands["cut"] = [](int c, char* v[]) {
        if (c < 5) throw std::invalid_argument("Uso: cut <input> <output> <start> <duration> [frames|seconds]");
        auto audio = createAudioFile(v[1]);
        audio->parse();
        
        std::string unit = (c > 5) ? v[5] : "frames";
        double start = std::stod(v[3]);
        double duration = std::stod(v[4]);

        CutStrategy strategy(start, duration, unit == "seconds");
        audio->applyCut(strategy);
        audio->save(v[2]);
        std::cout << "Cortado e salvo para: " << v[2] << "\n";
    };

    commands["bass-boost"] = [](int c, char* v[]) {
        if (c < 4) throw std::invalid_argument("Uso: bass-boost <input> <output> <gain_factor>");
        
        auto audio = createAudioFile(v[1]);
        audio->parse();
        
        PCMFile* pcmFile = dynamic_cast<PCMFile*>(audio.get());
        if (!pcmFile) {
            throw std::invalid_argument("bass-boost é suportado somente para arquivos PCM (.wav).");
        }
        
        float gain = std::stof(v[3]);
        pcmFile->applyBassBoost(gain);
        
        audio->save(v[2]);
        std::cout << "Aplicado bass boost (gain " << gain << ") e salvo para " << v[2] << "\n";
    };
}

void CommandDispatcher::execute(int argc, char* argv[]) {
    if (argc < 1) {
        throw std::invalid_argument("Nenhum comando dado.");
    }

    std::string command = argv[0];
    auto it = commands.find(command);
    if (it != commands.end()) {
        it->second(argc, argv);
    } else {
        throw std::invalid_argument("Comando desconhecido: " + command);
    }
}

void CommandDispatcher::runInteractive() {
    std::cout << "Editor de Áudio: digite 'help' para comandos ou 'quit' para encerrar.\n";
    std::string line;

    while (running) {
        std::cout << "audio-editor> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string arg;
        std::vector<std::string> args;
        while (ss >> arg) {
            args.push_back(arg);
        }

        if (args.empty()) continue;

        std::vector<char*> argv;
        for (auto& s : args) {
            argv.push_back(&s[0]);
        }

        try {
            execute(static_cast<int>(argv.size()), argv.data());
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }
}