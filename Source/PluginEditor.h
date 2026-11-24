#pragma once
#include "PluginProcessor.h"

namespace TruchoColours
{
    const juce::Colour bg0    (0xff05070d);
    const juce::Colour bg1    (0xff0c1428);
    const juce::Colour accent (0xff3d8bff);
    const juce::Colour glow   (0xff6cc3ff);
    const juce::Colour text   (0xffe3eeff);
    const juce::Colour dim    (0xff6b7a99);
}

inline juce::Font truchoFont (float h, bool bold = true)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

class TruchoLnF : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startA, float endA, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override {}
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
};

class TruchoRiserEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TruchoRiserEditor (TruchoRiserProcessor&);
    ~TruchoRiserEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void stepType (int delta);
    void drawWaveform (juce::Graphics&, juce::Rectangle<float>);

    TruchoRiserProcessor& proc;
    TruchoLnF lnf;

    juce::Slider rise;
    juce::ToggleButton phone { "Phone" }, chorus { "Chorus" }, tremolo { "Tremolo" },
                       rawfx { "" }, listen { "Listen" }, loop { "Loop" };
    juce::TextButton prev { "<" }, next { ">" }, resetBtn { "Reset" };

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SA> riseA;
    std::unique_ptr<BA> phoneA, chorusA, tremoloA, rawfxA;

    juce::Rectangle<float> waveArea;
    std::vector<float> jitter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TruchoRiserEditor)
};
