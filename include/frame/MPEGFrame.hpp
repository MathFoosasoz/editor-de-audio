#pragma once

#include "AudioFrame.hpp"
#include <cstdint>

// Layer do formato MPEG, que possui necessariamente 4 bytes de header
class MPEGFrame : public AudioFrame {
protected:
    uint32_t header = 0;
    size_t frameSize = 0;

public:
    virtual ~MPEGFrame() = default;
    size_t getSize() const override;
};