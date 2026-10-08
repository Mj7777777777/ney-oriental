// Rend une phrase de demonstration (mono, WAV 16 bit) avec le moteur seul.
#include "../Source/NeySynth.h"
#include <cstdio>
#include <cstring>
#include <string>
struct Ev { double t; int note; float vel; bool on; };
int main (int argc, char** argv)
{
    const double fs = argc > 2 ? atof (argv[2]) : 44100.0;
    ney::Synth s; s.prepare (fs);
    ney::Params p; p.breath = 0.6f; p.noise = 0.5f; p.brightness = 0.45f; p.vibDepth = 0.3f; p.glide = 0.07f; p.chiff = 0.6f; p.overblow = 0.15f;
    // Bayati sur re : mi demi-bemol
    p.cents = { 0, 0, 0, 0, -50, 0, 0, 0, 0, 0, 0, 0 };
    std::vector<Ev> ev; double t = 0.2;
    auto N = [&] (int n, double d, float v, double gap) { ev.push_back ({ t, n, v, true }); ev.push_back ({ t + d, n, 0, false }); t += d + gap; };
    N (62, 1.2, 0.7f, -0.05); N (64, 0.3, 0.6f, -0.05); N (65, 0.3, 0.6f, -0.05); N (67, 1.0, 0.8f, 0.25);
    N (69, 0.25, 0.9f, -0.04); N (67, 0.25, 0.6f, -0.04); N (65, 0.25, 0.6f, -0.04); N (64, 0.5, 0.6f, -0.04); N (62, 1.6, 0.7f, 0.4);
    N (74, 0.9, 1.0f, 0.1); N (72, 0.3, 0.7f, -0.04); N (70, 0.3, 0.6f, -0.04); N (69, 1.4, 0.75f, 0.3);
    const int n = (int) ((t + 1.0) * fs);
    std::vector<float> out ((size_t) n);
    std::sort (ev.begin(), ev.end(), [] (const Ev& a, const Ev& b) { return a.t < b.t || (a.t == b.t && a.on && ! b.on); });
    size_t e = 0;
    for (int i = 0; i < n; i += 32)
    {
        while (e < ev.size() && ev[e].t * fs <= i) { if (ev[e].on) s.noteOn (ev[e].note, ev[e].vel); else s.noteOff (ev[e].note); ++e; }
        s.render (out.data() + i, std::min (32, n - i), p);
    }
    FILE* f = fopen (argv[1], "wb");
    const int sr = (int) fs, bytes = n * 2;
    auto w32 = [&] (int v) { fwrite (&v, 4, 1, f); }; auto w16 = [&] (short v) { fwrite (&v, 2, 1, f); };
    fwrite ("RIFF", 1, 4, f); w32 (36 + bytes); fwrite ("WAVEfmt ", 1, 8, f); w32 (16); w16 (1); w16 (1); w32 (sr); w32 (sr * 2); w16 (2); w16 (16);
    fwrite ("data", 1, 4, f); w32 (bytes);
    float pk = 0; for (float x : out) pk = std::max (pk, std::abs (x));
    for (float x : out) w16 ((short) (x / std::max (pk, 1e-6f) * 0.89f * 32767.0f));
    fclose (f);
    printf ("peak %.3f  samples %d\n", pk, n);
}
