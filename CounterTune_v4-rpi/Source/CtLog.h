#pragma once

#include <JuceHeader.h>
#include <cstdio>

/** Always-on log for Pi/SSH: stderr (live) + optional FileLogger. */
inline void ctLog (const juce::String& s)
{
    juce::Logger::writeToLog (s);
    std::fprintf (stderr, "[CounterTune] %s\n", s.toRawUTF8());
    std::fflush (stderr);
}

#define CT_LOG(msg) ctLog (juce::String() << msg)
