#include "PluginEditor.h"

const juce::Colour NeyLookAndFeel::bg    { 0xff14100c };
const juce::Colour NeyLookAndFeel::panel { 0xff221a13 };
const juce::Colour NeyLookAndFeel::brass { 0xffc9a25a };
const juce::Colour NeyLookAndFeel::sand  { 0xffe9dcc3 };
const juce::Colour NeyLookAndFeel::dim   { 0xff8a7a63 };
const juce::Colour NeyLookAndFeel::ember { 0xffd9703a };

namespace
{
juce::Font uiFont (float height, bool bold = false)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}
const char* const kCentIds[12] = { "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "c10", "c11" };
}

// ------------------------------------------------------------ look and feel
NeyLookAndFeel::NeyLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, panel);
    setColour (juce::ComboBox::textColourId, sand);
    setColour (juce::ComboBox::outlineColourId, brass.withAlpha (0.35f));
    setColour (juce::ComboBox::arrowColourId, brass);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, sand);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, brass);
    setColour (juce::PopupMenu::highlightedTextColourId, bg);
    setColour (juce::Label::textColourId, dim);
    setColour (juce::Slider::textBoxTextColourId, sand);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ToggleButton::textColourId, sand);
}

void NeyLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                                       float startAngle, float endAngle, juce::Slider& slider)
{
    const auto area = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
    const float radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
    const auto centre = area.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float track = juce::jmax (2.5f, radius * 0.09f);

    juce::Path back;
    back.addCentredArc (centre.x, centre.y, radius - track, radius - track, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff3a2d20));
    g.strokePath (back, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, radius - track, radius - track, 0.0f, startAngle, angle, true);
    g.setColour (slider.isMouseOverOrDragging() ? ember : brass);
    g.strokePath (value, juce::PathStrokeType (track, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float inner = radius - track * 2.6f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a3a29), centre.x, centre.y - inner,
                                             juce::Colour (0xff1c150f), centre.x, centre.y + inner, false));
    g.fillEllipse (centre.x - inner, centre.y - inner, inner * 2.0f, inner * 2.0f);
    g.setColour (brass.withAlpha (0.25f));
    g.drawEllipse (centre.x - inner, centre.y - inner, inner * 2.0f, inner * 2.0f, 1.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-track * 0.4f, -inner * 0.92f, track * 0.8f, inner * 0.42f, track * 0.4f);
    g.setColour (sand);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

void NeyLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    juce::Path arrow;
    const float cx = (float) width - 14.0f, cy = (float) height * 0.5f;
    arrow.addTriangle (cx - 4.5f, cy - 2.5f, cx + 4.5f, cy - 2.5f, cx, cy + 3.5f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

void NeyLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    g.setColour (on ? brass : panel);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (brass.withAlpha (over ? 0.8f : 0.35f));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    g.setColour (on ? bg : sand);
    g.setFont (uiFont (13.0f, true));
    g.drawText (on ? "POLY 4 VOIX" : "MONO LEGATO", b.getLocalBounds(), juce::Justification::centred);
}

juce::Font NeyLookAndFeel::getComboBoxFont (juce::ComboBox&) { return uiFont (14.0f); }
juce::Font NeyLookAndFeel::getPopupMenuFont() { return uiFont (14.0f); }

// ------------------------------------------------------------ accordage
void CentsDisplay::refresh()
{
    const auto now = proc.getEffectiveCents();
    if (now != shown) { shown = now; repaint(); }
}

int CentsDisplay::noteAt (float x) const
{
    return juce::jlimit (0, 11, (int) (x / ((float) getWidth() / 12.0f)));
}

void CentsDisplay::setCent (int note, float value)
{
    if (auto* p = proc.apvts.getParameter (kCentIds[note]))
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (-50.0f, 50.0f, std::round (value))));
}

void CentsDisplay::mouseDown (const juce::MouseEvent& e)
{
    proc.makeCustomFromCurrent();
    dragNote = noteAt (e.position.x);
    dragStartY = e.position.y;
    dragStartValue = proc.getEffectiveCents()[(size_t) dragNote];
    if (auto* p = proc.apvts.getParameter (kCentIds[dragNote])) p->beginChangeGesture();
}

void CentsDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (dragNote < 0) return;
    const float range = (float) getHeight() - 34.0f;
    setCent (dragNote, dragStartValue + (dragStartY - e.position.y) * 100.0f / range);
    refresh();
}

void CentsDisplay::mouseUp (const juce::MouseEvent&)
{
    if (dragNote >= 0)
        if (auto* p = proc.apvts.getParameter (kCentIds[dragNote])) p->endChangeGesture();
    dragNote = -1;
}

void CentsDisplay::mouseDoubleClick (const juce::MouseEvent& e)
{
    proc.makeCustomFromCurrent();
    setCent (noteAt (e.position.x), 0.0f);
    refresh();
}

void CentsDisplay::paint (juce::Graphics& g)
{
    const auto names = NeyOrientalProcessor::rootNames();
    const float w = (float) getWidth() / 12.0f;
    const float top = 16.0f, bottom = (float) getHeight() - 18.0f, mid = (top + bottom) * 0.5f;
    g.setColour (NeyLookAndFeel::panel);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
    g.setColour (NeyLookAndFeel::dim.withAlpha (0.35f));
    g.drawHorizontalLine ((int) mid, 6.0f, (float) getWidth() - 6.0f);

    for (int i = 0; i < 12; ++i)
    {
        const float c = shown[(size_t) i];
        const float x = (float) i * w;
        const float h = (c / 50.0f) * (mid - top);
        const bool altered = std::abs (c) > 0.5f;
        g.setColour (altered ? NeyLookAndFeel::ember : NeyLookAndFeel::brass.withAlpha (0.45f));
        if (altered)
            g.fillRoundedRectangle (juce::Rectangle<float> (x + w * 0.3f, juce::jmin (mid, mid - h), w * 0.4f, std::abs (h)), 2.0f);
        g.fillEllipse (x + w * 0.5f - 3.5f, mid - h - 3.5f, 7.0f, 7.0f);

        g.setFont (uiFont (11.0f, true));
        g.setColour (altered ? NeyLookAndFeel::sand : NeyLookAndFeel::dim);
        g.drawText (names[i], juce::Rectangle<float> (x, bottom + 2.0f, w, 14.0f), juce::Justification::centred);
        if (altered)
        {
            g.setFont (uiFont (10.5f));
            g.drawText ((c > 0 ? "+" : "") + juce::String ((int) c), juce::Rectangle<float> (x, 2.0f, w, 13.0f), juce::Justification::centred);
        }
    }
}

// ------------------------------------------------------------ editeur
void NeyOrientalEditor::Content::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a1e14), 0.0f, 0.0f, NeyLookAndFeel::bg, 0.0f, b.getHeight(), false));
    g.fillAll();
    g.setGradientFill (juce::ColourGradient (NeyLookAndFeel::ember.withAlpha (0.16f), b.getWidth() * 0.5f, 150.0f,
                                             juce::Colours::transparentBlack, b.getWidth() * 0.5f, 520.0f, true));
    g.fillAll();

    // titre
    g.setColour (NeyLookAndFeel::sand);
    g.setFont (uiFont (27.0f, true).withExtraKerningFactor (0.22f));
    g.drawText ("NEY ORIENTAL", 28, 16, 320, 34, juce::Justification::centredLeft);
    g.setColour (NeyLookAndFeel::dim);
    g.setFont (uiFont (11.5f).withExtraKerningFactor (0.2f));
    g.drawText (U ("FL\xc3\x9bTE DE ROSEAU  \xc2\xb7  MOD\xc3\x88LE PHYSIQUE  \xc2\xb7  MJ7"), 30, 48, 420, 16, juce::Justification::centredLeft);

    // le ney : un roseau et ses trous
    const float y = 84.0f;
    g.setGradientFill (juce::ColourGradient (NeyLookAndFeel::brass, 28.0f, y, juce::Colour (0xff7a5a2c), 852.0f, y, false));
    g.fillRoundedRectangle (28.0f, y - 3.0f, 824.0f, 6.0f, 3.0f);
    g.setColour (NeyLookAndFeel::bg);
    for (int i = 0; i < 9; ++i) g.fillRect (28.0f + 91.5f * (float) i, y - 3.0f, 1.5f, 6.0f);
    for (int i = 0; i < 6; ++i) g.fillEllipse (420.0f + 58.0f * (float) i, y - 1.8f, 3.6f, 3.6f);

    // panneaux
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillRoundedRectangle (20.0f, 102.0f, 840.0f, 162.0f, 10.0f);
    g.fillRoundedRectangle (20.0f, 274.0f, 840.0f, 112.0f, 10.0f);
    g.fillRoundedRectangle (20.0f, 396.0f, 840.0f, 104.0f, 10.0f);
}

void NeyOrientalEditor::addKnob (Knob& k, const char* paramId, const char* text, bool big)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setPopupDisplayEnabled (true, true, this);
    k.slider.setNumDecimalPlacesToDisplay (2);
    content.addAndMakeVisible (k.slider);
    k.label.setText (U (text), juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (uiFont (big ? 13.0f : 11.5f, true).withExtraKerningFactor (0.12f));
    k.label.setColour (juce::Label::textColourId, big ? NeyLookAndFeel::sand : NeyLookAndFeel::dim);
    content.addAndMakeVisible (k.label);
    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, k.slider);
}

NeyOrientalEditor::NeyOrientalEditor (NeyOrientalProcessor& p)
    : AudioProcessorEditor (&p), proc (p), cents (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);

    addKnob (breath,  "breath",   "SOUFFLE",   true);
    addKnob (noise,   "noise",    "BRUIT",     true);
    addKnob (bright,  "bright",   "BRILLANCE", true);
    addKnob (vibrato, "vibDepth", "VIBRATO",   true);
    addKnob (reverb,  "revMix",   "REVERB",    true);

    addKnob (color,    "color",    "COULEUR",    false);
    addKnob (overblow, "overblow", "OVERBLOW",   false);
    addKnob (chiff,    "chiff",    "ATTAQUE",    false);
    addKnob (vibRate,  "vibRate",  "VIB. VITESSE", false);
    addKnob (vibDelay, "vibDelay", "VIB. D\xc3\x89LAI", false);
    addKnob (glide,    "glide",    "GLIDE",      false);
    addKnob (revSize,  "revSize",  "TAILLE",     false);
    addKnob (gain,     "gain",     "VOLUME",     false);

    auto setupLabel = [this] (juce::Label& l, const char* text)
    {
        l.setText (U (text), juce::dontSendNotification);
        l.setFont (uiFont (11.0f, true).withExtraKerningFactor (0.14f));
        content.addAndMakeVisible (l);
    };
    setupLabel (presetLabel, "PRESET");
    setupLabel (maqamLabel, "MAQAM");
    setupLabel (rootLabel, "TONIQUE");
    setupLabel (bendLabel, "PITCH BEND");

    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.onChange = [this]
    {
        const int index = presetBox.getSelectedItemIndex();
        if (index >= 0 && index != proc.getCurrentProgram()) proc.setCurrentProgram (index);
        lastProgram = proc.getCurrentProgram();
    };
    content.addAndMakeVisible (presetBox);

    maqamBox.addItemList (NeyOrientalProcessor::maqamNames(), 1);
    rootBox.addItemList (NeyOrientalProcessor::rootNames(), 1);
    content.addAndMakeVisible (maqamBox);
    content.addAndMakeVisible (rootBox);
    maqamAttachment = std::make_unique<ComboAttachment> (proc.apvts, "maqam", maqamBox);
    rootAttachment = std::make_unique<ComboAttachment> (proc.apvts, "root", rootBox);

    for (int i = 1; i <= 12; ++i) bendBox.addItem (U ("\xc2\xb1 ") + juce::String (i) + (i == 1 ? " demi-ton" : " demi-tons"), i);
    content.addAndMakeVisible (bendBox);
    bendAttachment = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter ("bend"),
        [this] (float v) { bendBox.setSelectedId (juce::roundToInt (v), juce::dontSendNotification); });
    bendBox.onChange = [this] { bendAttachment->setValueAsCompleteGesture ((float) bendBox.getSelectedId()); };
    bendAttachment->sendInitialUpdate();

    content.addAndMakeVisible (polyButton);
    polyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "poly", polyButton);

    content.addAndMakeVisible (cents);
    keyboard.setAvailableRange (48, 84);
    keyboard.setKeyWidth (840.0f / 22.0f);
    keyboard.setOctaveForMiddleC (5); // comme FL Studio : do central = C5
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, NeyLookAndFeel::sand);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff2a1e14));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, NeyLookAndFeel::ember.withAlpha (0.75f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, NeyLookAndFeel::brass.withAlpha (0.35f));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff6b5a44));
    content.addAndMakeVisible (keyboard);

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) kW / (double) kH);
    setResizeLimits (660, 450, 1760, 1200);
    setSize (kW, kH);
    startTimerHz (20);
    timerCallback();
}

NeyOrientalEditor::~NeyOrientalEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void NeyOrientalEditor::timerCallback()
{
    cents.refresh();
    if (lastProgram != proc.getCurrentProgram())
    {
        lastProgram = proc.getCurrentProgram();
        presetBox.setSelectedItemIndex (lastProgram, juce::dontSendNotification);
    }
}

void NeyOrientalEditor::paint (juce::Graphics& g) { g.fillAll (NeyLookAndFeel::bg); }

void NeyOrientalEditor::resized()
{
    content.setBounds (0, 0, kW, kH);
    content.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) kW));

    presetLabel.setBounds (520, 14, 120, 14);
    presetBox.setBounds (520, 30, 196, 28);
    polyButton.setBounds (728, 30, 124, 28);

    // grands boutons
    Knob* big[] = { &breath, &noise, &bright, &vibrato, &reverb };
    for (int i = 0; i < 5; ++i)
    {
        const int x = 20 + i * 168;
        big[i]->slider.setBounds (x + 34, 110, 100, 122);
        big[i]->label.setBounds (x, 236, 168, 18);
    }
    // petits boutons
    Knob* small[] = { &color, &overblow, &chiff, &vibRate, &vibDelay, &glide, &revSize, &gain };
    for (int i = 0; i < 8; ++i)
    {
        const int x = 20 + i * 105;
        small[i]->slider.setBounds (x + 22, 282, 61, 72);
        small[i]->label.setBounds (x, 358, 105, 16);
    }
    // accordage
    maqamLabel.setBounds (34, 404, 150, 14);
    maqamBox.setBounds (34, 420, 150, 26);
    rootLabel.setBounds (34, 450, 70, 14);
    rootBox.setBounds (34, 466, 70, 26);
    bendLabel.setBounds (112, 450, 130, 14);
    bendBox.setBounds (112, 466, 130, 26);
    cents.setBounds (258, 404, 590, 88);

    keyboard.setBounds (20, 510, 840, 78);
}
