#include <iostream>
#include "include/core/MP3File.hpp"
#include "include/exceptions/AudioExceptions.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_mp3>\n";
        return 1;
    }

    try {
        MP3File mp3(argv[1]);
        mp3.parse();
        std::cout << "MP3 file successfully parsed and validated.\n";
        std::cout << "Total frames found: " << mp3.getFrameCount() << "\n";
    } catch (const AudioException& e) {
        std::cerr << "Audio Error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Standard Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}