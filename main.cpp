#include <iostream>
#include "include/cli/CommandDispatcher.hpp"
#include "include/exceptions/AudioExceptions.hpp"

int main(int argc, char* argv[]) {
    CommandDispatcher dispatcher;

    try {
        if (argc < 2) {
            dispatcher.runInteractive();
        } else {
            dispatcher.execute(argc - 1, argv + 1);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}