class AudioFrame {
public:
    virtual ~AudioFrame() = default;
    virtual double getDurationInSeconds() const = 0;
};