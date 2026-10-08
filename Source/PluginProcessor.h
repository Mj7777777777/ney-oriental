#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "NeySynth.h"

inline juce::String U (const char* utf8) { return juce::String::fromUTF8 (utf8); }

class NeyOrientalProcessor : public juce::AudioProcessor
{
public:
    NeyOrientalProcessor();
    ~NeyOrientalProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Ney Oriental"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- accordage ---
    static juce::StringArray maqamNames();
    static juce::StringArray rootNames();
    static constexpr int kCustomMaqam = 4;
    std::array<float, 12> getEffectiveCents() const;   // par touche, do..si
    void makeCustomFromCurrent();                      // fil d'interface uniquement

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void handleMidi (const juce::MidiMessage&);
    void renderChunk (float* left, float* right, int numSamples);
    float raw (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    ney::Synth synth;
    ney::Params params;
    juce::Reverb reverb;
    juce::SmoothedValue<float> gainSmooth, mixSmooth;

    static constexpr int kChunk = 256;
    std::array<float, kChunk> mono {}, wetL {}, wetR {};
    float shelfLp = 0.0f, shelfCoef = 0.3f;
    float modWheel = 0.0f, expression = 1.0f, pressure = 1.0f, bendNorm = 0.0f;
    double fs = 44100.0;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeyOrientalProcessor)
};
