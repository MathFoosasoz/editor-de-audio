#include <iostream>
#include "include/cli/CommandDispatcher.hpp"
#include "include/exceptions/AudioExceptions.hpp"

int main(int argc, char* argv[]) {
 
    //std::cout << "b" << std::endl;
    try {
        CommandDispatcher dispatcher;
        dispatcher.execute(argc, argv);
    } catch (const AudioException& e) {
        std::cerr << "Audio Error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}