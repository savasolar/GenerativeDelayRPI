// SynthSound.h

#pragma once
#include <JuceHeader.h>

class SynthSound : public juce::SynthesiserSound
{
public:

    bool appliesToNote (int midiNoteNumber) override
    {
        return midiNoteNumber >= 30 && midiNoteNumber <= 95;
    }

    bool appliesToChannel (int /*midiChannel*/) override { return true; }

};
