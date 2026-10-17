#pragma once

#include <string>
#include <unordered_map>
#include <functional>

class CommandDispatcher {
private:
    std::unordered_map<std::string, std::function<void(int, char*[])>> commands;

public:
    CommandDispatcher();
    void execute(int argc, char* argv[]);
};