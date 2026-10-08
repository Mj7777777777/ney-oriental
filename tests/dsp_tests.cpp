// Tests du moteur sonore, sans JUCE :  g++ -std=c++20 -O2 -I Source tests/dsp_tests.cpp -o dsp_tests && ./dsp_tests
#include "NeySynth.h"
#include <cstdio>

static int failures = 0;
static void check (bool ok, const char* what) { std::printf ("%s  %s\n", ok ? "ok  " : "FAIL", what); if (! ok) ++failures; }

// frequence par passages a zero d'un signal filtre autour de la fondamentale
static double measureHz (const std::vector<float>& x, double fs, double expected)
{
    const double c = 1.0 - std::exp (-2.0 * ney::kPi * expected * 1.3 / fs);
    double a = 0, b = 0, d = 0, prev = 0, first = -1, last = -1; int count = 0;
    for (size_t i = 0; i < x.size(); ++i)
    {
        a += (x[i] - a) * c; b += (a - b) * c; d += (b - d) * 0.0005; const double y = b - d;
        if (i > x.size() / 3 && prev <= 0.0 && y > 0.0)
        {
            const double t = (double) i - y / (y - prev);
            if (first < 0) first = t; else ++count;
            last = t;
        }
        prev = y;
    }
    return count > 0 ? fs * count / (last - first) : 0.0;
}

int main()
{
    for (double fs : { 44100.0, 48000.0, 96000.0 })
    {
        double worst = 0.0; bool finite = true;
        for (int note = 48; note <= 84; note += 4)
        {
            ney::Synth s; s.prepare (fs);
            ney::Params p; p.vibDepth = 0; p.chiff = 0; p.humanize = 0; p.noise = 0.2f;
            std::vector<float> out ((size_t) (fs * 1.5));
            s.noteOn (note, 0.8f);
            s.render (out.data(), (int) out.size(), p);
            for (float v : out) finite = finite && std::isfinite (v) && std::abs (v) < 4.0f;
            const double hz = measureHz (out, fs, ney::midiToHz ((float) note));
            worst = std::max (worst, std::abs (1200.0 * std::log2 (hz / ney::midiToHz ((float) note))));
        }
        char msg[96]; std::snprintf (msg, sizeof (msg), "justesse a %.0f Hz : ecart max %.1f cents (< 8)", fs, worst);
        check (worst < 8.0, msg);
        check (finite, "sortie finie et bornee");
    }
    {   // quart de ton : -50 cents sur le mi
        ney::Synth s; s.prepare (48000.0);
        ney::Params p; p.vibDepth = 0; p.chiff = 0; p.humanize = 0; p.noise = 0; p.cents[4] = -50.0f;
        std::vector<float> out (72000);
        s.noteOn (64, 0.8f); s.render (out.data(), (int) out.size(), p);
        const double cents = 1200.0 * std::log2 (measureHz (out, 48000.0, 320.0) / ney::midiToHz (64.0f));
        check (std::abs (cents + 50.0) < 8.0, "quart de ton (-50 cents) respecte");
    }
    {   // silence apres relachement, legato mono
        ney::Synth s; s.prepare (44100.0);
        ney::Params p;
        std::vector<float> out (44100);
        s.noteOn (60, 0.9f); s.render (out.data(), 22050, p);
        s.noteOn (64, 0.9f); s.render (out.data(), 22050, p);
        s.noteOff (64);      s.render (out.data(), 22050, p);   // retour lie sur 60
        float peak = 0; for (int i = 11000; i < 22050; ++i) peak = std::max (peak, std::abs (out[(size_t) i]));
        check (peak > 0.02f, "mono : la note tenue reprend quand on relache la suivante");
        s.noteOff (60);      s.render (out.data(), 44100, p); s.render (out.data(), 44100, p);
        peak = 0; for (float v : out) peak = std::max (peak, std::abs (v));
        check (peak < 1.0e-3f, "silence apres relachement");
    }
    {   // polyphonie : 6 notes sur 4 voix sans debordement
        ney::Synth s; s.prepare (44100.0); s.setMono (false);
        ney::Params p; std::vector<float> out (44100); bool ok = true;
        for (int n : { 60, 63, 67, 70, 72, 75 }) s.noteOn (n, 1.0f);
        s.render (out.data(), 44100, p);
        for (float v : out) ok = ok && std::isfinite (v) && std::abs (v) < 8.0f;
        check (ok, "polyphonie stable");
    }
    std::printf (failures == 0 ? "\nTous les tests passent.\n" : "\n%d test(s) en echec.\n", failures);
    return failures == 0 ? 0 : 1;
}
