// Ney Oriental - moteur sonore (modele physique de flute a embouchure libre).
// C++ pur, sans dependance JUCE : peut etre teste hors plugin.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include "NeyTuning.h"

namespace ney
{
constexpr double kPi = 3.14159265358979323846;
constexpr int kMaxVoices = 4;

struct Rng
{
    uint32_t s = 0x2545F491u;
    float next() noexcept // bruit blanc [-1, 1]
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (float) (int32_t) s * (1.0f / 2147483648.0f);
    }
};

// Ligne a retard a lecture fractionnaire (Lagrange ordre 3).
class DelayLine
{
public:
    void resize (int minSize)
    {
        int n = 16;
        while (n < minSize) n <<= 1;
        buf.assign ((size_t) n, 0.0f);
        mask = n - 1;
        w = 0;
    }
    void clear() noexcept { std::fill (buf.begin(), buf.end(), 0.0f); }
    void push (float x) noexcept { buf[(size_t) w] = x; w = (w + 1) & mask; }
    // d >= 2 : retard en echantillons par rapport au prochain echantillon ecrit
    float read (float d) const noexcept
    {
        const int n0 = (int) d;
        const float fr = d - (float) n0;
        const int i = w - n0;
        const float ym1 = buf[(size_t) ((i + 1) & mask)];
        const float y0  = buf[(size_t) (i & mask)];
        const float y1  = buf[(size_t) ((i - 1) & mask)];
        const float y2  = buf[(size_t) ((i - 2) & mask)];
        const float c1 = y1 - ym1 * (1.0f / 3.0f) - y0 * 0.5f - y2 * (1.0f / 6.0f);
        const float c2 = (ym1 + y1) * 0.5f - y0;
        const float c3 = (y2 - ym1) * (1.0f / 6.0f) + (y0 - y1) * 0.5f;
        return ((c3 * fr + c2) * fr + c1) * fr + y0;
    }
private:
    std::vector<float> buf;
    int mask = 0, w = 0;
};

// Parametres communs a toutes les voix (valeurs normalisees 0..1 sauf mention).
struct Params
{
    float breath = 0.6f;       // pression du souffle
    float noise = 0.4f;        // quantite de souffle audible
    float noiseColor = 0.5f;   // sombre -> clair
    float brightness = 0.5f;   // ouverture du timbre
    float overblow = 0.0f;     // 0 = jamais, puis les notes fortes octavient a l'attaque, 1 = registre aigu tenu
    float chiff = 0.5f;        // attaque soufflee + petit glissando d'attaque
    float vibRate = 5.2f;      // Hz
    float vibDepth = 0.25f;    // 0..1 (1 = +/- 60 cents)
    float vibDelay = 0.25f;    // secondes
    float glide = 0.06f;       // secondes
    float bendSemis = 0.0f;    // pitch bend courant, en demi-tons
    float pressureCC = 1.0f;   // souffle continu (CC2 / aftertouch), 0..1
    float humanize = 1.0f;     // micro-instabilite naturelle du souffle (0 = aucune)
    std::array<float, 12> cents {}; // accordage par classe de hauteur
};

inline float midiToHz (float note) noexcept { return 440.0f * std::pow (2.0f, (note - 69.0f) / 12.0f); }

class Voice
{
public:
    void prepare (double fsIn)
    {
        fs = fsIn;
        bore.resize ((int) (fs / 18.0) + 8);
        jet.resize ((int) (fs / 40.0) + 8);
        ctrlN = std::max (8, (int) (fs / 6000.0));
        dcR = (float) (1.0 - 2.0 * kPi * 20.0 / fs);
        reset();
    }

    void reset() noexcept
    {
        bore.clear(); jet.clear();
        lpZ = dcX = dcY = 0.0f;
        env = envTarget = 0.0f;
        chiffEnv = scoop = 0.0f;
        nz1 = nz2 = airLp = airHp = 0.0f;
        active = false; gate = false;
        ctrlCount = 0; vibPhase = 0.0f; vibOnset = 0.0f; drift = 0.0f; slow = 0.0f;
        svfIc1 = svfIc2 = toneDc = 0.0f;
        regEnv = 0.0f; dyn = 0.0f; dynTarget = 0.0f; regOn = regArmed = false;
        boreD = boreInc = jetD = jetInc = 0.0f;
        first = true;
    }

    bool isActive() const noexcept { return active; }
    bool isGateOn() const noexcept { return gate; }
    int  note() const noexcept { return midiNote; }
    float lastVelocity() const noexcept { return velocity; }
    uint32_t age = 0;

    void noteOn (int n, float vel, bool legato) noexcept
    {
        midiNote = n;
        velocity = vel;
        targetNote = (float) n;
        if (! legato || ! active)
        {
            if (! active) first = true;
            curNote = targetNote;
            chiffEnv = 1.0f;
            scoop = 1.0f;
            vibOnset = 0.0f;
            regEnv = 1.0f;
            regArmed = true;
        }
        gate = true;
        active = true;
    }

    void noteOff() noexcept { gate = false; }

    // un echantillon au taux surechantillonne
    float tick (const Params& p) noexcept
    {
        if (! active) return 0.0f;
        if (ctrlCount <= 0) { control (p); ctrlCount = ctrlN; }
        --ctrlCount;
        boreD += boreInc; jetD += jetInc; lpA += lpAInc;
        env += (envTarget - env) * envCoef;

        // bruit de souffle colore
        const float w = rng.next();
        nz1 += (w - nz1) * nzCoef;
        const float breathNoise = nz1 + (w - nz1) * hiMix;

        const float press = env * pressure;
        const float blow = press * (1.0f + noiseGain * breathNoise + chiffEnv * chiffGain * w) + tremolo * press;

        const float boreOut = bore.read (boreD);
        lpZ += (boreOut - lpZ) * lpA;                 // pertes du tube
        float temp = -lpZ;
        const float dcOut = temp - dcX + dcR * dcY;   // coupe-continu
        dcX = temp; dcY = dcOut; temp = dcOut;

        const float pd = blow - jetRefl * temp;
        const float jd = jet.read (jetD);
        jet.push (pd);
        float nl = jd * (jd * jd - 1.0f);             // non-linearite du jet d'air
        nl = std::max (-1.0f, std::min (1.0f, nl));
        bore.push (nl + endRefl * temp);

        // souffle "a cote" de la note, typique du ney
        airLp += (w - airLp) * airLpC;
        airHp += (airLp - airHp) * airHpC;
        const float air = (airLp - airHp) * airGain * env * (0.6f + 0.4f * velocity + chiffEnv * chiffGain * 2.0f);

        if (! gate && env < 1.0e-4f && std::abs (boreOut) < 1.0e-4f) { active = false; }
        // mise en forme du timbre : harmoniques pairs + filtre suivant la note
        float tone = 0.42f * boreOut;
        tone += evenAmt * tone * tone;
        toneDc += (tone - toneDc) * toneDcC;
        tone -= toneDc;
        const float v3 = tone - svfIc2;
        const float v1 = svfA1 * svfIc1 + svfA2 * v3;
        const float v2 = svfIc2 + svfA2 * svfIc1 + svfA3 * v3;
        svfIc1 = 2.0f * v1 - svfIc1;
        svfIc2 = 2.0f * v2 - svfIc2;

        dyn += (dynTarget - dyn) * dynCoef;
        return (toneGain * v2 + air) * dyn;
    }

private:
    void control (const Params& p) noexcept
    {
        const float dt = (float) ctrlN / (float) fs;

        // enveloppe de souffle
        const float atk = 0.012f + 0.07f * (1.0f - velocity);
        envTarget = gate ? 1.0f : 0.0f;
        envCoef = 1.0f - std::exp (-1.0f / ((gate ? atk : 0.09f) * (float) fs));
        chiffEnv *= std::exp (-dt / 0.045f);
        scoop *= std::exp (-dt / 0.05f);

        // glissando entre notes liees
        if (p.glide < 0.002f) curNote = targetNote;
        else curNote += (targetNote - curNote) * (1.0f - std::exp (-dt / (p.glide * 0.35f)));

        // vibrato : apparition retardee + legere derive
        vibOnset = std::min (1.0f, vibOnset + dt / std::max (0.02f, p.vibDelay + 0.15f));
        const float onset = vibOnset < 0.5f ? 0.0f : (vibOnset - 0.5f) * 2.0f;
        drift += (rng.next() * 0.08f - drift) * 0.02f;
        vibPhase += dt * p.vibRate * (1.0f + drift);
        if (vibPhase >= 1.0f) vibPhase -= 1.0f;
        const float lfo = std::sin (2.0f * (float) kPi * vibPhase);
        const float vib = lfo * p.vibDepth * onset;
        tremolo = 0.12f * vib;

        // registre : avec Overblow, les attaques fortes partent a l'octave puis retombent
        const float ob = std::max (0.0f, std::min (1.0f, p.overblow));
        if (regArmed) { regOn = ob > 0.02f && velocity > 1.0f - ob; regArmed = false; }
        regEnv *= std::exp (-dt / (0.03f + 0.12f * ob));
        const bool upper = ob >= 0.97f || (regOn && regEnv > 0.5f);

        slow += (rng.next() - slow) * 0.004f; // micro-instabilite du souffle
        const int pc = ((int) std::lround (curNote) % 12 + 12) % 12;
        const float cents = p.cents[(size_t) pc] + 60.0f * vib + 40.0f * slow * p.humanize
                          - 35.0f * p.chiff * scoop * (0.4f + 0.6f * velocity);
        float note = curNote + p.bendSemis + cents * 0.01f + (upper ? 12.0f : 0.0f);
        note -= 0.01f * tuningCorrectionCents (note, p.brightness) * tuneOn;
        const float f = std::min (midiToHz (note), (float) fs * 0.2f);

        pressure = (0.95f + 0.22f * p.breath) * (0.94f + 0.06f * velocity);
        dynTarget = (0.28f + 0.72f * velocity * velocity) * (0.15f + 0.85f * p.pressureCC) * (0.55f + 0.45f * p.breath);

        // filtre de pertes (coupure en Hz -> independant de la frequence d'echantillonnage)
        const float fc = std::min (0.4f * (float) fs, midiToHz (curNote + (upper ? 12.0f : 0.0f)) * (3.5f + 3.5f * p.brightness));
        const float a = 1.0f - std::exp (-2.0f * (float) kPi * fc / (float) fs);

        // compensation de phase des filtres de boucle pour rester juste
        const float wn = 2.0f * (float) kPi * f / (float) fs;
        const float pl = 1.0f - a;
        const float lpDelay = std::atan2 (pl * std::sin (wn), 1.0f - pl * std::cos (wn)) / wn;
        const float dcLead = (std::atan2 (std::sin (wn), 1.0f - std::cos (wn)) - std::atan2 (dcR * std::sin (wn), 1.0f - dcR * std::cos (wn))) / wn;
        float total = (float) fs / (f * tuneK) - lpDelay + dcLead + sampleOffset;
        total = std::max (4.0f, total);
        const float nb = total, nj = std::max (2.5f, total * jetRatio);

        if (first) { boreD = nb; jetD = nj; lpA = a; first = false; }
        boreInc = (nb - boreD) / (float) ctrlN;
        jetInc = (nj - jetD) / (float) ctrlN;
        lpAInc = (a - lpA) / (float) ctrlN;

        // filtre de timbre (passe-bas 12 dB/oct qui suit la hauteur)
        const float bright = std::max (0.0f, std::min (1.0f, p.brightness + 0.25f * (velocity - 0.6f)));
        const float tfc = std::min (0.45f * (float) fs, f * (1.6f + 7.0f * bright * bright) + 300.0f);
        const float g = std::tan ((float) kPi * tfc / (float) fs);
        const float k = 1.25f;
        svfA1 = 1.0f / (1.0f + g * (g + k));
        svfA2 = g * svfA1;
        svfA3 = g * svfA2;
        evenAmt = 2.2f;
        toneGain = 1.5f - 0.5f * bright;
        toneDcC = 1.0f - std::exp (-2.0f * (float) kPi * 25.0f / (float) fs);
        dynCoef = 1.0f - std::exp (-1.0f / (0.012f * (float) fs));

        // bruit
        noiseGain = 0.04f + 0.42f * p.noise * p.noise + 0.1f * p.noise;
        chiffGain = 0.9f * p.chiff;
        const float nfc = 900.0f * std::pow (2.0f, 3.2f * p.noiseColor);
        nzCoef = 1.0f - std::exp (-2.0f * (float) kPi * nfc / (float) fs);
        hiMix = 0.15f + 0.5f * p.noiseColor;
        airGain = 0.5f * p.noise + 2.5f * p.noise * p.noise;
        airLpC = 1.0f - std::exp (-2.0f * (float) kPi * (2500.0f + 7000.0f * p.noiseColor) / (float) fs);
        airHpC = 1.0f - std::exp (-2.0f * (float) kPi * (700.0f + 1500.0f * p.noiseColor) / (float) fs);
    }

public:
    float tuneK = 0.66666f; // constante d'accord du modele
    float sampleOffset = -0.1f; // retard fixe de la boucle, en echantillons
    float tuneOn = 1.0f;    // 0 = desactive la table de correction (calibration)
private:
    double fs = 88200.0;
    DelayLine bore, jet;
    Rng rng;
    int midiNote = 60, ctrlN = 16, ctrlCount = 0;
    float velocity = 0.8f, targetNote = 60.0f, curNote = 60.0f;
    float lpZ = 0, dcX = 0, dcY = 0, dcR = 0.999f;
    float env = 0, envTarget = 0, envCoef = 0.001f, chiffEnv = 0, scoop = 0;
    float boreD = 0, boreInc = 0, jetD = 0, jetInc = 0, lpA = 0.3f, lpAInc = 0;
    float pressure = 1.0f, jetRatio = 0.32f, jetRefl = 0.5f, endRefl = 0.5f;
    float noiseGain = 0.1f, chiffGain = 0.5f, nzCoef = 0.2f, hiMix = 0.3f, nz1 = 0, nz2 = 0;
    float airGain = 0, airLp = 0, airHp = 0, airLpC = 0.3f, airHpC = 0.05f;
    float vibPhase = 0, vibOnset = 0, drift = 0, tremolo = 0, slow = 0;
    float regEnv = 0, dyn = 0, dynTarget = 0, dynCoef = 0.001f;
    float svfIc1 = 0, svfIc2 = 0, svfA1 = 0, svfA2 = 0, svfA3 = 0, evenAmt = 1.5f, toneGain = 1.0f, toneDc = 0, toneDcC = 0.001f;
    bool active = false, gate = false, first = true, regOn = false, regArmed = false;
};
} // namespace ney
