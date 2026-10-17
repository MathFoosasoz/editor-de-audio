#pragma once

#include "MPEGFrame.hpp"

class Layer3Frame : public MPEGFrame {
public:
    void parse(std::ifstream& file) override;
};