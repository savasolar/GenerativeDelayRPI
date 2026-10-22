#pragma once

#include <JuceHeader.h>
#include "SynthSound.h"
#include "SynthVoice.h"
#include <atomic>

class GuitarSynthesiser : public juce::Synthesiser
{
public:
    static constexpr int kStereoPanCc = 10;

    void handleController (int midiChannel, int controllerNumber, int controllerValue) override
    {
	if (controllerNumber == kStereoPanCc)
	{
	    nextNotePan = juce::jmap((float) controllerValue, 0.0f, 127.0f, -1.0f, 1.0f);
	    return;
	}

	juce::Synthesiser::handleController(midiChannel, controllerNumber, controllerValue);
    }

    void noteOn(int midiChannel, int midiNoteNumber, float velocity) override
    {
	const juce::ScopedLock sl(lock);

	for (auto* sound : sounds)
	{
	    if (sound->appliesToNote(midiNoteNumber) && sound->appliesToChannel(midiChannel))
	    {
		auto* voice = findFreeVoice(sound, midiChannel, midiNoteNumber, isNoteStealingEnabled());

		if (auto* gv = dynamic_cast<SynthVoice*>(voice)) gv->setPan(nextNotePan);

		startVoice(voice, sound, midiChannel, midiNoteNumber, velocity);
	    }
	}
    }

    void noteOff(int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff) override
    {
        const juce::ScopedLock sl(lock);

        juce::SynthesiserVoice* oldestHeld = nullptr;

        for (auto* voice : voices)
        {
            if (voice->getCurrentlyPlayingNote() != midiNoteNumber
                || !voice->isPlayingChannel(midiChannel)
                || !voice->isKeyDown())
                continue;

            if (auto sound = voice->getCurrentlyPlayingSound())
            {
                if (!sound->appliesToNote(midiNoteNumber)
                    || !sound->appliesToChannel(midiChannel))
                    continue;

                if (oldestHeld == nullptr || voice->wasStartedBefore(*oldestHeld))
                    oldestHeld = voice;
            }
        }

        if (oldestHeld == nullptr)
            return;

        oldestHeld->setKeyDown(false);

        if (!(oldestHeld->isSustainPedalDown() || oldestHeld->isSostenutoPedalDown()))
            stopVoice(oldestHeld, velocity, allowTailOff);
    }

private:
    float nextNotePan = 0.0f;

};

//==============================================================================

class GuitarGuyAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    GuitarGuyAudioProcessor();
    ~GuitarGuyAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // -1 = silent, 0..11 = pitch class of the highest active MIDI note
    int getDisplayNoteClass() const noexcept
    {
	return displayNoteClass.load(std::memory_order_relaxed);
    }

private:
//    juce::Synthesiser synth;


    // <new>
    GuitarSynthesiser synth;
    // </new>


    GuitarSampleBank sampleBank;
    std::atomic<int> displayNoteClass{ -1 };

#if JUCE_DEBUG
    juce::String lastActiveNotesDebugString;
#endif

    juce::AudioProcessorValueTreeState::ParameterLayout createParams();
    void loadGuitarSamples();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuitarGuyAudioProcessor)
};
