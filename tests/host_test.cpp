// Test d'integration : charge le plugin comme le ferait un hote, joue des notes, capture l'interface.
#include "PluginProcessor.h"
#include "PluginEditor.h"

static float sustainedPeak (float revMix, float velocity, bool poly)
{
    NeyOrientalProcessor plugin;
    plugin.setPlayConfigDetails (0, 2, 48000.0, 512);
    plugin.apvts.getParameter ("revMix")->setValueNotifyingHost (revMix);
    plugin.apvts.getParameter ("revSize")->setValueNotifyingHost (1.0f);
    plugin.apvts.getParameter ("poly")->setValueNotifyingHost (poly ? 1.0f : 0.0f);
    plugin.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> buffer (2, 512);
    float peak = 0.0f;
    for (int block = 0; block < 400; ++block)
    {
        juce::MidiBuffer midi;
        if (block == 0)
            for (int n : { 62, 65, 69, 72 }) { midi.addEvent (juce::MidiMessage::noteOn (1, n, velocity), 0); if (! poly) break; }
        plugin.processBlock (buffer, midi);
        peak = juce::jmax (peak, buffer.getMagnitude (0, 512));
    }
    return peak;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    int failures = 0;
    auto check = [&failures] (bool ok, const char* what) { std::printf ("%s  %s\n", ok ? "ok  " : "FAIL", what); if (! ok) ++failures; };

    const float dryPeak = sustainedPeak (0.0f, 1.0f, false), wetPeak = sustainedPeak (1.0f, 1.0f, false), polyPeak = sustainedPeak (0.5f, 1.0f, true);
    std::printf ("niveaux max : sec %.3f, reverb a fond %.3f, accord 4 voix %.3f\n", dryPeak, wetPeak, polyPeak);
    check (dryPeak > 0.2f && dryPeak < 0.95f && wetPeak < 0.95f && polyPeak < 1.0f, "niveaux de sortie sans saturation");

    std::unique_ptr<juce::AudioProcessor> plugin = std::make_unique<NeyOrientalProcessor>();
    plugin->setPlayConfigDetails (0, 2, 48000.0, 512);
    plugin->prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buffer (2, 512);
    float peak = 0.0f, tail = 0.0f; bool finite = true;
    for (int block = 0; block < 600; ++block)
    {
        juce::MidiBuffer midi;
        if (block == 2)   midi.addEvent (juce::MidiMessage::noteOn (1, 62, 0.8f), 17);
        if (block == 60)  midi.addEvent (juce::MidiMessage::noteOn (1, 65, 0.9f), 100);
        if (block == 62)  midi.addEvent (juce::MidiMessage::noteOff (1, 62), 3);
        if (block == 90)  midi.addEvent (juce::MidiMessage::pitchWheel (1, 12000), 0);
        if (block == 120) midi.addEvent (juce::MidiMessage::noteOff (1, 65), 500);
        if (block == 20)  plugin->setCurrentProgram (4);
        plugin->processBlock (buffer, midi);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 512; ++i)
            {
                const float v = buffer.getSample (ch, i);
                finite = finite && std::isfinite (v);
                if (block < 120) peak = juce::jmax (peak, std::abs (v));
                if (block > 590) tail = juce::jmax (tail, std::abs (v));
            }
    }
    std::printf ("niveau max %.3f, queue apres 5 s %.6f\n", peak, tail);
    check (finite, "sortie finie");
    check (peak > 0.05f && peak < 1.0f, "le plugin produit du son");
    check (tail < 0.01f, "la reverb s'eteint apres la derniere note");

    juce::MemoryBlock state;
    plugin->getStateInformation (state);
    plugin->setCurrentProgram (8);
    plugin->setStateInformation (state.getData(), (int) state.getSize());
    check (plugin->getCurrentProgram() == 4, "sauvegarde et rechargement de l'etat");

    std::unique_ptr<juce::AudioProcessorEditor> editor (plugin->createEditor());
    check (editor != nullptr && editor->getWidth() == 880, "l'interface se cree");
    if (editor != nullptr && argc > 1)
    {
        for (int pass = 0; pass < 2; ++pass)
        {
            if (pass == 1)
            {
                if (auto* p = dynamic_cast<NeyOrientalProcessor*> (plugin.get()))
                    p->apvts.getParameter ("maqam")->setValueNotifyingHost (0.25f); // Rast
                juce::MessageManager::getInstance()->runDispatchLoopUntil (200);
                editor->setSize (1100, 750);
            }
            const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
            juce::File file (juce::String (argv[1]) + (pass == 0 ? "_1.png" : "_2.png"));
            file.deleteFile();
            juce::FileOutputStream out (file);
            juce::PNGImageFormat().writeImageToStream (image, out);
        }
    }
    editor.reset();
    plugin->releaseResources();
    std::printf (failures == 0 ? "\nTous les tests passent.\n" : "\n%d test(s) en echec.\n", failures);
    return failures == 0 ? 0 : 1;
}
