#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
// Ecarts en cents par rapport au temperament egal, indexes par demi-tons au-dessus de la tonique.
const float kMaqams[4][12] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },          // Tempere
    { 0, 0, 0, 0, -50, 0, 0, 0, 0, 0, 0, -50 },      // Rast : tierce et septieme abaissees d'un quart de ton
    { 0, 0, -50, 0, 0, 0, 0, 0, 0, 0, 0, 0 },        // Bayati / Saba : seconde abaissee d'un quart de ton
    { 0, 20, 0, 0, -20, 0, 0, 0, 0, 0, 0, 0 },       // Hijaz : seconde augmentee resserree
};

struct Preset
{
    const char* name;
    float breath, noise, color, bright, overblow, chiff, vibRate, vibDepth, vibDelay, glide, revMix, revSize;
    bool poly;
};

const Preset kPresets[] = {
    { "Ney Classique",          0.60f, 0.45f, 0.50f, 0.45f, 0.10f, 0.50f, 5.2f, 0.30f, 0.30f, 0.07f, 0.25f, 0.55f, false },
    { "Ney Souffl\xc3\xa9",     0.50f, 0.85f, 0.60f, 0.35f, 0.00f, 0.70f, 4.8f, 0.25f, 0.35f, 0.08f, 0.30f, 0.60f, false },
    { "Kawala Brillant",        0.75f, 0.30f, 0.70f, 0.80f, 0.20f, 0.40f, 5.8f, 0.30f, 0.20f, 0.04f, 0.20f, 0.40f, false },
    { "Soufi Lent",             0.45f, 0.60f, 0.40f, 0.30f, 0.00f, 0.60f, 4.2f, 0.40f, 0.50f, 0.16f, 0.50f, 0.85f, false },
    { "D\xc3\xa9sert Cin\xc3\xa9matique", 0.55f, 0.60f, 0.45f, 0.40f, 0.10f, 0.50f, 4.6f, 0.35f, 0.40f, 0.12f, 0.65f, 0.95f, false },
    { "Ney Grave Sombre",       0.50f, 0.50f, 0.25f, 0.12f, 0.00f, 0.50f, 4.4f, 0.25f, 0.40f, 0.10f, 0.35f, 0.70f, false },
    { "Ney Turc",               0.65f, 0.55f, 0.55f, 0.50f, 0.30f, 0.60f, 5.5f, 0.45f, 0.20f, 0.09f, 0.30f, 0.60f, false },
    { "Lead Octavi\xc3\xa9",    0.90f, 0.30f, 0.60f, 0.70f, 1.00f, 0.50f, 5.6f, 0.30f, 0.25f, 0.03f, 0.25f, 0.50f, false },
    { "Trap Ney Court",         0.80f, 0.25f, 0.60f, 0.65f, 0.00f, 0.80f, 6.0f, 0.10f, 0.60f, 0.00f, 0.12f, 0.30f, false },
    { "Nappe Ambient",          0.45f, 0.70f, 0.40f, 0.30f, 0.00f, 0.20f, 4.5f, 0.30f, 0.50f, 0.12f, 0.80f, 1.00f, true  },
};
const int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));

// transparent sous 0.8, arrondit au-dela pour ne jamais ecreter durement
inline float softLimit (float x) noexcept
{
    const float a = std::abs (x);
    if (a <= 0.8f) return x;
    return std::copysign (0.8f + 0.2f * std::tanh ((a - 0.8f) * 5.0f), x);
}

const char* const kCentIds[12] = { "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "c10", "c11" };
}

juce::StringArray NeyOrientalProcessor::maqamNames()
{
    return { U ("Temp\xc3\xa9r\xc3\xa9"), "Rast", "Bayati / Saba", "Hijaz", U ("Personnalis\xc3\xa9") };
}

juce::StringArray NeyOrientalProcessor::rootNames()
{
    return { "Do", U ("Do#"), U ("R\xc3\xa9"), U ("Mib"), "Mi", "Fa", "Fa#", "Sol", "Lab", "La", "Sib", "Si" };
}

juce::AudioProcessorValueTreeState::ParameterLayout NeyOrientalProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using R = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto add = [&layout] (const char* id, const char* name, R range, float def)
    {
        layout.add (std::make_unique<F> (juce::ParameterID { id, 1 }, U (name), range, def));
    };
    add ("breath",   "Souffle",         R (0.0f, 1.0f), 0.60f);
    add ("noise",    "Bruit de souffle", R (0.0f, 1.0f), 0.45f);
    add ("color",    "Couleur du bruit", R (0.0f, 1.0f), 0.50f);
    add ("bright",   "Brillance",       R (0.0f, 1.0f), 0.45f);
    add ("overblow", "Overblow",        R (0.0f, 1.0f), 0.10f);
    add ("chiff",    "Attaque",         R (0.0f, 1.0f), 0.50f);
    add ("vibRate",  "Vibrato vitesse", R (3.0f, 8.0f), 5.2f);
    add ("vibDepth", "Vibrato",         R (0.0f, 1.0f), 0.30f);
    add ("vibDelay", "Vibrato d\xc3\xa9lai", R (0.0f, 1.0f), 0.30f);
    add ("glide",    "Glide",           R (0.0f, 0.3f), 0.07f);
    add ("revMix",   "Reverb",          R (0.0f, 1.0f), 0.25f);
    add ("revSize",  "Reverb taille",   R (0.0f, 1.0f), 0.55f);
    add ("gain",     "Volume",          R (-24.0f, 6.0f), 0.0f);
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "poly", 1 }, "Polyphonie", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "maqam", 1 }, "Maqam", maqamNames(), 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "root", 1 }, "Tonique", rootNames(), 2));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "bend", 1 }, "Pitch bend (demi-tons)", 1, 12, 2));
    const auto notes = rootNames();
    for (int i = 0; i < 12; ++i)
        layout.add (std::make_unique<F> (juce::ParameterID { kCentIds[i], 1 }, "Cents " + notes[i], R (-50.0f, 50.0f, 1.0f), 0.0f));
    return layout;
}

NeyOrientalProcessor::NeyOrientalProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "NEY", createLayout())
{
}

bool NeyOrientalProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void NeyOrientalProcessor::prepareToPlay (double sampleRate, int)
{
    fs = sampleRate;
    synth.prepare (sampleRate);
    reverb.setSampleRate (sampleRate);
    reverb.reset();
    gainSmooth.reset (sampleRate, 0.03);
    mixSmooth.reset (sampleRate, 0.05);
    gainSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (raw ("gain")));
    mixSmooth.setCurrentAndTargetValue (raw ("revMix"));
    shelfCoef = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 5000.0f / (float) sampleRate);
    shelfLp = 0.0f;
    modWheel = 0.0f; expression = 1.0f; pressure = 1.0f; bendNorm = 0.0f;
}

std::array<float, 12> NeyOrientalProcessor::getEffectiveCents() const
{
    std::array<float, 12> out {};
    const int maqam = (int) raw ("maqam");
    if (maqam >= kCustomMaqam)
    {
        for (int i = 0; i < 12; ++i) out[(size_t) i] = raw (kCentIds[i]);
        return out;
    }
    const int root = (int) raw ("root");
    for (int pc = 0; pc < 12; ++pc)
        out[(size_t) pc] = kMaqams[maqam][(pc - root + 12) % 12];
    return out;
}

void NeyOrientalProcessor::makeCustomFromCurrent()
{
    if ((int) raw ("maqam") >= kCustomMaqam) return;
    const auto current = getEffectiveCents();
    for (int i = 0; i < 12; ++i)
        if (auto* p = apvts.getParameter (kCentIds[i]))
            p->setValueNotifyingHost (p->convertTo0to1 (current[(size_t) i]));
    if (auto* p = apvts.getParameter ("maqam"))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) kCustomMaqam));
}

void NeyOrientalProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())                synth.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())          synth.noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())       bendNorm = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
    else if (m.isChannelPressure())  pressure = (float) m.getChannelPressureValue() / 127.0f;
    else if (m.isAllNotesOff() || m.isAllSoundOff()) synth.allNotesOff();
    else if (m.isController())
    {
        const float v = (float) m.getControllerValue() / 127.0f;
        switch (m.getControllerNumber())
        {
            case 1:  modWheel = v; break;
            case 2:  pressure = v; break;
            case 11: expression = v; break;
            case 64: synth.setSustainPedal (v >= 0.5f); break;
            default: break;
        }
    }
}

void NeyOrientalProcessor::renderChunk (float* left, float* right, int n)
{
    params.bendSemis = bendNorm * raw ("bend");
    params.pressureCC = pressure;
    params.vibDepth = juce::jlimit (0.0f, 1.0f, raw ("vibDepth") + modWheel);
    synth.render (mono.data(), n, params);

    // un peu d'air dans l'aigu, dose par la brillance
    const float air = -0.15f + 0.7f * params.brightness;
    const float level = raw ("poly") < 0.5f ? 0.42f : 0.17f;
    for (int i = 0; i < n; ++i)
    {
        shelfLp += (mono[(size_t) i] - shelfLp) * shelfCoef;
        const float x = (mono[(size_t) i] + air * (mono[(size_t) i] - shelfLp)) * level;
        mono[(size_t) i] = x;
        wetL[(size_t) i] = x;
        wetR[(size_t) i] = x;
    }
    reverb.processStereo (wetL.data(), wetR.data(), n);

    for (int i = 0; i < n; ++i)
    {
        const float g = gainSmooth.getNextValue() * expression;
        const float mix = mixSmooth.getNextValue();
        const float dry = mono[(size_t) i] * (1.0f - 0.35f * mix);
        const float l = softLimit ((dry + 0.55f * mix * wetL[(size_t) i]) * g);
        const float r = softLimit ((dry + 0.55f * mix * wetR[(size_t) i]) * g);
        left[i] = l;
        if (right != nullptr) right[i] = r; else left[i] = 0.5f * (l + r);
    }
}

void NeyOrientalProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);
    buffer.clear();
    if (buffer.getNumChannels() == 0) return;

    // parametres (lus une fois par bloc, lisses dans le moteur)
    params.breath = raw ("breath");
    params.noise = raw ("noise");
    params.noiseColor = raw ("color");
    params.brightness = raw ("bright");
    params.overblow = raw ("overblow");
    params.chiff = raw ("chiff");
    params.vibRate = raw ("vibRate");
    params.vibDelay = raw ("vibDelay");
    params.glide = raw ("glide");
    params.cents = getEffectiveCents();
    synth.setMono (raw ("poly") < 0.5f);
    gainSmooth.setTargetValue (juce::Decibels::decibelsToGain (raw ("gain")));
    mixSmooth.setTargetValue (raw ("revMix"));

    juce::Reverb::Parameters rp;
    const float size = raw ("revSize");
    rp.roomSize = 0.35f + 0.63f * size;
    rp.damping = 0.65f - 0.3f * size;
    rp.wetLevel = 1.0f / 3.0f;
    rp.dryLevel = 0.0f;
    rp.width = 1.0f;
    reverb.setParameters (rp);

    float* left = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    int pos = 0;
    auto renderUntil = [&] (int end)
    {
        while (pos < end)
        {
            const int n = juce::jmin (kChunk, end - pos);
            renderChunk (left + pos, right != nullptr ? right + pos : nullptr, n);
            pos += n;
        }
    };

    for (const auto meta : midi)
    {
        renderUntil (juce::jlimit (pos, numSamples, meta.samplePosition));
        handleMidi (meta.getMessage());
    }
    renderUntil (numSamples);
}

// ---------------------------------------------------------------- presets
int NeyOrientalProcessor::getNumPrograms() { return kNumPresets; }

const juce::String NeyOrientalProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, kNumPresets) ? U (kPresets[index].name) : juce::String();
}

void NeyOrientalProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, kNumPresets)) return;
    currentProgram = index;
    const auto& p = kPresets[index];
    auto set = [this] (const char* id, float value)
    {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    set ("breath", p.breath);   set ("noise", p.noise);       set ("color", p.color);
    set ("bright", p.bright);   set ("overblow", p.overblow); set ("chiff", p.chiff);
    set ("vibRate", p.vibRate); set ("vibDepth", p.vibDepth); set ("vibDelay", p.vibDelay);
    set ("glide", p.glide);     set ("revMix", p.revMix);     set ("revSize", p.revSize);
    set ("poly", p.poly ? 1.0f : 0.0f);
}

// ---------------------------------------------------------------- etat
void NeyOrientalProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void NeyOrientalProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (state.isValid() && state.hasType (apvts.state.getType()))
        {
            currentProgram = juce::jlimit (0, kNumPresets - 1, (int) state.getProperty ("program", 0));
            apvts.replaceState (state);
        }
    }
}

juce::AudioProcessorEditor* NeyOrientalProcessor::createEditor() { return new NeyOrientalEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NeyOrientalProcessor(); }
