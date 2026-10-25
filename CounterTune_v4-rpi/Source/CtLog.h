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

// Named String required: juce::String() is a temporary and operator<< needs a non-const lvalue.
#define CT_LOG(msg) do { juce::String _ctLogMsg; _ctLogMsg << msg; ctLog (_ctLogMsg); } while (0)