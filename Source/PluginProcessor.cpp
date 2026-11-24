#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float kBeats[5] = { 1.0f, 2.0f, 4.0f, 8.0f, 16.0f };
    constexpr int kChorSize = 16384;
}

const juce::StringArray& TruchoRiserProcessor::typeNames()
{
    static const juce::StringArray names { "Reverse 1/4 Bar", "Reverse 1/2 Bar", "Reverse 1 Bar",
                                           "Reverse 2 Bars", "Reverse 4 Bars" };
    return names;
}

juce::AudioProcessorValueTreeState::ParameterLayout TruchoRiserProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "rise", 1 }, "Rise",
                juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "type", 1 }, "Type",
                typeNames(), 1));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "phone", 1 }, "Phone", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "chorus", 1 }, "Chorus", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "tremolo", 1 }, "Tremolo", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "fx", 1 }, "FX", true));
    return layout;
}

TruchoRiserProcessor::TruchoRiserProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pRise   = apvts.getRawParameterValue ("rise");
    pType   = apvts.getRawParameterValue ("type");
    pPhone  = apvts.getRawParameterValue ("phone");
    pChorus = apvts.getRawParameterValue ("chorus");
    pTrem   = apvts.getRawParameterValue ("tremolo");
    pFx     = apvts.getRawParameterValue ("fx");
}

bool TruchoRiserProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void TruchoRiserProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    maxLen = (int) (25.0 * sr);
    for (auto& set : buf)
        for (auto& c : set)
            c.assign ((size_t) maxLen, 0.0f);
    for (auto& c : chorBuf)
        c.assign ((size_t) kChorSize, 0.0f);

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) samplesPerBlock, 2 };
    for (int ch = 0; ch < 2; ++ch)
    {
        sweepHP[ch].prepare (spec); sweepHP[ch].setType (juce::dsp::StateVariableTPTFilterType::highpass);
        sweepHP[ch].setCutoffFrequency (60.0f);
        phoneHP[ch].prepare (spec); phoneHP[ch].setType (juce::dsp::StateVariableTPTFilterType::highpass);
        phoneHP[ch].setCutoffFrequency (400.0f);
        phoneLP[ch].prepare (spec); phoneLP[ch].setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        phoneLP[ch].setCutoffFrequency (3400.0f);
        sweepHP[ch].reset(); phoneHP[ch].reset(); phoneLP[ch].reset();
    }

    reverb.setSampleRate (sr);
    reverb.reset();
    wetBuf.setSize (2, samplesPerBlock);
    riseSm.reset (sr, 0.05);
    riseSm.setCurrentAndTargetValue (pRise->load());

    pos = 0; capLen = (int) sr; playLen = 0; writeIdx = 0;
    chorW = 0; chorPhase = tremPhase = 0.0;
    noiseCap = wasListen = false;
}

void TruchoRiserProcessor::atSegmentEnd (int newLen)
{
    playLen = capLen;
    writeIdx ^= 1;
    pos = 0;
    capLen = newLen;

    if (listenOn.load() && ! loopOn.load())
        if (--listenSegs <= 0)
            listenOn = false;

    noiseCap = listenOn.load() && (loopOn.load() || listenSegs == 2);
}

void TruchoRiserProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int numCh = juce::jmin (2, buffer.getNumChannels());
    for (int ch = numCh; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, n);
    if (maxLen <= 0 || n == 0)
        return;

    const bool  fxOn   = pFx->load() > 0.5f;
    const bool  phone  = pPhone->load() > 0.5f;
    const bool  chorus = pChorus->load() > 0.5f;
    const bool  trem   = pTrem->load() > 0.5f;
    const float rise   = pRise->load();
    const int   type   = juce::jlimit (0, 4, (int) pType->load());
    riseSm.setTargetValue (rise);

    // Info del host
    double bpm = 120.0, ppq = 0.0;
    bool hostPlaying = false, hasPpq = false;
    if (auto* ph = getPlayHead())
        if (auto info = ph->getPosition())
        {
            if (auto b = info->getBpm()) bpm = *b;
            hostPlaying = info->getIsPlaying();
            if (auto p = info->getPpqPosition()) { ppq = *p; hasPpq = true; }
        }
    bpm = juce::jlimit (30.0, 300.0, bpm);

    const double segBeats = (double) kBeats[type];
    const int segLen = juce::jlimit (64, maxLen, (int) (segBeats * 60.0 / bpm * sr));

    // Listen: arranca un segmento limpio con ruido
    const bool listening = listenOn.load();
    if (listening && ! wasListen)
    {
        listenSegs = 2; pos = 0; capLen = segLen; playLen = 0; noiseCap = true;
    }
    wasListen = listening;

    // Sincroniza el segmento con la rejilla del DAW
    if (! listening && hostPlaying && hasPpq && capLen == segLen)
    {
        double phase = std::fmod (ppq, segBeats);
        if (phase < 0.0) phase += segBeats;
        const int expected = juce::jmin (segLen - 1, (int) (phase / segBeats * segLen));
        const int diff = std::abs (expected - pos);
        if (diff > 32 && diff < segLen - 32)
            pos = expected;
    }

    wetBuf.setSize (2, n, false, false, true);

    const float endFade = juce::jmax (1.0f, 0.004f * (float) sr);
    const double tremInc = juce::MathConstants<double>::twoPi * (bpm / 60.0 * 2.0) / sr;
    const double chorInc = juce::MathConstants<double>::twoPi * 0.7 / sr;

    for (int i = 0; i < n; ++i)
    {
        if (pos >= capLen)
            atSegmentEnd (segLen);

        const float rs  = riseSm.getNextValue();
        const float t   = playLen > 0 ? juce::jmin (1.0f, (float) pos / (float) playLen) : 0.0f;
        const float env = t * t;
        const float fade = juce::jlimit (0.0f, 1.0f, (float) (playLen - pos) / endFade);

        if ((i & 15) == 0)
        {
            const float fc = juce::jmin (18000.0f, 60.0f * std::pow (130.0f, t * rs));
            sweepHP[0].setCutoffFrequency (fc);
            sweepHP[1].setCutoffFrequency (fc);
        }

        const float tremGain = trem ? 1.0f - 0.75f * (0.5f + 0.5f * (float) std::sin (tremPhase)) : 1.0f;

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float dry = buffer.getReadPointer (ch)[i];

            // Captura (voz o ruido de prueba)
            float src = dry;
            if (noiseCap)
            {
                const float nz = (rng.nextFloat() * 2.0f - 1.0f) * 0.35f;
                noiseLp[ch] += 0.4f * (nz - noiseLp[ch]);
                src = noiseLp[ch];
            }
            buf[writeIdx][ch][(size_t) pos] = src;

            // Reproduccion invertida del segmento anterior
            const int ridx = playLen - 1 - pos;
            float w = ridx >= 0 ? buf[writeIdx ^ 1][ch][(size_t) ridx] : 0.0f;
            w *= env * fade;

            w = sweepHP[ch].processSample (ch, w);

            if (phone)
            {
                w = phoneHP[ch].processSample (ch, w);
                w = phoneLP[ch].processSample (ch, w) * 1.4f;
            }

            // Chorus
            chorBuf[ch][(size_t) chorW] = w;
            if (chorus)
            {
                const float d = (0.012f + 0.004f * (float) std::sin (chorPhase + ch * 1.57)) * (float) sr;
                float rp = (float) chorW - d;
                if (rp < 0.0f) rp += (float) kChorSize;
                const int i0 = (int) rp;
                const int i1 = (i0 + 1) & (kChorSize - 1);
                const float fr = rp - (float) i0;
                const float y = chorBuf[ch][(size_t) i0] * (1.0f - fr) + chorBuf[ch][(size_t) i1] * fr;
                w = 0.5f * w + 0.5f * y;
            }

            w *= tremGain;
            w *= 0.2f + 1.1f * rs;
            wetBuf.getWritePointer (ch)[i] = w;
        }

        chorW = (chorW + 1) & (kChorSize - 1);
        chorPhase += chorInc; if (chorPhase > juce::MathConstants<double>::twoPi) chorPhase -= juce::MathConstants<double>::twoPi;
        tremPhase += tremInc; if (tremPhase > juce::MathConstants<double>::twoPi) tremPhase -= juce::MathConstants<double>::twoPi;
        ++pos;
    }

    // Reverb sobre la señal del riser
    juce::Reverb::Parameters rp;
    const float mix = 0.3f + 0.5f * rise;
    rp.roomSize = 0.55f + 0.4f * rise;
    rp.damping = 0.35f;
    rp.width = 1.0f;
    rp.freezeMode = 0.0f;
    rp.wetLevel = mix * 0.33f;
    rp.dryLevel = (1.0f - mix) * 0.5f;
    reverb.setParameters (rp);

    float* wl = wetBuf.getWritePointer (0);
    if (numCh > 1) reverb.processStereo (wl, wetBuf.getWritePointer (1), n);
    else           reverb.processMono (wl, n);

    if (fxOn)
        for (int ch = 0; ch < numCh; ++ch)
            buffer.addFrom (ch, 0, wetBuf, ch, 0, n);

    playhead.store ((float) pos / (float) juce::jmax (1, capLen));
}

juce::AudioProcessorEditor* TruchoRiserProcessor::createEditor() { return new TruchoRiserEditor (*this); }

void TruchoRiserProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void TruchoRiserProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TruchoRiserProcessor(); }
