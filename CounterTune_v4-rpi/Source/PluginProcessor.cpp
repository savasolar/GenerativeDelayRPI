#include "PluginProcessor.h"
#include "CtLog.h"
#include <array>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstdlib>

namespace
{
    constexpr const char* NOTE_NAMES[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    constexpr std::array<double, 12> MAJOR_PROFILE = {
        6.35, 2.23, 3.48, 2.33, 4.38, 4.09,
        2.52, 5.19, 2.39, 3.66, 2.29, 2.88
    };
    constexpr std::array<double, 12> MINOR_PROFILE = {
        6.33, 2.68, 3.52, 5.38, 2.60, 3.53,
        2.54, 4.75, 3.98, 2.69, 3.34, 3.17
    };

    std::array<double, 12> notesToPitchClassHistogram(const std::vector<int>& midiNotes)
    {
        std::array<double, 12> histogram{};
        histogram.fill(0.0);
        for (int note : midiNotes)
        {
            const int pc = ((note % 12) + 12) % 12;
            histogram[(size_t)pc] += 1.0;
        }
        return histogram;
    }

    double correlate(const std::array<double, 12>& histogram, int tonic,
        const std::array<double, 12>& profile)
    {
        std::array<double, 12> rotated{};
        for (int i = 0; i < 12; ++i)
            rotated[(size_t)((tonic + i) % 12)] = profile[(size_t)i];

        const double histMean = std::accumulate(histogram.begin(), histogram.end(), 0.0) / 12.0;
        const double profMean = std::accumulate(rotated.begin(), rotated.end(), 0.0) / 12.0;

        double num = 0.0, histSS = 0.0, profSS = 0.0;
        for (int i = 0; i < 12; ++i)
        {
            const double hd = histogram[(size_t)i] - histMean;
            const double pd = rotated[(size_t)i] - profMean;
            num += hd * pd;
            histSS += hd * hd;
            profSS += pd * pd;
        }

        if (histSS == 0.0 || profSS == 0.0)
            return 0.0;

        return num / std::sqrt(histSS * profSS);
    }

    std::vector<std::pair<juce::String, double>> estimateKey(const std::vector<int>& midiNotes, int topN = 5)
    {
        if (midiNotes.empty())
            return {};

        const auto histogram = notesToPitchClassHistogram(midiNotes);

        std::vector<std::pair<juce::String, double>> scores;
        scores.reserve(24);

        for (int tonic = 0; tonic < 12; ++tonic)
        {
            const juce::String majName = juce::String(NOTE_NAMES[tonic]) + "maj";
            const juce::String minName = juce::String(NOTE_NAMES[tonic]) + "min";

            scores.emplace_back(majName, correlate(histogram, tonic, MAJOR_PROFILE));
            scores.emplace_back(minName, correlate(histogram, tonic, MINOR_PROFILE));
        }

        std::sort(scores.begin(), scores.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        if (static_cast<int>(scores.size()) > topN)
            scores.resize(static_cast<size_t>(topN));

        return scores;
    }

    constexpr int MAJOR_DEG[7] = { 0, 2, 4, 5, 7, 9, 11 };
    constexpr int MINOR_DEG[7] = { 0, 2, 3, 5, 7, 8, 10 };

    bool parseKeyName(const juce::String& name, int& tonic, bool& minor)
    {
        if (name.endsWith("maj"))
            minor = false;
        else if (name.endsWith("min"))
            minor = true;
        else
            return false;

        const juce::String root = name.dropLastCharacters(3);
        for (int i = 0; i < 12; ++i)
        {
            if (root == NOTE_NAMES[i])
            {
                tonic = i;
                return true;
            }
        }
        return false;
    }

    // Every scale degree in a window centered on the detected-note range,
    // never thinner than one octave.
    std::vector<int> realizeScale(int tonic, bool minor, const std::vector<int>& midiNotes)
    {
        if (midiNotes.empty())
            return {};

        bool inScale[12] = {};
        const int* deg = minor ? MINOR_DEG : MAJOR_DEG;
        for (int i = 0; i < 7; ++i)
            inScale[(tonic + deg[i]) % 12] = true;

        const int detLo = *std::min_element(midiNotes.begin(), midiNotes.end());
        const int detHi = *std::max_element(midiNotes.begin(), midiNotes.end());
        const double center = 0.5 * (double)(detLo + detHi);
        const int span = std::max(detHi - detLo, 12);
        const int lo = (int)std::floor(center - 0.5 * (double)span);
        const int hi = (int)std::ceil(center + 0.5 * (double)span);

        std::vector<int> out;
        out.reserve((size_t)((hi - lo) / 2 + 2));
        for (int n = lo; n <= hi; ++n)
        {
            const int pc = ((n % 12) + 12) % 12;
            if (inScale[pc])
                out.push_back(n);
        }
        return out;
    }

    juce::String formatNoteList(const std::vector<int>& notes)
    {
        juce::String s("[");
        for (size_t i = 0; i < notes.size(); ++i)
        {
            if (i > 0)
                s += ", ";
            s += juce::String(notes[i]);
        }
        s += "]";
        return s;
    }

    juce::String formatRealizedKey(const juce::String& keyName, const std::vector<int>& midiNotes)
    {
        int tonic = 0;
        bool minor = false;
        if (!parseKeyName(keyName, tonic, minor))
            return keyName;

        return keyName + " " + formatNoteList(realizeScale(tonic, minor, midiNotes));
    }

} // namespace

PPDAudioProcessor::PPDAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    apvts(*this, nullptr, "Parameters", createParams())
#endif
{
    if (juce::Logger::getCurrentLogger() == nullptr)
        juce::Logger::setCurrentLogger (
            juce::FileLogger::createDefaultAppLogger (
                "MixedSignals", "CounterTune.log", "CounterTune"));

    CT_LOG ("CounterTune starting (Release-safe logging on)");
    pdct.connectOutput(&pdctListener);
    pluginFormatManager.addDefaultFormats();
    loadDefaultPlugin();
}

PPDAudioProcessor::~PPDAudioProcessor()
{
}

const juce::String PPDAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PPDAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PPDAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PPDAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PPDAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PPDAudioProcessor::getNumPrograms()
{
    return 1;
}

int PPDAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PPDAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused(index);
}

const juce::String PPDAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused(index);
    return {};
}

void PPDAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void PPDAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // monophonic pitch detector setup
    dywapitch_inittracking(&mpPitchTracker);
    mpPitchDetectorFillPos = 0;
    mpPitch = 0.0;
    lastValidPitch = 0.0;
    lastChosenNote = 0;
    monoHoldRemainingMs = 0.0;

    stereoLeftNext = true;


    currentSampleRate = sampleRate;

    const int numCh = juce::jmax(1, getTotalNumInputChannels());
    pdctScratch.assign((size_t)samplesPerBlock * (size_t)numCh, 0.0);

    // Re-tune the resonator bank for the host's actual sample rate and
    // clear all filter/energy state.
    pdct.init(sampleRate);
    pdct.reset();

    const juce::ScopedLock sl(detectedNotesLock);

    activeNotes.clear();
    pendingOnsets.clear();
    lingeringNotes.clear();
    availableNotes.clear();
    soundingNotes.clear();
    velSumSq = 0.0;
    velSamples = 0;
    syncPpd();




    dryWetMixer.prepare(juce::dsp::ProcessSpec{ sampleRate, static_cast<std::uint32_t>(samplesPerBlock), static_cast<std::uint32_t>(getTotalNumOutputChannels()) });
    dryWetMixer.setMixingRule(juce::dsp::DryWetMixingRule::balanced);

    msElapsed = 0.0;

#ifdef DEMO_BUILD
//    demoSeconds = 0.0;
#endif

    active = true;
    {
        const juce::ScopedLock sl(innerMutex);
        if (innerPlugin != nullptr)
        {
            innerPlugin->setRateAndBufferSizeDetails(sampleRate, samplesPerBlock);
            innerPlugin->prepareToPlay(sampleRate, samplesPerBlock);
        }
    }
}

// void PPDAudioProcessor::releaseResources()
// {
    // active = false;
    // const juce::ScopedLock sl(innerMutex);
    // if (innerPlugin != nullptr)
        // innerPlugin->releaseResources();
// }

void PPDAudioProcessor::releaseResources()
{
    active = false;
    const juce::ScopedLock sl(innerMutex);
    if (innerPlugin != nullptr)
    {
        // soundingNotes is wiped (silently) in prepareToPlay on the next start,
        // but the inner plugin was never told those notes are off. Flush real
        // note-offs now, while it's still prepared, so it doesn't keep holding
        // them (which is what stacks extra un-released voices on restart).
        const juce::ScopedLock sl2(detectedNotesLock);
        if (!soundingNotes.empty())
        {
            juce::MidiBuffer allNotesOff;
            for (const auto& sn : soundingNotes)
                if (sn.note >= 0 && sn.note <= 127)
                    allNotesOff.addEvent(juce::MidiMessage::noteOff(1, sn.note), 0);

            juce::AudioBuffer<float> silence(juce::jmax(1, getTotalNumOutputChannels()), 1);
            silence.clear();
            innerPlugin->processBlock(silence, allNotesOff);
        }

        innerPlugin->releaseResources();
    }
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PPDAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void PPDAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // -------------------------------------------------------
    // if using host tempo synchronizeBpm() here.
    // -------------------------------------------------------
    // if applying pregain, do it here.
    // -------------------------------------------------------


    const int numSamples = buffer.getNumSamples();





    auto totalNumInputChannels = getTotalNumInputChannels();



    // MONO-PHONIC PITCH DETECTION

    // step 1: mix to mono...
    juce::AudioBuffer<float> monoBlock(1, numSamples);
    monoBlock.clear();





//    const int numChannels = juce::jmin(getTotalNumInputChannels(), buffer.getNumChannels());
//    for (int ch = 0; ch < numChannels; ++ch)
//	monoBlock.addFrom(0, 0, buffer, ch, 0, numSamples);

//    if (numChannels > 0) monoBlock.applyGain(1.0f / numChannels);

    // RPI's input audio is already mono, so averaging it against an empty second channel only quiets it
    if (buffer.getNumChannels() > 0)
        monoBlock.copyFrom(0, 0, buffer, 0, 0, numSamples);





    // ... mono block noise gate ...
    if (monoBlock.getRMSLevel(0, 0, numSamples) < 0.01f)
	monoBlock.clear();

    auto* monoData = monoBlock.getReadPointer(0);

    for (int i = 0; i < numSamples; ++i)
    {
        const double s = (double)monoData[i];
        velSumSq += s * s;
        ++velSamples;
    }

    // ... mid-res mp-pd ...
    int mpAnalysisSpaceLeft = mpAnalysisBuffer.getNumSamples() - mpPitchDetectorFillPos;
    int mpAnalysisToCopy = juce::jmin(mpAnalysisSpaceLeft, numSamples);
    mpAnalysisBuffer.copyFrom(0, mpPitchDetectorFillPos, monoData, mpAnalysisToCopy);
    mpPitchDetectorFillPos += mpAnalysisToCopy;

    if (mpPitchDetectorFillPos >= mpAnalysisBuffer.getNumSamples())
    {
	std::vector<double> doubleSamples(1024);
	for (int i = 0; i < 1024; ++i)
            doubleSamples[i] = mpAnalysisBuffer.getSample(0, i);

    mpPitch = dywapitch_computepitch(&mpPitchTracker, doubleSamples.data(), 0, 1024);
    mpPitch *= (getSampleRate() / 44100.0);
    if (mpPitch > 20.0 && mpPitch < 5000.0)
    {
        lastValidPitch = mpPitch;
        monoHoldRemainingMs = kMonoHoldMs;
    }

//    DBG(juce::String(frequencyToMidiNote((float)mpPitch)));

        mpPitchDetectorFillPos = 0;
//	mpPitch = 0.0;
    }




    // POLY-PHONIC PITCH DETECTION

    const int numCh = juce::jmax(1, totalNumInputChannels);
    const size_t needed = (size_t)numSamples * (size_t)numCh;
    if (pdctScratch.size() < needed)
        pdctScratch.resize(needed);

    const double detectorGain = juce::Decibels::decibelsToGain(6.0);

    for (int ch = 0; ch < numCh; ++ch)
    {
        const float* src = (ch < totalNumInputChannels) ? buffer.getReadPointer(ch) : nullptr;

        for (int i = 0; i < numSamples; ++i)
            pdctScratch[(size_t)i * (size_t)numCh + (size_t)ch] = (src != nullptr) ? (double)src[i] * detectorGain : 0.0;
    }

    pdct.process(pdctScratch.data(), numSamples, numCh);

    // Promote any pending notes that have now been continuously present
    // long enough.  This is the only place that adds notes to activeNotes.
    {
        const juce::ScopedLock sl(detectedNotesLock);
        tickPpd();
    }

    // Every getTimeMsInt() milliseconds, DBG print ("test" + juce::String(testN)) and increment testN
    const double msInterval = (double)getTimeMsInt();







    // CYCLE TRIGGERING

    {
        const juce::ScopedLock sl(detectedNotesLock);
        triggerCycle = !detectedNotes.empty() || !availableNotes.empty()
                    || mpPitch > 0.0 || monoHoldRemainingMs > 0.0;
    }

    const double blockMs = (numSamples / currentSampleRate) * 1000.0;
    if (monoHoldRemainingMs > 0.0)
        monoHoldRemainingMs = juce::jmax(0.0, monoHoldRemainingMs - blockMs);

    struct PendingOff { int note; int offset; };
    std::vector<PendingOff> pendingOffs;
    std::set<int> notesStartedThisBlock;

    {
        const juce::ScopedLock sl(detectedNotesLock);
        pendingOffs.reserve(soundingNotes.size());
        size_t i = 0;
        while (i < soundingNotes.size())
        {
            if (soundingNotes[i].remainingMs <= blockMs)
            {
                int offset = 0;
                if (blockMs > 0.0 && numSamples > 0)
                    offset = juce::jlimit(0, numSamples - 1,
                        (int)std::floor((soundingNotes[i].remainingMs / blockMs) * (double)numSamples));

                pendingOffs.push_back({ soundingNotes[i].note, offset });
                soundingNotes.erase(soundingNotes.begin() + (int)i);
            }
            else
            {
                soundingNotes[i].remainingMs -= blockMs;
                ++i;
            }
        }
    }

    if (triggerCycle)
    {
        msElapsed += blockMs;

        while (msInterval > 0.0 && msElapsed >= msInterval)
        {
            const double rms = (velSamples > 0) ? std::sqrt(velSumSq / (double)velSamples) : 0.0;
            const double rmsDb = juce::Decibels::gainToDecibels(rms, -60.0);
            constexpr double minDb = -36.0;
            constexpr double maxDb = -6.0;
            const double normalized = juce::jlimit(0.0, 1.0, (rmsDb - minDb) / (maxDb - minDb));
            const int velocity = juce::jlimit(1, 127, (int)std::lround(normalized * 127.0));

            int chosenNote = -1;
            {
                const juce::ScopedLock sl(detectedNotesLock);
                if (!availableNotes.empty())
                    chosenNote = availableNotes[(size_t)noteRandom.nextInt((int)availableNotes.size())];
            }

            if (chosenNote < 0 || chosenNote > 127)
                chosenNote = frequencyToMidiNote((float)mpPitch);

            if (chosenNote < 0 || chosenNote > 127)
                chosenNote = frequencyToMidiNote((float)lastValidPitch);

            if (chosenNote < 0 || chosenNote > 127)
                chosenNote = lastChosenNote;

            if (chosenNote < 0 || chosenNote > 127)
                chosenNote = 0;

            bool needRetriggerOff = false;
            {
                const juce::ScopedLock sl(detectedNotesLock);
                size_t s = 0;
                while (s < soundingNotes.size())
                {
                    if (soundingNotes[s].note == chosenNote)
                    {
                        needRetriggerOff = true;
                        soundingNotes.erase(soundingNotes.begin() + (int)s);
                    }
                    else
                    {
                        ++s;
                    }
                }
            }
            for (const auto& off : pendingOffs)
            {
                if (off.note == chosenNote)
                    needRetriggerOff = true;
            }





            if (needRetriggerOff)
                midiMessages.addEvent(juce::MidiMessage::noteOff(1, chosenNote), 0);

	        const float stereo = getStereoFloat();  // 0 = center, 1 = hard L/R
	        const float pan01 = stereoLeftNext ? (0.5f - 0.5f * stereo) : (0.5f + 0.5f * stereo);
	        stereoLeftNext = !stereoLeftNext;
            const int panCc = juce::jlimit(0, 127, (int) std::lround(pan01 * 127.0f));

            midiMessages.addEvent(juce::MidiMessage::controllerEvent(1, kStereoPanCc, panCc), 0);
            midiMessages.addEvent(juce::MidiMessage::noteOn(1, chosenNote, (juce::uint8)velocity), 0);






            // DBG: NOTE ON: chosenNote   | VELOCITY: velocity
	        CT_LOG ("NOTE ON: " << chosenNote << "   | VELOCITY: " << (int) (juce::uint8) velocity);

            notesStartedThisBlock.insert(chosenNote);
            {
                const juce::ScopedLock sl(detectedNotesLock);
                soundingNotes.push_back({ chosenNote, 80.0 + (double)getBloomFloat() * 2920.0 });
            }
            lastChosenNote = chosenNote;

//            DBG("test" + juce::String(testN));
//            ++testN;

            mpPitch = 0.0;

            velSumSq = 0.0;
            velSamples = 0;

            msElapsed -= msInterval;
        }
    }
    else
    {
        msElapsed = 0.0;
        velSumSq = 0.0;
        velSamples = 0;
	stereoLeftNext = true;
    }

    for (const auto& off : pendingOffs)
    {
        if (off.note < 0 || off.note > 127)
            continue;
        if (notesStartedThisBlock.count(off.note) > 0)
            continue;
        midiMessages.addEvent(juce::MidiMessage::noteOff(1, off.note), off.offset);
    }








    // Graduate dry signal
    juce::dsp::AudioBlock<float> block(buffer);
    dryWetMixer.pushDrySamples(block);

    // Hosted plugin renders the wet path (synth overwrites; effect processes in place)
    {
        const juce::ScopedLock sl(innerMutex);
        if (innerPlugin != nullptr)
            innerPlugin->processBlock(buffer, midiMessages);
        else
            block.clear();
    }

    midiMessages.clear();

    // Boost wet signal, it's too quiet

//    block.multiplyBy(juce::Decibels::decibelsToGain(18.0f));
// undid, 'cause guitarguy now has an internal boost
// ... but it's too quiet, so

    block.multiplyBy(juce::Decibels::decibelsToGain(5.0f));



    // Graduate wet signal
    dryWetMixer.setWetMixProportion(getMixFloat());
    dryWetMixer.mixWetSamples(block);

#ifdef DEMO_BUILD
    applyDemoFade(buffer);
#endif
}

#ifdef DEMO_BUILD
void PPDAudioProcessor::applyDemoFade(juce::AudioBuffer<float>& buffer)
{
    const double sr = currentSampleRate > 0.0 ? currentSampleRate : getSampleRate();
    if (sr <= 0.0)
        return;

    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    if (numSamples <= 0 || numCh <= 0)
        return;

    const double invSr = 1.0 / sr;
    float* const* chans = buffer.getArrayOfWritePointers();

    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    if (lastDemoProcessMs > 0.0 && (nowMs - lastDemoProcessMs) > 5000.0)
        demoSeconds = 0.0;
    lastDemoProcessMs = nowMs;

    for (int i = 0; i < numSamples; ++i)
    {
        while (demoSeconds >= kDemoCycleSeconds)
            demoSeconds -= kDemoCycleSeconds;

        float g = 1.0f;
        if (demoSeconds >= kDemoFullSeconds)
        {
            const double fadeT = demoSeconds - kDemoFullSeconds;
            g = (fadeT < kDemoFadeSeconds)
                ? static_cast<float>(1.0 - fadeT / kDemoFadeSeconds)   // 5s out
                : static_cast<float>((fadeT - kDemoFadeSeconds) / kDemoFadeSeconds); // 5s in
        }

        for (int ch = 0; ch < numCh; ++ch)
            chans[ch][i] *= g;

        demoSeconds += invSr;
    }
}
#endif

bool PPDAudioProcessor::hasEditor() const
{
    return false;
}

juce::AudioProcessorEditor* PPDAudioProcessor::createEditor()
{
    return nullptr;
}

void PPDAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    {
        const juce::ScopedLock sl(innerMutex);
        if (innerPlugin != nullptr)
        {
            juce::MemoryBlock pluginState;
            innerPlugin->getStateInformation(pluginState);

            juce::ValueTree pluginTree("InnerPlugin");
            if (auto xml = innerPlugin->getPluginDescription().createXml())
                pluginTree.setProperty("description", xml->toString(), nullptr);
            pluginTree.setProperty("state", pluginState.toBase64Encoding(), nullptr);
            state.addChild(pluginTree, -1, nullptr);
        }
    }

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void PPDAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState == nullptr)
        return;

    juce::ValueTree state = juce::ValueTree::fromXml(*xmlState);
    apvts.replaceState(state);

    juce::ValueTree pluginTree = state.getChildWithName("InnerPlugin");
    if (!pluginTree.isValid())
        return;

    const juce::String descriptionXmlString = pluginTree.getProperty("description");
    const juce::String stateBase64 = pluginTree.getProperty("state");
    juce::MemoryBlock pluginState;
    pluginState.fromBase64Encoding(stateBase64);

    std::unique_ptr<juce::XmlElement> descriptionXml(juce::XmlDocument::parse(descriptionXmlString));
    if (descriptionXml == nullptr)
        return;

    juce::PluginDescription pd;
    if (pd.loadFromXml(*descriptionXml))
        setNewPlugin(pd, pluginState);
}

void PPDAudioProcessor::noteOn(int note)
{
    const juce::ScopedLock sl(detectedNotesLock);

    // Already confirmed and sounding – nothing to do.
    if (activeNotes.count(note) > 0)
        return;

    pendingOnsets[note] = pdct.getCurrentTime();
}

void PPDAudioProcessor::noteOff(int note)
{
    const juce::ScopedLock sl(detectedNotesLock);

    pendingOnsets.erase(note);

    if (activeNotes.erase(note) > 0)
    {
        lingeringNotes[note] = pdct.getCurrentTime();
        syncPpd();
    }
}

void PPDAudioProcessor::syncPpd()
{
    detectedNotes.clear();
    detectedNotes.reserve(activeNotes.size() + lingeringNotes.size());
    detectedNotes.assign(activeNotes.begin(), activeNotes.end());

    for (const auto& kv : lingeringNotes) detectedNotes.push_back(kv.first);
    std::sort(detectedNotes.begin(), detectedNotes.end());

    /*
    juce::String notesStr;
    for (size_t i = 0; i < detectedNotes.size(); ++i)
    {
        if (i > 0)
            notesStr += ", ";
        notesStr += juce::String(detectedNotes[i]);
    }*/

//    DBG("--------------------------------------------------------------------------------");

    //DBG("Detected Notes updated.");
//    DBG("Detected notes:  " + /*notesStr*/formatNoteList(detectedNotes));

    estimatedKeys = detectedNotes.empty() ? std::vector<std::pair<juce::String, double>>{} : estimateKey(detectedNotes, 5);

    //DBG("Estimated Keys updated");
    //DBG("Scale 1: " + (estimatedKeys.size() > 0 ? formatRealizedKey(estimatedKeys[0].first, detectedNotes) : juce::String()));
    //DBG("Scale 2: " + (estimatedKeys.size() > 1 ? formatRealizedKey(estimatedKeys[1].first, detectedNotes) : juce::String()));
    //DBG("Scale 3: " + (estimatedKeys.size() > 2 ? formatRealizedKey(estimatedKeys[2].first, detectedNotes) : juce::String()));
    //DBG("Scale 4: " + (estimatedKeys.size() > 3 ? formatRealizedKey(estimatedKeys[3].first, detectedNotes) : juce::String()));
    //DBG("Scale 5: " + (estimatedKeys.size() > 4 ? formatRealizedKey(estimatedKeys[4].first, detectedNotes) : juce::String()));

    //DBG("Deduplicating...");
    // Conjoin detectedNotes with the realized scale for each of the (up to 5) estimated keys, then dedupe left to right, keeping first-seen order.
    std::vector<int> notePool = detectedNotes;
    for (size_t i = 0; i < estimatedKeys.size() && i < 5; ++i)
    {
        int tonic = 0;
        bool minor = false;
        if (parseKeyName(estimatedKeys[i].first, tonic, minor))
        {
            const auto scaleNotes = realizeScale(tonic, minor, detectedNotes);
            notePool.insert(notePool.end(), scaleNotes.begin(), scaleNotes.end());
        }
    }
    std::vector<int> dedupedNotePool;
    dedupedNotePool.reserve(notePool.size());
    for (int n : notePool)
    {
        if (std::find(dedupedNotePool.begin(), dedupedNotePool.end(), n) == dedupedNotePool.end())
            dedupedNotePool.push_back(n);
    }
    notePool = std::move(dedupedNotePool);
//    DBG("Available notes: " + formatNoteList(notePool));

    const int N = (int)notePool.size();
    const int D = (int)detectedNotes.size();
    const float p = getChaosFloat();
    const float m = chaosMidpoint;

    if (p <= m)
        chaosBandwidth = D + (int)std::round(p * (float)(N - D));
    else
        chaosBandwidth = D + (int)std::round(m * (float)(N - D));

    if (chaosBandwidth > N) chaosBandwidth = N;
    if (chaosBandwidth < 0) chaosBandwidth = 0;

    int incrementUnit = 0;
    if (p > m)
        incrementUnit = (int)std::round((p - m) / (1.0f - m) * (float)(N - chaosBandwidth));

    if (incrementUnit + chaosBandwidth > N)
        incrementUnit = N - chaosBandwidth;
    if (incrementUnit < 0)
        incrementUnit = 0;

    availableNotes.clear();
    if (chaosBandwidth > 0)
        availableNotes.assign(notePool.begin() + incrementUnit, notePool.begin() + incrementUnit + chaosBandwidth);

//    DBG("Chaotic subset:  " + formatNoteList(availableNotes));






}

void PPDAudioProcessor::tickPpd()
{
    const double now = pdct.getCurrentTime();
    bool dirty = false;

    for (auto it = pendingOnsets.begin(); it != pendingOnsets.end(); )
    {
        if (now - it->second >= kConfirmationSeconds)
        {
            activeNotes.insert(it->first);
            lingeringNotes.erase(it->first);
            it = pendingOnsets.erase(it);
            dirty = true;
        }
        else
        {
            ++it;
        }
    }

    for (auto it = lingeringNotes.begin(); it != lingeringNotes.end(); )
    {
        if (now - it->second >= kContextHoldSeconds)
        {
            it = lingeringNotes.erase(it);
            dirty = true;
        }
        else
        {
            ++it;
        }
    }

    if (dirty)
        syncPpd();
}

std::vector<int> PPDAudioProcessor::getDetectedNotes() const
{
    const juce::ScopedLock sl(detectedNotesLock);
    return detectedNotes;
}

std::vector<std::pair<juce::String, double>> PPDAudioProcessor::getEstimatedKeys() const
{
    const juce::ScopedLock sl(detectedNotesLock);
    return estimatedKeys;
}

std::vector<int> PPDAudioProcessor::getSoundingNotes() const
{
    const juce::ScopedLock sl(detectedNotesLock);
    std::vector<int> notes;
    notes.reserve(soundingNotes.size());
    for (const auto& sn : soundingNotes)
        notes.push_back(sn.note);
    return notes;
}

void PPDAudioProcessor::setNewPlugin(const juce::PluginDescription& pd, const juce::MemoryBlock& mb)
{
    const juce::ScopedLock sl(innerMutex);

    if (innerPlugin != nullptr)
    {
        innerPlugin->releaseResources();
        innerPlugin = nullptr;
    }

    auto callback = [this, mb](std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error)
        {
            if (error.isNotEmpty())
            {
                CT_LOG ("Failed to load hosted plugin: " << error);
                return;
            }

            const juce::ScopedLock sl2(innerMutex);
            innerPlugin = std::move(instance);

            if (innerPlugin != nullptr)
                CT_LOG ("Hosted plugin loaded: " << innerPlugin->getName());
            else
                CT_LOG ("Hosted plugin instance was null after createPluginInstanceAsync");

            if (!mb.isEmpty() && innerPlugin != nullptr)
                innerPlugin->setStateInformation(mb.getData(), (int)mb.getSize());

            if (active && innerPlugin != nullptr)
            {
                innerPlugin->setRateAndBufferSizeDetails(getSampleRate(), getBlockSize());
                innerPlugin->prepareToPlay(getSampleRate(), getBlockSize());
            }

            juce::MessageManager::callAsync([this]()
                {
                    juce::NullCheckedInvocation::invoke(pluginChanged);
                });
        };

    pluginFormatManager.createPluginInstanceAsync(pd, getSampleRate(), getBlockSize(), callback);
}

bool PPDAudioProcessor::isPluginLoaded() const
{
    const juce::ScopedLock sl(innerMutex);
    return innerPlugin != nullptr;
}

juce::String PPDAudioProcessor::getLoadedPluginName() const
{
    const juce::ScopedLock sl(innerMutex);
    return innerPlugin != nullptr ? innerPlugin->getName() : juce::String("empty");
}

void PPDAudioProcessor::clearPlugin()
{
    const juce::ScopedLock sl(innerMutex);
    innerPlugin = nullptr;
    juce::NullCheckedInvocation::invoke(pluginChanged);
}

juce::File PPDAudioProcessor::findDefaultPluginFile() const
{
    const juce::StringArray candidateNames { "GuitarGuy.vst3", "GuitarGuy.component" };

    if (const char* env = std::getenv ("COUNTERTUNE_PLUGIN"))
    {
        const juce::File fromEnv (env);
        if (fromEnv.exists())
        {
            CT_LOG ("Using COUNTERTUNE_PLUGIN=" << fromEnv.getFullPathName());
            return fromEnv;
        }
        CT_LOG ("COUNTERTUNE_PLUGIN set but missing: " << env);
    }

    juce::File dir = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory();

    // Walk up from the raw binary so a sibling GuitarGuy.vst3 next to the Standalone still resolves.
    for (int i = 0; i < 6; ++i)
    {
        for (const auto& name : candidateNames)
        {
            const juce::File candidate = dir.getChildFile (name);
            if (candidate.exists())
            {
                CT_LOG ("Found hosted plugin: " << candidate.getFullPathName());
                return candidate;
            }
        }
        dir = dir.getParentDirectory();
    }

    const juce::File home = juce::File::getSpecialLocation (juce::File::userHomeDirectory);
    for (const auto& name : candidateNames)
    {
        const juce::File candidate = home.getChildFile (name);
        if (candidate.exists())
        {
            CT_LOG ("Found hosted plugin in home: " << candidate.getFullPathName());
            return candidate;
        }
    }

    return {};
}

void PPDAudioProcessor::loadDefaultPlugin()
{
    const juce::File pluginFile = findDefaultPluginFile();
    if (!pluginFile.exists())
    {
        CT_LOG ("GuitarGuy.vst3 not found (sibling walk, $HOME, COUNTERTUNE_PLUGIN).");
        return;
    }

    juce::OwnedArray<juce::PluginDescription> descriptions;
    for (auto* format : pluginFormatManager.getFormats())
    {
        if (format->fileMightContainThisPluginType(pluginFile.getFullPathName()))
        {
            format->findAllTypesForFile(descriptions, pluginFile.getFullPathName());
            if (descriptions.size() > 0)
            {
                setNewPlugin(*descriptions[0]);
                return;
            }
        }
    }
}


juce::AudioProcessorValueTreeState::ParameterLayout PPDAudioProcessor::createParams()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("Chaos", "Chaos", juce::NormalisableRange<float>{0.00f, 1.0f, 0.01f}, 0.00f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("Bloom", "Bloom", juce::NormalisableRange<float>{0.00f, 1.0f, 0.01f}, 0.00f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("TimeMs", "TimeMs", 80, 800, 200));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("Stereo", "Stereo", juce::NormalisableRange<float>{0.00f, 1.0f, 0.01f}, 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("Mix", "Mix", juce::NormalisableRange<float>{0.00f, 1.0f, 0.01f}, 0.50f));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PPDAudioProcessor();
}
