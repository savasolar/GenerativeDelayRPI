// PluginProcessor.cpp

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GuitarGuyAudioProcessor::GuitarGuyAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts (*this, nullptr, "Parameters", createParams())
#endif
{
    loadGuitarSamples();

    for (int i = 0; i < 16; ++i)
    {
        auto* voice = new SynthVoice();
	voice->setSampleBank (&sampleBank);
	synth.addVoice (voice);
    }

    synth.addSound (new SynthSound());
}

GuitarGuyAudioProcessor::~GuitarGuyAudioProcessor()
{
}

//==============================================================================
const juce::String GuitarGuyAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GuitarGuyAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool GuitarGuyAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool GuitarGuyAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double GuitarGuyAudioProcessor::getTailLengthSeconds() const
{
    return 10.0;
}

int GuitarGuyAudioProcessor::getNumPrograms()
{
    return 1;
}

int GuitarGuyAudioProcessor::getCurrentProgram()
{
    return 0;
}

void GuitarGuyAudioProcessor::setCurrentProgram (int /*index*/)
{
}

const juce::String GuitarGuyAudioProcessor::getProgramName (int /*index*/)
{
    return {};
}

void GuitarGuyAudioProcessor::changeProgramName (int /*index*/, const juce::String& /*newName*/)
{
}

//==============================================================================
void GuitarGuyAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
	    voice->prepareToPlay (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    }
}

void GuitarGuyAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool GuitarGuyAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void GuitarGuyAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SynthVoice*> (synth.getVoice (i)))
	{
	    auto& attack = *apvts.getRawParameterValue("ATTACK");
	    auto& decay = *apvts.getRawParameterValue("DECAY");
	    auto& sustain = *apvts.getRawParameterValue("SUSTAIN");
	    auto& release = *apvts.getRawParameterValue("RELEASE");

	    voice->update (attack.load(), decay.load(), sustain.load(), release.load());
	}
    }

    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());

//    int highest = -1;
//    for (int i = 0; i < synth.getNumVoices(); ++i)
//    {
//	auto* voice = synth.getVoice(i);
//	if (voice != nullptr && voice->isVoiceActive())
//	    highest = juce::jmax(highest, voice->getCurrentlyPlayingNote());
//    }
//    displayNoteClass.store(highest < 0 ? -1 : (highest % 12), std::memory_order_relaxed);

    int highest = -1;
#if JUCE_DEBUG
    juce::String activeNotesStr;
#endif

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
	auto* voice = synth.getVoice(i);
	if (voice != nullptr && voice->isVoiceActive())
	{
	    const int note = voice->getCurrentlyPlayingNote();
	    highest = juce::jmax(highest, note);

	   #if JUCE_DEBUG
	    if (activeNotesStr.isNotEmpty())
	        activeNotesStr << ", ";
	    activeNotesStr << note;
	   #endif
	}
    }

#if JUCE_DEBUG
    if (activeNotesStr != lastActiveNotesDebugString)
    {
        lastActiveNotesDebugString = activeNotesStr;
        DBG ("Active MIDI notes: [" << (activeNotesStr.isEmpty() ? "none" : activeNotesStr)
             << "] -> pitch class " << (highest < 0 ? -1 : (highest % 12)));
    }
#endif

    displayNoteClass.store(highest < 0 ? -1 : (highest % 12), std::memory_order_relaxed);

}

//==============================================================================
bool GuitarGuyAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* GuitarGuyAudioProcessor::createEditor()
{
    return new GuitarGuyAudioProcessorEditor (*this);
}

void GuitarGuyAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void GuitarGuyAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GuitarGuyAudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout GuitarGuyAudioProcessor::createParams()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("ATTACK",  "Attack",  juce::NormalisableRange<float> { 0.01f, 1.0f, 0.01f }, 0.01f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("DECAY",   "Decay",   juce::NormalisableRange<float> { 0.01f, 1.0f, 0.01f }, 0.1f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("SUSTAIN", "Sustain", juce::NormalisableRange<float> { 0.00f, 1.0f, 0.01f }, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("RELEASE", "Release", juce::NormalisableRange<float> { 0.01f, 5.0f, 0.01f }, 0.01f));

    return { params.begin(), params.end() };
}

void GuitarGuyAudioProcessor::loadGuitarSamples()
{
    static const char* suffixes[] = { "", "b", "c", "d", "h" };
    juce::WavAudioFormat wavFormat;
    int loaded = 0;

    for (int note = 30; note <= 95; ++note)
    {
        sampleBank.variants[(size_t) note].clear();

        for (auto* suf : suffixes)
        {
            const juce::String resourceName = "_" + juce::String (note) + suf + "_wav";
            int dataSize = 0;
            const char* data = BinaryData::getNamedResource (resourceName.toRawUTF8(), dataSize);

            if (data == nullptr || dataSize <= 0)
                continue;

            std::unique_ptr<juce::AudioFormatReader> reader (
                wavFormat.createReaderFor (new juce::MemoryInputStream (data, (size_t) dataSize, false), true));

            if (reader == nullptr)
                continue;

            GuitarSample sample;
            sample.buffer.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
            reader->read (&sample.buffer, 0, (int) reader->lengthInSamples, 0, true, true);
            sampleBank.sourceSampleRate = reader->sampleRate;
            sampleBank.variants[(size_t) note].push_back (std::move (sample));
            ++loaded;
        }
    }

    DBG ("Loaded " << loaded << " guitar samples");
}


