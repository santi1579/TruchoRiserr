#include "PluginEditor.h"

using namespace TruchoColours;

//==============================================================================
void TruchoLnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                  float startA, float endA, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
    const auto c = b.getCentre();
    const float radius = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const float knobR = radius * 0.80f;

    // Resplandor azul
    g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.30f), c.x, c.y,
                                             accent.withAlpha (0.0f), c.x, c.y - radius * 1.15f, true));
    g.fillEllipse (c.x - radius * 1.15f, c.y - radius * 1.15f, radius * 2.3f, radius * 2.3f);

    // Puntos alrededor
    const int nDots = 28;
    for (int i = 0; i < nDots; ++i)
    {
        const float f = (float) i / (float) (nDots - 1);
        const float a = startA + f * (endA - startA);
        const float rr = radius * 0.94f;
        const float px = c.x + rr * std::sin (a);
        const float py = c.y - rr * std::cos (a);
        const bool lit = f <= pos + 0.0001f;
        const float s = lit ? 5.0f : 3.6f;
        g.setColour (lit ? glow : dim.withAlpha (0.55f));
        g.fillEllipse (px - s * 0.5f, py - s * 0.5f, s, s);
    }

    // Cuerpo del knob
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a3550), c.x, c.y - knobR,
                                             juce::Colour (0xff090d17), c.x, c.y + knobR, false));
    g.fillEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f);
    g.setColour (juce::Colour (0xff56648a));
    g.drawEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f, 2.5f);
    g.setColour (accent.withAlpha (0.30f));
    g.drawEllipse (c.x - knobR + 8.0f, c.y - knobR + 8.0f, knobR * 2.0f - 16.0f, knobR * 2.0f - 16.0f, 1.4f);

    // Indicador
    const float a = startA + pos * (endA - startA);
    juce::Path p;
    p.addRoundedRectangle (-2.5f, -knobR * 0.88f, 5.0f, knobR * 0.30f, 2.0f);
    g.setColour (text);
    g.fillPath (p, juce::AffineTransform::rotation (a).translated (c.x, c.y));
}

void TruchoLnF::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat();
    const bool on = b.getToggleState();
    const auto name = b.getName();

    if (name == "rawfx")
    {
        g.setFont (truchoFont (13.0f));
        auto left = r.removeFromLeft (r.getWidth() * 0.55f);
        g.setColour (on ? dim : text);
        g.drawText ("RAW", left, juce::Justification::centredRight);
        g.setColour (on ? accent : dim);
        g.drawText ("FX", r.withTrimmedLeft (6.0f), juce::Justification::centredLeft);
        return;
    }

    auto icon = r.removeFromLeft (20.0f);
    const float cx = icon.getCentreX(), cy = icon.getCentreY();
    const auto col = on ? glow : dim;
    g.setColour (col);

    juce::Path p;
    if (name == "listen")
    {
        p.addTriangle (cx - 4.0f, cy - 6.0f, cx - 4.0f, cy + 6.0f, cx + 6.0f, cy);
        g.setColour (on ? glow : text);
        g.fillPath (p);
    }
    else if (name == "loop")
    {
        p.addCentredArc (cx, cy, 6.0f, 6.0f, 0.0f, 0.5f, 5.6f, true);
        g.setColour (on ? glow : text);
        g.strokePath (p, juce::PathStrokeType (1.8f));
    }
    else
    {
        p.addCentredArc (cx, cy, 6.0f, 6.0f, 0.0f, 0.7f, juce::MathConstants<float>::twoPi - 0.7f, true);
        p.startNewSubPath (cx, cy - 7.0f);
        p.lineTo (cx, cy - 1.0f);
        g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setFont (truchoFont (12.5f));
    g.setColour (on ? text : (name == "listen" || name == "loop" ? text : dim));
    g.drawText (b.getButtonText().toUpperCase(), r.withTrimmedLeft (4.0f), juce::Justification::centredLeft);
}

void TruchoLnF::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    const bool isReset = b.getButtonText() == "Reset";
    g.setColour (isReset ? dim : accent);
    g.setFont (truchoFont (isReset ? 12.0f : 22.0f));
    g.drawText (isReset ? "RESET" : b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

//==============================================================================
TruchoRiserEditor::TruchoRiserEditor (TruchoRiserProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (520, 650);
    setLookAndFeel (&lnf);

    juce::Random r (7);
    for (int i = 0; i < 96; ++i) jitter.push_back (r.nextFloat());

    rise.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    rise.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    rise.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                              juce::MathConstants<float>::pi * 2.75f, true);
    rise.setDoubleClickReturnValue (true, 0.5);

    listen.setName ("listen");
    loop.setName ("loop");
    rawfx.setName ("rawfx");
    loop.setToggleState (true, juce::dontSendNotification);

    for (juce::Component* c : { (juce::Component*) &rise, (juce::Component*) &phone, (juce::Component*) &chorus,
                                (juce::Component*) &tremolo, (juce::Component*) &rawfx, (juce::Component*) &listen,
                                (juce::Component*) &loop, (juce::Component*) &prev, (juce::Component*) &next,
                                (juce::Component*) &resetBtn })
        addAndMakeVisible (c);

    riseA    = std::make_unique<SA> (proc.apvts, "rise", rise);
    phoneA   = std::make_unique<BA> (proc.apvts, "phone", phone);
    chorusA  = std::make_unique<BA> (proc.apvts, "chorus", chorus);
    tremoloA = std::make_unique<BA> (proc.apvts, "tremolo", tremolo);
    rawfxA   = std::make_unique<BA> (proc.apvts, "fx", rawfx);

    listen.onClick = [this] { proc.listenOn = listen.getToggleState(); };
    loop.onClick   = [this] { proc.loopOn = loop.getToggleState(); };
    prev.onClick   = [this] { stepType (-1); };
    next.onClick   = [this] { stepType (1); };
    resetBtn.onClick = [this]
    {
        for (auto* prm : proc.getParameters())
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->getDefaultValue());
            prm->endChangeGesture();
        }
        proc.listenOn = false;
    };

    startTimerHz (24);
}

TruchoRiserEditor::~TruchoRiserEditor() { setLookAndFeel (nullptr); }

void TruchoRiserEditor::stepType (int delta)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter ("type")))
    {
        const int n = p->choices.size();
        const int idx = (p->getIndex() + delta + n) % n;
        p->beginChangeGesture();
        *p = idx;
        p->endChangeGesture();
    }
}

void TruchoRiserEditor::timerCallback()
{
    listen.setToggleState (proc.listenOn.load(), juce::dontSendNotification);
    repaint();
}

void TruchoRiserEditor::resized()
{
    waveArea = { 100.0f, 80.0f, 320.0f, 110.0f };
    resetBtn.setBounds (410, 28, 80, 24);
    listen.setBounds (40, 214, 86, 26);
    loop.setBounds (130, 214, 74, 26);
    prev.setBounds (318, 212, 28, 30);
    next.setBounds (346, 212, 28, 30);
    rawfx.setBounds (390, 214, 100, 26);
    rise.setBounds (120, 252, 280, 280);
    phone.setBounds (90, 584, 100, 26);
    chorus.setBounds (210, 584, 100, 26);
    tremolo.setBounds (330, 584, 100, 26);
}

void TruchoRiserEditor::drawWaveform (juce::Graphics& g, juce::Rectangle<float> a)
{
    const float rs = proc.apvts.getRawParameterValue ("rise")->load();
    const float play = proc.playhead.load();
    const bool active = proc.listenOn.load();
    const int bars = 96;
    const float step = a.getWidth() / (float) bars;
    const float cy = a.getCentreY();

    g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.0f), a.getX(), cy,
                                             accent.withAlpha (0.20f), a.getRight(), cy, false));
    g.fillRoundedRectangle (a.expanded (0.0f, 10.0f), 30.0f);

    for (int k = 0; k < bars; ++k)
    {
        const float t = (float) k / (float) (bars - 1);
        const float h = (0.03f + 0.97f * std::pow (t, 1.7f) * (0.72f + 0.28f * jitter[(size_t) k]))
                        * (0.3f + 0.7f * rs) * a.getHeight();
        auto col = accent.interpolatedWith (glow, t).withAlpha (0.45f + 0.55f * t);
        if (active && t <= play) col = col.brighter (0.6f);
        g.setColour (col);
        g.fillRoundedRectangle (a.getX() + (float) k * step + step * 0.2f, cy - h * 0.5f, step * 0.6f, h, step * 0.3f);
    }

    if (active)
    {
        g.setColour (text.withAlpha (0.6f));
        const float px = a.getX() + play * a.getWidth();
        g.drawLine (px, a.getY(), px, a.getBottom(), 1.2f);
    }
}

void TruchoRiserEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg0);

    auto b = getLocalBounds().toFloat().reduced (10.0f);
    g.setGradientFill (juce::ColourGradient (bg1, b.getX(), b.getY(), bg0, b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 18.0f);
    g.setColour (accent.withAlpha (0.20f));
    g.drawRoundedRectangle (b, 18.0f, 1.2f);

    // Logo
    juce::AttributedString logo;
    logo.append ("TRUCHO", truchoFont (19.0f, false), text);
    logo.append ("RISER", truchoFont (19.0f, true), accent);
    logo.draw (g, juce::Rectangle<float> (34.0f, 26.0f, 240.0f, 28.0f));

    drawWaveform (g, waveArea);

    // Nombre del tipo de riser
    const auto name = TruchoRiserProcessor::typeNames()[(int) proc.apvts.getRawParameterValue ("type")->load()]
                          .toUpperCase();
    const auto first = name.upToFirstOccurrenceOf (" ", false, false);
    const auto rest  = name.fromFirstOccurrenceOf (" ", false, false);
    juce::AttributedString label;
    label.append (first + " ", truchoFont (20.0f, true), text);
    label.append (rest, truchoFont (20.0f, false), text.withAlpha (0.85f));
    label.setJustification (juce::Justification::centred);
    label.draw (g, juce::Rectangle<float> (0.0f, 536.0f, (float) getWidth(), 30.0f));

    g.setColour (dim.withAlpha (0.8f));
    g.setFont (truchoFont (13.0f, false));
    g.drawText ("TruchoRiser", getLocalBounds().withTrimmedTop (getHeight() - 36).toFloat(),
                juce::Justification::centred);
}
