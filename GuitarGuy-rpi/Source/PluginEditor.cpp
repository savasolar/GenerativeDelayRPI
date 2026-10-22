// PluginEditor.cpp

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::String noteResourceName (int noteClass)
    {
	    return "_" + juce::String (noteClass) + "_png";
    }
}

juce::Image GuitarGuyAudioProcessorEditor::loadBinaryImage (const juce::String& resourceName)
{
    int dataSize = 0;
    const char* data = BinaryData::getNamedResource (resourceName.toRawUTF8(), dataSize);

    if (data == nullptr || dataSize <= 0)
        return {};

    return juce::ImageCache::getFromMemory (data, dataSize);
}

GuitarGuyAudioProcessorEditor::GuitarGuyAudioProcessorEditor (GuitarGuyAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (300, 225);
    setOpaque (true);
    setResizable (false, false);

    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
    setBroughtToFrontOnMouseClick (false);

    silentImage = loadBinaryImage("fin_png");

    for (int i = 0; i < 12; ++i)
	noteImages[i] = loadBinaryImage(noteResourceName(i));

    updateDisplayedImage();
    startTimerHz(30);
}

GuitarGuyAudioProcessorEditor::~GuitarGuyAudioProcessorEditor()
{
    stopTimer();
}

void GuitarGuyAudioProcessorEditor::updateDisplayedImage()
{
    const int noteClass = audioProcessor.getDisplayNoteClass();

    if (noteClass == currentNoteClass)
	return;

    currentNoteClass = noteClass;

    if (noteClass >= 0 && noteClass < 12 && noteImages[noteClass].isValid())
	currentImage = noteImages[noteClass];
    else
	currentImage = silentImage;

    repaint();
}

void GuitarGuyAudioProcessorEditor::timerCallback()
{
    updateDisplayedImage();
}

void GuitarGuyAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (currentImage.isValid())
    {
	g.drawImage(currentImage, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit, false);
    }
    else
    {
	g.fillAll(juce::Colours::black);
    }
}

void GuitarGuyAudioProcessorEditor::resized()
{
    
}
