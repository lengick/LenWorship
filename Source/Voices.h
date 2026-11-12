#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace lenvoice
{
// Saw sin aliasing (PolyBLEP)
inline float polyBlep (float t, float dt) noexcept
{
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }

    if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }

    return 0.0f;
}
} // namespace lenvoice

//==============================================================================
struct WorshipSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================
// Cada voz toca dos capas a la vez:
//  1) Piano oscuro: sintesis aditiva con parciales inarmonicos, decaimiento largo,
//     pocos armonicos agudos y un golpe de martillo suave.
//  2) Pad ambiental: 4 sierras desafinadas + sub, filtro oscuro, ataque y release lentos.
class WorshipVoice : public juce::SynthesiserVoice
{
public:
    WorshipVoice() = default;

    void setLevels (float piano, float pad) noexcept
    {
        pianoTarget = piano;
        padTarget = pad;
    }

    bool canPlaySound (juce::SynthesiserSound* s) override
    {
        return dynamic_cast<WorshipSound*> (s) != nullptr;
    }

    void setCurrentPlaybackSampleRate (double newRate) override
    {
        juce::SynthesiserVoice::setCurrentPlaybackSampleRate (newRate);
        padAdsr.setSampleRate (newRate > 0.0 ? newRate : 44100.0);
    }

    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int) override
    {
        sr = getSampleRate() > 0.0 ? getSampleRate() : 44100.0;
        const float fsr = (float) sr;
        const float pi = juce::MathConstants<float>::pi;
        const double f0 = juce::MidiMessage::getMidiNoteInHertz (midiNote);

        // ------------------------------------------------ piano
        numPartials = 0;
        const float bright = 0.35f + 0.65f * velocity;
        const double T1 = 20.0 * std::pow (2.0, -(midiNote - 36) / 24.0);
        const double B = 0.00005 * std::pow (1.05, (double) juce::jmax (0, midiNote - 40));

        for (int n = 1; n <= 14; ++n)
        {
            const double fn = f0 * n * std::sqrt (1.0 + B * n * n);
            const float a = std::pow ((float) n, -1.2f) * std::pow (bright, (float) (n - 1) * 0.4f);
            const double T = T1 / (1.0 + 0.6 * (n - 1));

            if (n <= 3) // "dos cuerdas" ligeramente desafinadas -> movimiento lento
            {
                addPartial (fn * 0.9995, a * 0.55f, T);
                addPartial (fn * 1.0005, a * 0.55f, T * 0.9);
            }
            else
            {
                addPartial (fn, a, T);
            }
        }

        velGain = 0.3f + 0.7f * velocity;
        attackGain = 0.0f;
        attackInc = 1.0f / (0.003f * fsr);
        relGain = 1.0f;
        releasing = false;

        hammerEnv = 1.0f;
        hammerDecay = std::exp (-6.9078f / (0.035f * fsr));
        hammerAmp = 2.0f * velocity;
        hammerCoef = 1.0f - std::exp (-2.0f * pi * 700.0f / fsr);
        hammer1 = hammer2 = 0.0f;

        const float pan = juce::jmap ((float) midiNote, 21.0f, 108.0f, -0.35f, 0.35f);
        const float ang = (pan + 1.0f) * pi * 0.25f;
        panL = std::cos (ang);
        panR = std::sin (ang);

        // ------------------------------------------------ pad
        padFreq = (float) f0;
        const float detune[4] = { -0.0070f, -0.0023f, 0.0023f, 0.0070f };
        const float pans[4] = { -0.85f, -0.3f, 0.3f, 0.85f };

        for (int i = 0; i < 4; ++i)
        {
            auto& o = padOsc[(size_t) i];
            o.phase = rng.nextFloat();
            o.ratio = 1.0f + detune[i];
            const float a = (pans[i] + 1.0f) * pi * 0.25f;
            o.gl = std::cos (a);
            o.gr = std::sin (a);
            o.lfoSign = (i % 2 == 0) ? 1.0f : -1.0f;
        }

        subPhase = 0.0f;
        lfoPhase = rng.nextFloat();
        lfoInc = 0.23f / fsr;

        const float fc = juce::jlimit (300.0f, 5000.0f, padFreq * 4.0f + 600.0f);
        padCoef = 1.0f - std::exp (-2.0f * pi * fc / fsr);
        padL1 = padL2 = padR1 = padR2 = 0.0f;

        padAdsr.setSampleRate (sr);
        padAdsr.setParameters ({ 1.1f, 1.5f, 0.85f, 3.5f });
        padAdsr.reset();
        padAdsr.noteOn();

        pianoLevel = pianoTarget;
        padLevel = padTarget;
        levelCoef = 1.0f - std::exp (-1.0f / (0.012f * fsr));
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            releasing = true;
            relCoef = std::exp (-6.9078f / (0.5f * (float) sr));
            padAdsr.noteOff();
        }
        else
        {
            padAdsr.reset();
            numPartials = 0;
            clearCurrentNote();
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        if (! isVoiceActive())
            return;

        auto* L = out.getWritePointer (0, startSample);
        auto* R = out.getNumChannels() > 1 ? out.getWritePointer (1, startSample) : nullptr;

        const double twoPiD = juce::MathConstants<double>::twoPi;
        const float twoPi = juce::MathConstants<float>::twoPi;

        for (int i = 0; i < numSamples; ++i)
        {
            pianoLevel += levelCoef * (pianoTarget - pianoLevel);
            padLevel += levelCoef * (padTarget - padLevel);
            const float pg = pianoLevel * pianoLevel * 1.5f;
            const float dg = padLevel * padLevel * 1.5f;

            // ---------------- piano
            float ps = 0.0f;
            for (int k = 0; k < numPartials; ++k)
            {
                auto& p = partials[(size_t) k];
                ps += p.amp * p.env * std::sin ((float) p.phase);
                p.phase += p.inc;
                if (p.phase >= twoPiD)
                    p.phase -= twoPiD;
                p.env *= p.coef;
            }

            {
                const float n = rng.nextFloat() * 2.0f - 1.0f;
                hammer1 += hammerCoef * (n - hammer1);
                hammer2 += hammerCoef * (hammer1 - hammer2);
                ps += hammer2 * hammerEnv * hammerAmp;
                hammerEnv *= hammerDecay;
            }

            attackGain = std::min (1.0f, attackGain + attackInc);
            if (releasing)
                relGain *= relCoef;

            ps *= pianoScale * velGain * attackGain * relGain * pg;

            // ---------------- pad
            lfoPhase += lfoInc;
            if (lfoPhase >= 1.0f)
                lfoPhase -= 1.0f;
            const float lfo = std::sin (twoPi * lfoPhase);

            float pl = 0.0f, pr = 0.0f;
            for (auto& o : padOsc)
            {
                const float f = padFreq * o.ratio * (1.0f + 0.0012f * lfo * o.lfoSign);
                const float dt = f / (float) sr;
                o.phase += dt;
                if (o.phase >= 1.0f)
                    o.phase -= 1.0f;

                const float s = 2.0f * o.phase - 1.0f - lenvoice::polyBlep (o.phase, dt);
                pl += s * o.gl;
                pr += s * o.gr;
            }

            subPhase += 0.5f * padFreq / (float) sr;
            if (subPhase >= 1.0f)
                subPhase -= 1.0f;
            const float sub = std::sin (twoPi * subPhase) * 0.8f;
            pl += sub;
            pr += sub;

            padL1 += padCoef * (pl - padL1);
            padL2 += padCoef * (padL1 - padL2);
            padR1 += padCoef * (pr - padR1);
            padR2 += padCoef * (padR1 - padR2);

            const float env = padAdsr.getNextSample();
            const float padGain = padScale * env * dg;

            L[i] += ps * panL + padL2 * padGain;
            if (R != nullptr)
                R[i] += ps * panR + padR2 * padGain;

            if (releasing && relGain < 1.0e-4f && ! padAdsr.isActive())
            {
                numPartials = 0;
                clearCurrentNote();
                break;
            }
        }
    }

private:
    static constexpr int kMaxPartials = 24;

    struct Partial
    {
        double phase = 0.0, inc = 0.0;
        float amp = 0.0f, env = 1.0f, coef = 1.0f;
    };

    struct PadOsc
    {
        float phase = 0.0f, ratio = 1.0f, gl = 0.7f, gr = 0.7f, lfoSign = 1.0f;
    };

    void addPartial (double freq, float amp, double T)
    {
        if (numPartials >= kMaxPartials || freq > sr * 0.45)
            return;

        auto& p = partials[(size_t) numPartials++];
        p.phase = 0.0;
        p.inc = juce::MathConstants<double>::twoPi * freq / sr;
        p.amp = amp;
        p.env = 1.0f;
        p.coef = (float) std::exp (-6.9078 / (T * sr));
    }

    double sr = 44100.0;
    juce::Random rng;

    // niveles (suavizados)
    float pianoTarget = 0.8f, padTarget = 0.5f, pianoLevel = 0.8f, padLevel = 0.5f, levelCoef = 0.003f;

    // piano
    std::array<Partial, kMaxPartials> partials {};
    int numPartials = 0;
    float velGain = 1.0f, attackGain = 0.0f, attackInc = 0.01f;
    float relGain = 1.0f, relCoef = 0.9999f;
    bool releasing = false;
    float hammerEnv = 0.0f, hammerDecay = 0.99f, hammerAmp = 0.0f, hammerCoef = 0.1f, hammer1 = 0.0f, hammer2 = 0.0f;
    float panL = 0.7071f, panR = 0.7071f;
    const float pianoScale = 0.16f;

    // pad
    std::array<PadOsc, 4> padOsc {};
    float padFreq = 220.0f, subPhase = 0.0f, lfoPhase = 0.0f, lfoInc = 0.0f, padCoef = 0.1f;
    float padL1 = 0.0f, padL2 = 0.0f, padR1 = 0.0f, padR2 = 0.0f;
    const float padScale = 0.12f;
    juce::ADSR padAdsr;
};
