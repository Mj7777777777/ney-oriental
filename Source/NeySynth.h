// Ney Oriental - gestion des voix (mono lie / poly 4 voix) et surechantillonnage x2.
#pragma once
#include "NeyEngine.h"

namespace ney
{
// Decimateur x2 (FIR demi-bande, fenetre de Blackman).
class Decimator2
{
public:
    Decimator2()
    {
        const int m = kTaps / 2;
        double sum = 0.0;
        for (int i = 0; i < kTaps; ++i)
        {
            const int k = i - m;
            const double sinc = k == 0 ? 0.5 : std::sin (0.5 * kPi * k) / (kPi * k);
            const double win = 0.42 - 0.5 * std::cos (2.0 * kPi * i / (kTaps - 1)) + 0.08 * std::cos (4.0 * kPi * i / (kTaps - 1));
            h[(size_t) i] = (float) (sinc * win);
            sum += sinc * win;
        }
        for (auto& c : h) c = (float) (c / sum);
        reset();
    }
    void reset() noexcept { z.fill (0.0f); pos = 0; }
    float process (float a, float b) noexcept
    {
        z[(size_t) pos] = a; pos = (pos + 1) & (kSize - 1);
        z[(size_t) pos] = b; pos = (pos + 1) & (kSize - 1);
        float acc = 0.0f;
        int idx = pos - 1;
        for (int i = 0; i < kTaps; ++i) { acc += h[(size_t) i] * z[(size_t) (idx & (kSize - 1))]; --idx; }
        return acc;
    }
private:
    static constexpr int kTaps = 63, kSize = 64;
    std::array<float, kTaps> h {};
    std::array<float, kSize> z {};
    int pos = 0;
};

class Synth
{
public:
    void prepare (double hostSampleRate)
    {
        os = hostSampleRate < 100000.0 ? 2 : 1;
        for (auto& v : voices) v.prepare (hostSampleRate * os);
        dec.reset();
        held.clear(); held.reserve (32);
        sustained.clear(); sustained.reserve (32);
        counter = 0; pedal = false;
    }

    void setMono (bool shouldBeMono) noexcept
    {
        if (shouldBeMono != mono) { allNotesOff(); mono = shouldBeMono; }
    }

    void noteOn (int note, float vel) noexcept
    {
        vel = std::max (0.02f, std::min (1.0f, vel));
        removeFrom (sustained, note);
        if (mono)
        {
            const bool legato = voices[0].isGateOn();
            removeFrom (held, note);
            if (held.size() < 32) held.push_back (note);
            voices[0].noteOn (note, vel, legato);
            return;
        }
        Voice* target = nullptr;
        for (auto& v : voices) if (! v.isActive()) { target = &v; break; }
        if (target == nullptr)
            for (auto& v : voices) if (! v.isGateOn() && (target == nullptr || v.age < target->age)) target = &v;
        if (target == nullptr)
            for (auto& v : voices) if (target == nullptr || v.age < target->age) target = &v;
        target->age = ++counter;
        target->noteOn (note, vel, false);
    }

    void noteOff (int note) noexcept
    {
        if (pedal) { if (sustained.size() < 32) sustained.push_back (note); return; }
        release (note);
    }

    void setSustainPedal (bool down) noexcept
    {
        pedal = down;
        if (! down)
        {
            for (size_t i = 0; i < sustained.size(); ++i) release (sustained[i]);
            sustained.clear();
        }
    }

    void allNotesOff() noexcept
    {
        held.clear(); sustained.clear(); pedal = false;
        for (auto& v : voices) v.noteOff();
    }

    // rend n echantillons mono au taux de l'hote
    void render (float* out, int n, const Params& p) noexcept
    {
        for (int i = 0; i < n; ++i)
        {
            if (os == 2)
            {
                float a = 0.0f, b = 0.0f;
                for (auto& v : voices) { a += v.tick (p); b += v.tick (p); }
                out[i] = dec.process (a, b);
            }
            else
            {
                float a = 0.0f;
                for (auto& v : voices) a += v.tick (p);
                out[i] = a;
            }
        }
    }

private:
    static void removeFrom (std::vector<int>& list, int note) noexcept
    {
        list.erase (std::remove (list.begin(), list.end(), note), list.end());
    }

    void release (int note) noexcept
    {
        if (mono)
        {
            const bool wasCurrent = ! held.empty() && held.back() == note;
            removeFrom (held, note);
            if (held.empty()) { if (voices[0].note() == note || wasCurrent) voices[0].noteOff(); }
            else if (wasCurrent) voices[0].noteOn (held.back(), voices[0].lastVelocity(), true);
            return;
        }
        for (auto& v : voices) if (v.isGateOn() && v.note() == note) v.noteOff();
    }

    std::array<Voice, kMaxVoices> voices;
    Decimator2 dec;
    std::vector<int> held, sustained;
    uint32_t counter = 0;
    int os = 2;
    bool mono = true, pedal = false;
};
} // namespace ney
