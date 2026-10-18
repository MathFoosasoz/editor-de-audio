#pragma once

#include <string>
#include <unordered_map>
#include <functional>

// Classe para receber um comando por CLI, fazer parse nele, e dar dispatch do comando no nosso programa
class CommandDispatcher {
private:
    std::unordered_map<std::string, std::function<void(int, char*[])>> commands;

public:
    CommandDispatcher();
    void execute(int argc, char* argv[]);
};