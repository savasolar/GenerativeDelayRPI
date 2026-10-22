#pragma once
#include <JuceHeader.h>
#include "SynthSound.h"
#include "ADSRData.h"

// Preloaded guitar samples
struct GuitarSample
{
    juce::AudioBuffer<float> buffer;
};

struct GuitarSampleBank
{
    std::array<std::vector<GuitarSample>, 128> variants;
    double sourceSampleRate = 44100.0;
};

class SynthVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound(juce::SynthesiserSound*) override;
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels);
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void update(const float attack, const float decay, const float sustain, const float release);

    void setSampleBank (const GuitarSampleBank* bank) { sampleBank = bank; }

    void setPan (float newPan) noexcept { pan = juce::jlimit (-1.0f, 1.0f, newPan); }

private:
    ADSRData adsr;
    juce::AudioBuffer<float> synthBuffer;

    const GuitarSampleBank* sampleBank = nullptr;
    const GuitarSample* playingSample = nullptr;
    int readPos = 0;

    juce::dsp::Gain<float> gain;
    float pan = 0.0f;  // -1 left, +1 right, set before startNote
    bool isPrepared{ false };
    juce::Random random;

    void WAV(const juce::AudioBuffer<float>& buffer, juce::String filename);

};
