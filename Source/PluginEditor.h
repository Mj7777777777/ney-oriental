#pragma once
#include "PluginProcessor.h"

class NeyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeyLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool, int, int, int, int, juce::ComboBox&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;

    static const juce::Colour bg, panel, brass, sand, dim, ember;
};

// Affiche et edite l'accordage des 12 notes (en cents).
class CentsDisplay : public juce::Component
{
public:
    explicit CentsDisplay (NeyOrientalProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void refresh();
private:
    int noteAt (float x) const;
    void setCent (int note, float cents);
    NeyOrientalProcessor& proc;
    std::array<float, 12> shown {};
    int dragNote = -1;
    float dragStartY = 0.0f, dragStartValue = 0.0f;
};

class NeyOrientalEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NeyOrientalEditor (NeyOrientalProcessor&);
    ~NeyOrientalEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    void addKnob (Knob&, const char* paramId, const char* text, bool big);

    struct Content : public juce::Component
    {
        void paint (juce::Graphics&) override;
    };

    static constexpr int kW = 880, kH = 600;
    NeyOrientalProcessor& proc;
    NeyLookAndFeel lnf;
    Content content;

    Knob breath, noise, bright, vibrato, reverb;
    Knob color, overblow, chiff, vibRate, vibDelay, glide, revSize, gain;
    juce::ComboBox presetBox, maqamBox, rootBox, bendBox;
    juce::ToggleButton polyButton;
    juce::Label presetLabel, maqamLabel, rootLabel, bendLabel;
    CentsDisplay cents;
    juce::MidiKeyboardComponent keyboard;

    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboAttachment> maqamAttachment, rootAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> polyAttachment;
    std::unique_ptr<juce::ParameterAttachment> bendAttachment;
    int lastProgram = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeyOrientalEditor)
};
