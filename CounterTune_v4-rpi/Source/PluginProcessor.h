#pragma once

#include <JuceHeader.h>
#include "cPdct.h"
#include "cPdctOutput.h"
#include "dywapitchtrack.h"
#include <utility>
#include <set>
#include <vector>
#include <map>

class PPDAudioProcessor  : public juce::AudioProcessor
{
public:
    PPDAudioProcessor();
    ~PPDAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    std::vector<int> getDetectedNotes() const;

    std::vector<std::pair<juce::String, double>> getEstimatedKeys() const;

    std::vector<int> getSoundingNotes() const;


    float getChaosFloat() const { return *apvts.getRawParameterValue("Chaos"); }
    void setChaosFloat(float newChaosFloat) { auto* param = apvts.getParameter("Chaos"); auto range = param->getNormalisableRange(); param->setValueNotifyingHost(range.convertTo0to1(newChaosFloat)); }

    float getBloomFloat() const { return *apvts.getRawParameterValue("Bloom"); }
    void setBloomFloat(float newBloomFloat) { auto* param = apvts.getParameter("Bloom"); auto range = param->getNormalisableRange(); param->setValueNotifyingHost(range.convertTo0to1(newBloomFloat)); }

   
    int getTimeMsInt() const { return *apvts.getRawParameterValue("TimeMs"); }
    void setTimeMsInt(int newTimeMsInt) { auto* param = apvts.getParameter("TimeMs"); auto range = param->getNormalisableRange(); param->setValueNotifyingHost(range.convertTo0to1(newTimeMsInt)); }

    

    float getStereoFloat() const { return *apvts.getRawParameterValue("Stereo"); }
    void setStereoFloat(float newStereoFloat) { auto* param = apvts.getParameter("Stereo"); auto range = param->getNormalisableRange(); param->setValueNotifyingHost(range.convertTo0to1(newStereoFloat)); }

    
    float getMixFloat() const { return *apvts.getRawParameterValue("Mix"); }
    void setMixFloat(float newMixFloat) { auto* param = apvts.getParameter("Mix"); auto range = param->getNormalisableRange(); param->setValueNotifyingHost(range.convertTo0to1(newMixFloat)); }

    juce::AudioProcessorValueTreeState apvts;

    // internal plugin hosting
    juce::AudioPluginFormatManager pluginFormatManager;
    std::function<void()> pluginChanged;
    void setNewPlugin(const juce::PluginDescription& pd, const juce::MemoryBlock& mb = {});
    bool isPluginLoaded() const;
    juce::String getLoadedPluginName() const;
    void clearPlugin();
    void loadDefaultPlugin();


private:
    // MONO-PHONIC PITCH DETECTION
    dywapitchtracker mpPitchTracker;
    juce::AudioBuffer<float> mpAnalysisBuffer{ 1, 1024 };
    int mpPitchDetectorFillPos = 0;
        double mpPitch = 0.0;
    double lastValidPitch = 0.0;
    int lastChosenNote = 0;
    double monoHoldRemainingMs = 0.0;
    static constexpr double kMonoHoldMs = 250.0;
    std::vector<float> mpDetectedFrequencies;
    std::vector<int> mpDetectedNoteNumbers;
    inline int frequencyToMidiNote(float frequency)
    {
        if (frequency <= 20.0f || frequency > 5000.0f) return -1;
        const int note = static_cast<int>(std::round(12.0f * std::log2(frequency / 440.0f) + 69.0f));
        if (note < 0 || note > 127) return -1;
        return note;
    }


    // apparently, at the end of a "cycle" (in this case, a single note?) -- RESET pitchDetectorFillPos, clear detectedFrequencies, detectedNoteNumbers, and don't touch analysisBuffer


    // POLY-PHONIC PITCH DETECTION
    class NoteListener : public cPdctOutput
    {
    public:
        explicit NoteListener(PPDAudioProcessor& ownerIn) : owner(ownerIn) {}
        void onDetectNoteOn(int note, int velocity) override
        {
            juce::ignoreUnused(velocity);
            owner.noteOn(note);
        }
        void onDetectNoteOff(int note, int velocity) override
        {
            juce::ignoreUnused(velocity);
            owner.noteOff(note);
        }
    private:
        PPDAudioProcessor& owner;
    };

    void noteOn(int note);
    void noteOff(int note);

    // Must be called with detectedNotesLock already held.
    void syncPpd();
    void tickPpd();
    double currentSampleRate = 44100.0;

    static constexpr double kConfirmationSeconds = 0.18;  // RAISED FROM 0.1 FOR SLIGHTLY IMPROVED ACCURACY

    static constexpr double kContextHoldSeconds = 3.0;

    cPdct pdct;
    NoteListener pdctListener{ *this };

    std::vector<double> pdctScratch;

    mutable juce::CriticalSection detectedNotesLock;
    std::set<int> activeNotes;
    std::map<int, double> pendingOnsets;
    std::map<int, double> lingeringNotes;
    std::vector<int> detectedNotes;
    std::vector<int> availableNotes;
    std::vector<std::pair<juce::String, double>> estimatedKeys;

    juce::Random noteRandom;

    struct SoundingNote
    {
        int note = 0;
        double remainingMs = 0.0;
    };
    std::vector<SoundingNote> soundingNotes;

    double velSumSq = 0.0;
    int velSamples = 0;








    juce::AudioProcessorValueTreeState::ParameterLayout createParams();





    // EQ-LIKE CHAOS CONTROL


    int chaosBandwidth = 0;
    float chaosMidpoint = 0.2f;



    // time flies
    bool triggerCycle = false;
    bool stereoLeftNext = true;
    static constexpr int kStereoPanCc = 10;



    int testN = 0;
    double msElapsed = 0.0;


    // PLAYBACK VARS
    juce::dsp::DryWetMixer<float> dryWetMixer;


#ifdef DEMO_BUILD
    void applyDemoFade(juce::AudioBuffer<float>& buffer);
    double demoSeconds = 0.0;
    double lastDemoProcessMs = 0.0;
    static constexpr double kDemoFullSeconds  = 45.0;
    static constexpr double kDemoFadeSeconds  = 5.0;
    static constexpr double kDemoCycleSeconds = kDemoFullSeconds + kDemoFadeSeconds * 2.0; // 55
#endif



    bool active = false;
    juce::CriticalSection innerMutex;
    std::unique_ptr<juce::AudioPluginInstance> innerPlugin;

    juce::File findDefaultPluginFile() const;











    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PPDAudioProcessor)
};
