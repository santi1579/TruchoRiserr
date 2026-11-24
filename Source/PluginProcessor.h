#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class TruchoRiserProcessor : public juce::AudioProcessor
{
public:
    TruchoRiserProcessor();
    ~TruchoRiserProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "TruchoRiser"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static const juce::StringArray& typeNames();

    juce::AudioProcessorValueTreeState apvts;

    // Estado compartido con el editor
    std::atomic<bool> listenOn { false };
    std::atomic<bool> loopOn { true };
    std::atomic<float> playhead { 0.0f };

private:
    void atSegmentEnd (int newLen);

    std::atomic<float> *pRise = nullptr, *pType = nullptr, *pPhone = nullptr,
                       *pChorus = nullptr, *pTrem = nullptr, *pFx = nullptr;

    double sr = 44100.0;
    int maxLen = 0, pos = 0, capLen = 1, playLen = 0, writeIdx = 0, listenSegs = 2;
    bool noiseCap = false, wasListen = false;

    std::vector<float> buf[2][2];   // [doble buffer][canal]
    std::vector<float> chorBuf[2];
    int chorW = 0;
    double chorPhase = 0.0, tremPhase = 0.0;
    float noiseLp[2] { 0.0f, 0.0f };

    juce::Random rng;
    juce::dsp::StateVariableTPTFilter<float> sweepHP[2], phoneHP[2], phoneLP[2];
    juce::Reverb reverb;
    juce::AudioBuffer<float> wetBuf;
    juce::SmoothedValue<float> riseSm;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TruchoRiserProcessor)
};
