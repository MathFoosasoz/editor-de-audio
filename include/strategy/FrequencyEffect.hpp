#pragma once

// Strategy que define o ganho de cada frequência, em amplitude linear.
class FrequencyEffect {
public:
    virtual ~FrequencyEffect() = default;
    virtual void validateSampleRate(int sampleRate) const {}
    virtual double getGain(double frequencyHz) const = 0;
};
