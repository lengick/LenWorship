#pragma once

// Efectos de Len-Worship: delay ping-pong oscuro y reverb enorme tipo "supermassive"
// (red de retardos con realimentacion, FDN). No depende de JUCE.

#include <algorithm>
#include <cmath>
#include <vector>

namespace lenfx
{
constexpr float kPi = 3.14159265358979323846f;

//==============================================================================
// Delay ping-pong con realimentacion filtrada (cada repeticion suena mas oscura)
class PingPongDelay
{
public:
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        size = (int) (sr * 2.0) + 8;
        bufL.assign ((size_t) size, 0.0f);
        bufR.assign ((size_t) size, 0.0f);
        writePos = 0;
        lpL = lpR = 0.0f;
        lpCoef = 1.0f - std::exp (-2.0f * kPi * 3000.0f / sampleRate);
        smoothCoef = 1.0f - std::exp (-1.0f / (0.12f * sampleRate));
        delaySamples = targetSamples = 0.45f * sampleRate;
    }

    void setTimeMs (float ms)
    {
        targetSamples = std::min (std::max (ms, 20.0f) * 0.001f * sampleRate, (float) (size - 4));
    }

    void setMix (float m) { mix = m; }
    void setFeedback (float f) { feedback = f; }

    void process (float& l, float& r)
    {
        float* bl = bufL.data();
        float* br = bufR.data();

        delaySamples += smoothCoef * (targetSamples - delaySamples);

        float readPos = (float) writePos - delaySamples;
        if (readPos < 0.0f)
            readPos += (float) size;

        int i0 = (int) readPos;
        const float frac = readPos - (float) i0;
        if (i0 >= size)
            i0 -= size;
        int i1 = i0 + 1;
        if (i1 >= size)
            i1 = 0;

        const float dl = bl[i0] + frac * (bl[i1] - bl[i0]);
        const float dr = br[i0] + frac * (br[i1] - br[i0]);

        lpL += lpCoef * (dl - lpL);
        lpR += lpCoef * (dr - lpR);

        const float in = 0.5f * (l + r);
        bl[writePos] = in + lpR * feedback;   // ping-pong: izquierda -> derecha -> izquierda
        br[writePos] = lpL * feedback;

        if (++writePos >= size)
            writePos = 0;

        l += dl * mix;
        r += dr * mix;
    }

private:
    std::vector<float> bufL, bufR;
    int size = 0, writePos = 0;
    float sampleRate = 44100.0f;
    float delaySamples = 0.0f, targetSamples = 0.0f, smoothCoef = 0.001f;
    float lpL = 0.0f, lpR = 0.0f, lpCoef = 0.3f;
    float mix = 0.0f, feedback = 0.5f;
};

//==============================================================================
// Reverb enorme: 4 difusores allpass + 8 lineas de retardo moduladas + matriz Hadamard
class BigReverb
{
public:
    static constexpr int N = 8;

    void prepare (double sr)
    {
        sampleRate = (float) sr;
        const float scale = sampleRate / 44100.0f;

        static const int baseLens[N] = { 1931, 2347, 2753, 3187, 3643, 4093, 4523, 4967 };
        modDepth = 7.0f * scale;

        for (int i = 0; i < N; ++i)
        {
            auto& ln = lines[i];
            ln.baseLen = (float) baseLens[i] * scale;
            ln.size = (int) (ln.baseLen + modDepth) + 8;
            ln.buf.assign ((size_t) ln.size, 0.0f);
            ln.w = 0;
            ln.lp = 0.0f;
            ln.gain = 0.0f;
            ln.phase = (float) i / (float) N;
            ln.phaseInc = (0.11f + 0.057f * (float) i) / sampleRate;
        }

        static const int apLens[4] = { 142, 107, 379, 277 };
        for (int i = 0; i < 4; ++i)
            aps[i].init ((int) ((float) apLens[i] * scale), 0.7f);

        preSize = (int) (0.022f * sampleRate) + 1;
        pre.assign ((size_t) preSize, 0.0f);
        preW = 0;

        dampCoef = 1.0f - std::exp (-2.0f * kPi * 3200.0f / sampleRate);
        rt60 = targetRt60;
        counter = 0;
        updateGains();
    }

    // size 0..1  ->  tiempo de decaimiento de ~0.8 s a ~16 s
    void setSize (float s01) { targetRt60 = 0.8f * std::pow (20.0f, std::min (std::max (s01, 0.0f), 1.0f)); }
    void setMix (float m) { mix = m; }

    void process (float& l, float& r)
    {
        if ((++counter & 63) == 0)
        {
            rt60 += 0.2f * (targetRt60 - rt60);
            updateGains();
        }

        float in = 0.5f * (l + r);

        // pre-delay
        {
            const float delayed = pre[(size_t) preW];
            pre[(size_t) preW] = in;
            if (++preW >= preSize)
                preW = 0;
            in = delayed;
        }

        for (auto& ap : aps)
            in = ap.process (in);

        float o[N];
        for (int i = 0; i < N; ++i)
        {
            auto& ln = lines[i];
            float* b = ln.buf.data();

            ln.phase += ln.phaseInc;
            if (ln.phase >= 1.0f)
                ln.phase -= 1.0f;

            const float d = ln.baseLen + modDepth * std::sin (2.0f * kPi * ln.phase);
            float rpos = (float) ln.w - d;
            if (rpos < 0.0f)
                rpos += (float) ln.size;

            int i0 = (int) rpos;
            const float fr = rpos - (float) i0;
            if (i0 >= ln.size)
                i0 -= ln.size;
            int i1 = i0 + 1;
            if (i1 >= ln.size)
                i1 = 0;

            const float x = b[i0] + fr * (b[i1] - b[i0]);
            ln.lp += dampCoef * (x - ln.lp);
            o[i] = ln.lp;
        }

        float m[N];
        for (int i = 0; i < N; ++i)
            m[i] = o[i];
        hadamard8 (m);

        static const float inSign[N] = { 1.f, -1.f, 1.f, -1.f, -1.f, 1.f, -1.f, 1.f };
        const float norm = 0.35355339f;

        for (int i = 0; i < N; ++i)
        {
            auto& ln = lines[i];
            ln.buf[(size_t) ln.w] = in * inSign[i] * 0.35f + m[i] * norm * ln.gain;
            if (++ln.w >= ln.size)
                ln.w = 0;
        }

        const float wetL = 0.5f * (o[0] - o[2] + o[4] - o[6]);
        const float wetR = 0.5f * (o[1] - o[3] + o[5] - o[7]);

        l = l * (1.0f - 0.35f * mix) + wetL * wetScale * mix;
        r = r * (1.0f - 0.35f * mix) + wetR * wetScale * mix;
    }

private:
    struct Line
    {
        std::vector<float> buf;
        int size = 0, w = 0;
        float baseLen = 0.0f, lp = 0.0f, gain = 0.0f, phase = 0.0f, phaseInc = 0.0f;
    };

    struct Allpass
    {
        std::vector<float> buf;
        int size = 1, w = 0;
        float g = 0.7f;

        void init (int len, float gain)
        {
            size = std::max (len, 1);
            buf.assign ((size_t) size, 0.0f);
            w = 0;
            g = gain;
        }

        float process (float x)
        {
            const float delayed = buf[(size_t) w];
            const float v = x + g * delayed;
            buf[(size_t) w] = v;
            if (++w >= size)
                w = 0;
            return delayed - g * v;
        }
    };

    static void hadamard8 (float* x)
    {
        for (int h = 1; h < 8; h <<= 1)
            for (int i = 0; i < 8; i += h * 2)
                for (int j = i; j < i + h; ++j)
                {
                    const float a = x[j], b = x[j + h];
                    x[j] = a + b;
                    x[j + h] = a - b;
                }
    }

    void updateGains()
    {
        for (auto& ln : lines)
            ln.gain = std::pow (10.0f, -3.0f * ln.baseLen / (rt60 * sampleRate));
    }

    Line lines[N];
    Allpass aps[4];
    std::vector<float> pre;
    int preSize = 1, preW = 0;
    float sampleRate = 44100.0f, modDepth = 7.0f, dampCoef = 0.4f;
    float rt60 = 6.0f, targetRt60 = 6.0f;
    float mix = 0.0f;
    float wetScale = 1.4f;
    unsigned int counter = 0;
};
} // namespace lenfx
