#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    void setupImageButton(juce::ImageButton& button,
                          const char* normalData, int normalSize,
                          const char* downData, int downSize)
    {
        const auto normal = juce::ImageCache::getFromMemory(normalData, normalSize);
        const auto down   = juce::ImageCache::getFromMemory(downData, downSize);

        button.setImages(false, true, true,
                         normal, 1.0f, juce::Colours::transparentBlack,
                         normal, 1.0f, juce::Colours::transparentBlack,
                         down,   1.0f, juce::Colours::transparentBlack);
        button.setWantsKeyboardFocus(false);
        button.setMouseClickGrabsKeyboardFocus(false);
    }
}

PPDAudioProcessorEditor::PPDAudioProcessorEditor(PPDAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Chaos
    addAndMakeVisible(chaosKnob);
    chaosKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    chaosKnob.setMouseDragSensitivity(333);
    chaosKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 54, 18);
    chaosAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Chaos", chaosKnob);
    chaosKnob.setLookAndFeel(&knobLAF);
    chaosKnob.textFromValueFunction = [](double value) {
        return juce::String(juce::roundToInt(value * 100)) + "%";
    };
    chaosKnob.valueFromTextFunction = [](const juce::String& text) {
        return text.trimCharactersAtEnd("%").getDoubleValue() / 100.0;
    };
    chaosKnob.updateText();
    addAndMakeVisible(chaosTitleLabel);
    chaosTitleLabel.setText("CHAOS", juce::dontSendNotification);
    chaosTitleLabel.setJustificationType(juce::Justification::centred);
    chaosTitleLabel.setFont(getHeavyFont());
    chaosTitleLabel.setColour(juce::Label::textColourId, juce::Colours::black);

    // Bloom
    addAndMakeVisible(bloomKnob);
    bloomKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    bloomKnob.setMouseDragSensitivity(333);
    bloomKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 54, 18);
    bloomAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Bloom", bloomKnob);
    bloomKnob.setLookAndFeel(&knobLAF);
    bloomKnob.textFromValueFunction = [](double value) {
        return juce::String(juce::roundToInt(value * 100)) + "%";
    };
    bloomKnob.valueFromTextFunction = [](const juce::String& text) {
        return text.trimCharactersAtEnd("%").getDoubleValue() / 100.0;
    };
    bloomKnob.updateText();
    addAndMakeVisible(bloomTitleLabel);
    bloomTitleLabel.setText("BLOOM", juce::dontSendNotification);
    bloomTitleLabel.setJustificationType(juce::Justification::centred);
    bloomTitleLabel.setFont(getHeavyFont());
    bloomTitleLabel.setColour(juce::Label::textColourId, juce::Colours::black);


    // Time (ms)
    addAndMakeVisible(timeMsKnob);
    timeMsKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    timeMsKnob.setMouseDragSensitivity(333);
    timeMsKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 54, 18);
    timeMsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "TimeMs", timeMsKnob);
    timeMsKnob.setLookAndFeel(&knobLAF);
    timeMsKnob.setTextValueSuffix(" MS");
    timeMsKnob.updateText();
    
    addAndMakeVisible(timeTitleLabel);
    timeTitleLabel.setText("TIME", juce::dontSendNotification);
    timeTitleLabel.setJustificationType(juce::Justification::centred);
    timeTitleLabel.setFont(getHeavyFont());
    timeTitleLabel.setColour(juce::Label::textColourId, juce::Colours::black);

    // Stereo
    addAndMakeVisible(stereoKnob);
    stereoKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    stereoKnob.setMouseDragSensitivity(333);
    stereoKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 54, 18);
    stereoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Stereo", stereoKnob);
    stereoKnob.setLookAndFeel(&knobLAF);
    stereoKnob.textFromValueFunction = [](double value) {
        return juce::String(juce::roundToInt(value * 100)) + "%";
    };
    stereoKnob.valueFromTextFunction = [](const juce::String& text) {
        return text.trimCharactersAtEnd("%").getDoubleValue() / 100.0;
    };
    stereoKnob.updateText();
    addAndMakeVisible(stereoTitleLabel);
    stereoTitleLabel.setText("STEREO", juce::dontSendNotification);
    stereoTitleLabel.setJustificationType(juce::Justification::centred);
    stereoTitleLabel.setFont(getHeavyFont());
    stereoTitleLabel.setColour(juce::Label::textColourId, juce::Colours::black);

    // Mix
    addAndMakeVisible(mixKnob);
    mixKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixKnob.setMouseDragSensitivity(333);
    mixKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 54, 18);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Mix", mixKnob);
    mixKnob.setLookAndFeel(&knobLAF);
    mixKnob.textFromValueFunction = [](double value) {
        return juce::String(juce::roundToInt(value * 100)) + "%";
    };
    mixKnob.valueFromTextFunction = [](const juce::String& text) {
        return text.trimCharactersAtEnd("%").getDoubleValue() / 100.0;
    };
    mixKnob.updateText();
    addAndMakeVisible(mixTitleLabel);
    mixTitleLabel.setText("MIX", juce::dontSendNotification);
    mixTitleLabel.setJustificationType(juce::Justification::centred);
    mixTitleLabel.setFont(getHeavyFont());
    mixTitleLabel.setColour(juce::Label::textColourId, juce::Colours::black);

    // Texture
    addAndMakeVisible(textureTitleLabel);
    textureTitleLabel.setText("TEXTURE", juce::dontSendNotification);
    textureTitleLabel.setJustificationType(juce::Justification::centred);
    textureTitleLabel.setFont(getHeavyFont());
    textureTitleLabel.setColour(juce::Label::textColourId, juce::Colours::black);

    // Plugin hosting
    addAndMakeVisible(openPluginButton);
    setupImageButton(openPluginButton, BinaryData::openbutton_png, BinaryData::openbutton_pngSize, BinaryData::openbuttonactive_png, BinaryData::openbuttonactive_pngSize);
    openPluginButton.onClick = [this]() { openPluginWindow(); };
    addAndMakeVisible(openLabel);
    openLabel.setText("OPEN", juce::dontSendNotification);
    openLabel.setJustificationType(juce::Justification::centred);
    openLabel.setFont(getHeavyFont());
    openLabel.setColour(juce::Label::textColourId, juce::Colours::black);
    
    addAndMakeVisible(loadPluginButton);
    setupImageButton(loadPluginButton, BinaryData::loadbutton_png, BinaryData::loadbutton_pngSize, BinaryData::loadbuttonactive_png, BinaryData::loadbuttonactive_pngSize);
    loadPluginButton.onClick = [this]() { selectPlugin(); };
    addAndMakeVisible(loadLabel);
    loadLabel.setText("LOAD", juce::dontSendNotification);
    loadLabel.setJustificationType(juce::Justification::centred);
    loadLabel.setFont(getHeavyFont());
    loadLabel.setColour(juce::Label::textColourId, juce::Colours::black);

    addAndMakeVisible(resetPluginButton);
    setupImageButton(resetPluginButton, BinaryData::resetbutton_png, BinaryData::resetbutton_pngSize, BinaryData::resetbuttonactive_png, BinaryData::resetbuttonactive_pngSize);
    resetPluginButton.onClick = [this]() { resetPlugin(); };
    addAndMakeVisible(resetLabel);
    resetLabel.setText("RESET", juce::dontSendNotification);
    resetLabel.setJustificationType(juce::Justification::centred);
    resetLabel.setFont(getHeavyFont());
    resetLabel.setColour(juce::Label::textColourId, juce::Colours::black);

    // Graphics
    backgroundImage = juce::ImageCache::getFromMemory(BinaryData::template_png, BinaryData::template_pngSize);
    knobImage = juce::ImageCache::getFromMemory(BinaryData::knobfacade_png, BinaryData::knobfacade_pngSize);
    knobLAF.setImages(knobImage, knobImage);
    
    setSize(567, 330);

    startTimerHz(25);
}

PPDAudioProcessorEditor::~PPDAudioProcessorEditor()
{
    activeWindows.clear();
    chaosKnob.setLookAndFeel(nullptr);
    bloomKnob.setLookAndFeel(nullptr);
    timeMsKnob.setLookAndFeel(nullptr);
    stereoKnob.setLookAndFeel(nullptr);
    mixKnob.setLookAndFeel(nullptr);
}

void PPDAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.drawImage(backgroundImage, getLocalBounds().toFloat());

    auto outNotes = audioProcessor.getSoundingNotes();
    if (outNotes.empty())
        outNotes = heldOutNotes;
    if (outNotes.empty())
        return;
    auto inNotes = audioProcessor.getDetectedNotes();
    if (inNotes.empty())
        inNotes = outNotes;
    if (inNotes.empty())
        return;
    
    
    
    auto noteToX = [](int note) -> float
        {
            return 19.0f + (float)(note - 40) * 11.0f;
        };

    constexpr int kNoteMin = 40;
    constexpr int kNoteMax = 88;
    constexpr float kYOut = 16.0f;
    constexpr float kYIn = 214.0f;

    g.saveState();
    g.reduceClipRegion(juce::Rectangle<int>(17, 15, 534, 200));
    g.setColour(juce::Colours::white);

    for (int inNote : inNotes)
    {
        if (inNote < kNoteMin || inNote > kNoteMax)
            continue;

        const float xIn = noteToX(inNote);

        for (int outNote : outNotes)
        {
            if (outNote < kNoteMin || outNote > kNoteMax)
                continue;

            g.drawLine(xIn, kYIn, noteToX(outNote), kYOut, 2.0f);
        }
    }

    g.restoreState();
}

void PPDAudioProcessorEditor::resized()
{
    chaosTitleLabel.setBounds(15, 230, 54, 18);
    chaosKnob.setBounds(15, 251, 54, 69);

    bloomTitleLabel.setBounds(84, 230, 54, 18);
    bloomKnob.setBounds(84, 251, 54, 69);

    timeTitleLabel.setBounds(153, 230, 54, 18);
    timeMsKnob.setBounds(153, 251, 54, 69);

    stereoTitleLabel.setBounds(222, 230, 54, 18);
    stereoKnob.setBounds(222, 251, 54, 69);

    mixTitleLabel.setBounds(291, 230, 54, 18);
    mixKnob.setBounds(291, 251, 54, 69);

    textureTitleLabel.setBounds(419, 230, 74, 18);

    openPluginButton.setBounds(367, 253, 42, 42);
    openLabel.setBounds(360, 302, 54, 18);
    
    loadPluginButton.setBounds(436, 253, 42, 42);
    loadLabel.setBounds(429, 302, 54, 18);
    
    resetPluginButton.setBounds(505, 253, 42, 42);
    resetLabel.setBounds(498, 302, 54, 18);
}

void PPDAudioProcessorEditor::timerCallback()
{
    const auto liveOut = audioProcessor.getSoundingNotes();
    const double nowMs = juce::Time::getMillisecondCounterHiRes();

    if (!liveOut.empty())
    {
        heldOutNotes = liveOut;
        lastOutputActivityMs = nowMs;
    }
    else if (nowMs - lastOutputActivityMs >= kOutHoldMs)
    {
        heldOutNotes.clear();
    }

    repaint(17, 15, 534, 200);
}

void PPDAudioProcessorEditor::selectPlugin()
{
    activeWindows.clear();

#if JUCE_MAC
    const juce::String wildcard = "*.vst;*.vst3;*.component";
    const juce::File startDir("/Library/Audio/Plug-Ins");
#elif JUCE_WINDOWS
    const juce::String wildcard = "*.dll;*.vst3";
    const juce::File startDir = juce::File::getSpecialLocation(juce::File::globalApplicationsDirectory)
        .getChildFile("Common Files");
#else
    const juce::String wildcard = "*.vst3;*.so";
    const juce::File startDir;
#endif

    chooser = std::make_unique<juce::FileChooser>("Select a plugin...", startDir, wildcard);
    const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(flags, [this](const juce::FileChooser& fc)
        {
            const juce::File file = fc.getResult();
            if (!file.exists())
                return;

            juce::OwnedArray<juce::PluginDescription> descriptions;
            for (auto* format : audioProcessor.pluginFormatManager.getFormats())
            {
                if (format->fileMightContainThisPluginType(file.getFullPathName()))
                {
                    format->findAllTypesForFile(descriptions, file.getFullPathName());
                    if (descriptions.size() > 0)
                    {
                        audioProcessor.setNewPlugin(*descriptions[0]);
                        return;
                    }
                }
            }
        });
}

void PPDAudioProcessorEditor::openPluginWindow()
{
    activeWindows.clear();

    if (!audioProcessor.isPluginLoaded())
        return;

    auto editor = audioProcessor.createInnerEditor();
    if (editor == nullptr)
        return;



    const bool editorIsResizable = editor->isResizable();

    class PluginWindow : public juce::DocumentWindow
    {
    public:
        PluginWindow(const juce::String& name, juce::Colour backgroundColour, bool resizable)
            : DocumentWindow(name, backgroundColour,
                juce::DocumentWindow::minimiseButton | juce::DocumentWindow::closeButton,
                false)
        {
            setUsingNativeTitleBar(true);
            setResizable(resizable, false);
        }

        void closeButtonPressed() override
        {
            if (onClosing)
                onClosing();
        }

        std::function<void()> onClosing;
    };

    auto* window = new PluginWindow(audioProcessor.getLoadedPluginName(), juce::Colours::darkgrey, editorIsResizable);

    window->setContentOwned(editor.release(), true);
    window->addToDesktop(window->getDesktopWindowStyleFlags());
    window->centreWithSize(window->getWidth(), window->getHeight());

    window->onClosing = [this, window]() { activeWindows.removeObject(window); };
    activeWindows.add(window);
    window->setAlwaysOnTop(true);
    window->setVisible(true);

    if (auto* peer = window->getPeer())
        peer->toFront(true);
}

void PPDAudioProcessorEditor::resetPlugin()
{
    activeWindows.clear();
    audioProcessor.loadDefaultPlugin();
}

