#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

static juce::Font getHeavyFont()
{
#if JUCE_MAC
    float size = 14.0f;
#else
    float size = 16.0f;
#endif
    static auto typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::AzeretMonoBold_ttf, BinaryData::AzeretMonoBold_ttfSize);
    return juce::Font(typeface).withHeight(size);
}

class CustomKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomKnobLookAndFeel() {}

    void setImages(const juce::Image& normalImg, const juce::Image& hoverImg)
    {
        normalImage = normalImg;
        hoverImage = hoverImg;
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, const float rotaryStartAngle,
        const float rotaryEndAngle, juce::Slider& slider) override
    {
        juce::Image& img = slider.isMouseButtonDown() ? hoverImage : normalImage;

        float centerX = x + width * 0.5f;
        float centerY = y + height * 0.5f;

        float imgW = (float)img.getWidth();
        float imgH = (float)img.getHeight();

        float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        auto transform = juce::AffineTransform::translation(-imgW * 0.5f, -imgH * 0.5f)
            .rotated(angle)
            .translated(centerX, centerY);

        g.drawImageTransformed(img, transform, false);
    }

    juce::Slider::SliderLayout getSliderLayout(juce::Slider& slider) override
    {
        juce::Slider::SliderLayout layout;
        layout.textBoxBounds = { 0, 51, 54, 18 };
        layout.sliderBounds = { 9, 0, 42, 42 };
        return layout;
    }

    juce::Label* createSliderTextBox(juce::Slider& slider) override
    {
        auto* l = juce::LookAndFeel_V4::createSliderTextBox(slider);
        l->setJustificationType(juce::Justification::centred);
        l->setFont(getHeavyFont());
        l->setColour(juce::Label::textColourId, juce::Colours::black);
        l->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
        l->setColour(juce::Label::outlineWhenEditingColourId, juce::Colours::transparentBlack);
        return l;
    }

private:
    juce::Image normalImage;
    juce::Image hoverImage;
};

class PPDAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    PPDAudioProcessorEditor(PPDAudioProcessor&);
    ~PPDAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;


private:

    PPDAudioProcessor& audioProcessor;

    void timerCallback() override;

    CustomKnobLookAndFeel knobLAF;

    // Chaos (%)
    juce::Label chaosTitleLabel;
    juce::Slider chaosKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> chaosAttachment;

    // Time (ms)
    juce::Label timeTitleLabel;
    juce::Slider timeMsKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> timeMsAttachment;

    // Bloom (%)
    juce::Label bloomTitleLabel;
    juce::Slider bloomKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bloomAttachment;

    // Stereo (%)
    juce::Label stereoTitleLabel;
    juce::Slider stereoKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stereoAttachment;

    // Mix (%)
    juce::Label mixTitleLabel;
    juce::Slider mixKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;




    // Texture
    juce::Label textureTitleLabel;

    juce::Label openLabel;
    juce::Label loadLabel;
    juce::Label resetLabel;

    juce::ImageButton openPluginButton;
    juce::ImageButton loadPluginButton;
    juce::ImageButton resetPluginButton;

    void selectPlugin();
    void openPluginWindow();
    void resetPlugin();
    juce::OwnedArray<juce::DocumentWindow> activeWindows;
    std::unique_ptr<juce::FileChooser> chooser;




    // Graphics
    juce::Image backgroundImage;
    juce::Image knobImage;

    std::vector<int> heldOutNotes;
    double lastOutputActivityMs = 0.0;
    static constexpr double kOutHoldMs = 800.0;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PPDAudioProcessorEditor)
};
