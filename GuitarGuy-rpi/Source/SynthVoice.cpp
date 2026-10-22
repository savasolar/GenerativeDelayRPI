#include "SynthVoice.h"

bool SynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*>(sound) != nullptr;
}

void SynthVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{
    juce::ignoreUnused (sound, currentPitchWheelPosition);

    playingSample = nullptr;
    readPos = 0;

    if (sampleBank == nullptr || midiNoteNumber < 30 || midiNoteNumber > 95)
    {
        clearCurrentNote();
        return;
    }

    const auto& variants = sampleBank->variants[(size_t) midiNoteNumber];

    if (variants.empty())
    {
        clearCurrentNote();
        return;
    }

    const int index = random.nextInt ((int) variants.size());
    playingSample = &variants[(size_t) index];

	if (playingSample->buffer.getNumSamples() <= 0)
	{
		playingSample = nullptr;
		clearCurrentNote();
		return;
	}

//    gain.setGainLinear (0.25f * juce::jlimit (0.0f, 1.0f, velocity));

//    adsr.noteOn();

// <new>
    constexpr float maxLevel   = 1.0f;   // overall ceiling
    constexpr float minLevelDb = -30.0f;  // how quiet the softest velocity should be

    const float normVelocity = juce::jlimit (0.0f, 1.0f, velocity);
    const float velocityDb   = juce::jmap (normVelocity, 0.0f, 1.0f, minLevelDb, 0.0f);

    gain.setGainLinear (maxLevel * juce::Decibels::decibelsToGain (velocityDb));

    adsr.noteOn();
// </new>
}

void SynthVoice::stopNote(float velocity, bool allowTailOff)
{
    adsr.noteOff();

    if (!allowTailOff || !adsr.isActive())
        clearCurrentNote();
}

void SynthVoice::pitchWheelMoved(int /*newPitchWheelValue*/)
{

}

void SynthVoice::controllerMoved(int /*controllerNumber*/, int /*newControllerValue*/)
{

}

void SynthVoice::prepareToPlay(double sampleRate, int samplesPerBlock, int outputChannels)
{
    adsr.setSampleRate(sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = (juce::uint32) outputChannels;

    gain.prepare(spec);
    gain.setGainLinear(1.0f);

    isPrepared = true;
}

void SynthVoice::update(const float attack, const float decay, const float sustain, const float release)
{
    adsr.updateADSR(attack, decay, sustain, release);
}

//                    _           _   _           _   ____  _            _    
// _ __ ___ _ __   __| | ___ _ __| \ | | _____  _| |_| __ )| | ___   ___| | __
//| '__/ _ \ '_ \ / _` |/ _ \ '__|  \| |/ _ \ \/ / __|  _ \| |/ _ \ / __| |/ /
//| | |  __/ | | | (_| |  __/ |  | |\  |  __/>  <| |_| |_) | | (_) | (__|   < 
//|_|  \___|_| |_|\__,_|\___|_|  |_| \_|\___/_/\_\\__|____/|_|\___/ \___|_|\_\
//

void SynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    jassert(isPrepared);

    if (!isVoiceActive() || playingSample == nullptr || playingSample->buffer.getNumSamples() <= 0)
        return;

    synthBuffer.setSize (outputBuffer.getNumChannels(), numSamples, false, false, true);
    synthBuffer.clear();

    const int numOutCh  = synthBuffer.getNumChannels();
    const int srcFrames = playingSample->buffer.getNumSamples();
    const int srcChans  = playingSample->buffer.getNumChannels();

    if (srcChans > 0)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            if (readPos >= srcFrames)
                break;

            for (int ch = 0; ch < numOutCh; ++ch)
            {
                const int sCh = ch % srcChans;
                synthBuffer.getWritePointer (ch)[i] = playingSample->buffer.getReadPointer (sCh)[readPos];
            }

            ++readPos;
        }
    }

    juce::dsp::AudioBlock<float> audioBlock{ synthBuffer };
    gain.process(juce::dsp::ProcessContextReplacing<float>(audioBlock));
    adsr.applyEnvelopeToBuffer(synthBuffer, 0, synthBuffer.getNumSamples());

    const float angle = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
    const float leftG = std::cos (angle);
    const float rightG = std::sin (angle);

    for (int channels = 0; channels < outputBuffer.getNumChannels(); ++channels)
    {
        const float channelGain = (channels == 0) ? leftG : (channels == 1) ? rightG : 1.0f;
	
        outputBuffer.addFrom(channels, startSample, synthBuffer, channels, 0, numSamples, channelGain);

        if (!adsr.isActive())
            clearCurrentNote();
    }
}

void SynthVoice::WAV(const juce::AudioBuffer<float>& buffer, juce::String filename)
{
    juce::File outputDir("C:\\WAVBUFFERS");
    outputDir.createDirectory();

    juce::File outputFile = outputDir.getChildFile(filename + ".wav");

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(
            new juce::FileOutputStream(outputFile),
            getSampleRate(),
            buffer.getNumChannels(),
            24,
            {},
            0
        )
    );

    if (writer != nullptr)
    {
        writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
        DBG(filename + ".wav saved to C:\\WAVBUFFERS");
    }
    else
    {
        DBG("WAV(): Failed to create writer for " + filename + ".wav");
    }
}
