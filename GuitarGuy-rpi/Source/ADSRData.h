/*
  ==============================================================================

    ADSRData.h
    Created: 10 May 2026 1:52:34pm
    Author:  savas

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class ADSRData : public juce::ADSR
{
public:
    void updateADSR(const float attack, const float decay, const float sustain, const float release);

private:
    juce::ADSR::Parameters adsrParams;
};