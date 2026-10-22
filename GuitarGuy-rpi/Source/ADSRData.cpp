/*
  ==============================================================================

    ADSRData.cpp
    Created: 10 May 2026 1:52:34pm
    Author:  savas

  ==============================================================================
*/

#include "ADSRData.h"

void ADSRData::updateADSR(const float attack, const float decay, const float sustain, const float release)
{
    adsrParams.attack = attack;
    adsrParams.decay = decay;
    adsrParams.sustain = sustain;
    adsrParams.release = release;

    setParameters(adsrParams);
}