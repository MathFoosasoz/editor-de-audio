#pragma once

#include "AudioFrame.hpp"
#include <cstdint>

class MPEGFrame : public AudioFrame {
protected:
    uint32_t header = 0;
    size_t frameSize = 0;

public:
    virtual ~MPEGFrame() = default;
    size_t getSize() const override;
};