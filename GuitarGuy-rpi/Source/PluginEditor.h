// PluginEditor.h

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================

class GuitarGuyAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    GuitarGuyAudioProcessorEditor (GuitarGuyAudioProcessor&);
    ~GuitarGuyAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateDisplayedImage();
    static juce::Image loadBinaryImage (const juce::String& resourceName);

    GuitarGuyAudioProcessor& audioProcessor;

    juce::Image silentImage;
    juce::Image noteImages[12];
    juce::Image currentImage;
    int currentNoteClass = -2; // force first update

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuitarGuyAudioProcessorEditor)
};
