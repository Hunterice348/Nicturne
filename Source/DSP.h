#pragma once
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>

namespace hunterice {

constexpr float PI = 3.14159265358979f;
constexpr float TWO_PI = 6.28318530717959f;

// ============ POLYBLEP антиалиасинг ============
inline float polyBlep(float t, float dt) {
    if (t < dt) { t /= dt; return t + t - t * t - 1.0f; }
    else if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

// ============ ОСЦИЛЛЯТОР ============
class Oscillator {
public:
    enum Waveform { Sine = 0, Triangle, Saw, Square, Pulse, Noise, NumWaves };

    void prepare(double sr) { fs = sr; phase = 0.0; }

    void setFrequency(float f) { frequency = std::max(20.0f, std::min(f, 20000.0f)); }
    void setWaveform(int w) { waveform = std::clamp(w, 0, (int)NumWaves - 1); }
    void setPulseWidth(float pw) { pulseWidth = std::clamp(pw, 0.1f, 0.9f); }
    void setDetune(float cents) { detuneCents = cents; }

    float render() {
        double freq = frequency * std::pow(2.0, detuneCents / 1200.0);
        double inc = freq / fs;
        phase += inc;
        if (phase >= 1.0) phase -= 1.0;
        float t = (float)phase;
        float dt = (float)inc;

        switch (waveform) {
            case Sine:     return std::sin(TWO_PI * (float)phase);
            case Triangle: return 2.0f * std::abs(2.0f * t - 1.0f) - 1.0f;
            case Saw:      return 2.0f * t - 1.0f - polyBlep(t, dt);
            case Square: {
                float sq = (t < pulseWidth) ? 1.0f : -1.0f;
                return sq + polyBlep(t, dt) - polyBlep(std::fmod(t + pulseWidth, 1.0f), dt);
            }
            case Pulse: {
                float p = (t < pulseWidth) ? 1.0f : 0.0f;
                return p - pulseWidth;
            }
            case Noise:    return (float)(rand() / (float)RAND_MAX) * 2.0f - 1.0f;
            default:       return 0.0f;
        }
    }

    void reset() { phase = 0.0; }

private:
    double fs = 44100.0;
    double phase = 0.0;
    float frequency = 440.0f;
    float detuneCents = 0.0f;
    float pulseWidth = 0.5f;
    int waveform = Saw;
};

// ============ ADSR ОГИБАЮЩАЯ ============
class ADSR {
public:
    void prepare(double sr) { fs = sr; value = 0.0f; }

    void setParameters(float a, float d, float s, float r) {
        attack  = std::max(0.001f, a);
        decay   = std::max(0.001f, d);
        sustain = std::clamp(s, 0.0f, 1.0f);
        release = std::max(0.001f, r);
    }

    void noteOn()  { stage = Attack; }
    void noteOff() { stage = Release; }

    float process() {
        switch (stage) {
            case Attack:
                value += (float)(1.0 / (attack * fs));
                if (value >= 1.0f) { value = 1.0f; stage = Decay; }
                break;
            case Decay:
                value -= (float)((1.0 - sustain) / (decay * fs));
                if (value <= sustain) { value = sustain; stage = Sustain; }
                break;
            case Sustain:
                value = sustain;
                break;
            case Release:
                value -= (float)(value / (release * fs));
                if (value < 0.0001f) { value = 0.0f; stage = Idle; }
                break;
            case Idle:
                value = 0.0f;
                break;
        }
        return value;
    }

    bool isActive() const { return stage != Idle; }

private:
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    Stage stage = Idle;
    double fs = 44100.0;
    float value = 0.0f;
    float attack = 0.01f, decay = 0.2f, sustain = 0.7f, release = 0.3f;
};

// ============ LFO ============
class LFO {
public:
    enum Shape { Sine = 0, Triangle, Saw, Square, Random, SH, NumShapes };

    void prepare(double sr) { fs = sr; phase = 0.0; }
    void setRate(float hz) { rate = std::max(0.01f, std::min(hz, 50.0f)); }
    void setDepth(float d) { depth = std::clamp(d, 0.0f, 1.0f); }
    void setShape(int s) { shape = std::clamp(s, 0, (int)NumShapes - 1); }

    float process() {
        phase += rate / fs;
        if (phase >= 1.0) { phase -= 1.0; randomValue = (float)(rand() / (float)RAND_MAX) * 2.0f - 1.0f; }
        float t = (float)phase;
        float v = 0.0f;
        switch (shape) {
            case Sine:     v = std::sin(TWO_PI * t); break;
            case Triangle: v = 2.0f * std::abs(2.0f * t - 1.0f) - 1.0f; break;
            case Saw:      v = 2.0f * t - 1.0f; break;
            case Square:   v = (t < 0.5f) ? 1.0f : -1.0f; break;
            case Random:   v = randomValue; break;
            case SH:       v = sampleHold; if (t < 0.01) sampleHold = randomValue; break;
        }
        return v * depth;
    }

private:
    double fs = 44100.0;
    double phase = 0.0;
    float rate = 1.0f;
    float depth = 0.5f;
    int shape = Sine;
    float randomValue = 0.0f;
    float sampleHold = 0.0f;
};

// ============ ФИЛЬТР SVF (LP/HP/BP/Notch) ============
class SVFFilter {
public:
    enum Mode { LP = 0, HP, BP, Notch, NumModes };

    void prepare(double sr) { fs = sr; ic1eq = ic2eq = 0.0f; }

    void setParameters(float cutoff, float reso, int mode) {
        this->cutoff = std::clamp(cutoff, 20.0f, 20000.0f);
        this->reso = std::clamp(reso, 0.5f, 10.0f);
        this->mode = std::clamp(mode, 0, (int)NumModes - 1);
    }

    float process(float in) {
        float g = std::tan(PI * cutoff / (float)fs);
        float k = 1.0f / reso;
        float a1 = 1.0f / (1.0f + g * (g + k));
        float a2 = g * a1;
        float a3 = g * a2;

        float v3 = in - ic2eq;
        float v1 = a1 * ic1eq + a2 * v3;
        float v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;

        switch (mode) {
            case LP:    return v2;
            case HP:    return in - k * v1 - v2;
            case BP:    return v1;
            case Notch: return in - k * v1;
            default:    return in;
        }
    }

    void reset() { ic1eq = ic2eq = 0.0f; }

private:
    double fs = 44100.0;
    float ic1eq = 0.0f, ic2eq = 0.0f;
    float cutoff = 5000.0f, reso = 1.0f;
    int mode = LP;
};

// ============ DISTORTION (10 режимов) ============
class Distortion {
public:
    enum Mode { HardClip=0, SoftClip, Tube, Diode, Fuzz, Foldback, Waveshape, Asym, Digital, Industrial, NumModes };

    void setMode(int m) { mode = std::clamp(m, 0, (int)NumModes - 1); }

    float process(float in, float drive) {
        float x = in * drive;
        switch (mode) {
            case HardClip:   return std::tanh(x);
            case SoftClip:   return std::tanh(x * 0.8f);
            case Tube:       return std::tanh(std::sqrt(std::abs(x) + 1e-9f) * std::copysign(1.0f, x) * 1.5f);
            case Diode:      return (x > 0 ? 1.0f - std::exp(-x) : -1.0f + std::exp(x));
            case Fuzz:       return std::tanh(x * 3.0f) * 0.7f;
            case Foldback: {
                float th = 1.0f;
                while (std::abs(x) > th) x = std::copysign(th, x) - (x - std::copysign(th, x));
                return x;
            }
            case Waveshape:  return std::sin(x * PI * 0.5f);
            case Asym:       return std::tanh(x > 0 ? x * 1.5f : x * 0.7f);
            case Digital: {
                float steps = 16.0f;
                return std::tanh(std::round(x * steps) / steps);
            }
            case Industrial: return std::tanh(std::tanh(x) + 0.3f * std::sin(x * 5.0f));
            default:         return x;
        }
    }

private:
    int mode = SoftClip;
};

// ============ DELAY ============
class Delay {
public:
    void prepare(double sr, float maxTimeSec = 2.0f) {
        fs = sr;
        buffer.assign((size_t)(sr * maxTimeSec), 0.0f);
        writePos = 0;
    }

    float process(float in, float timeSec, float feedback, float mix) {
        int delaySamples = std::max(1, (int)(timeSec * fs));
        if (delaySamples >= (int)buffer.size()) delaySamples = (int)buffer.size() - 1;
        int readPos = writePos - delaySamples;
        if (readPos < 0) readPos += (int)buffer.size();
        float delayed = buffer[readPos];
        buffer[writePos] = in + delayed * feedback;
        writePos++;
        if (writePos >= (int)buffer.size()) writePos = 0;
        return in * (1.0f - mix) + delayed * mix;
    }

    void reset() { std::fill(buffer.begin(), buffer.end(), 0.0f); }

private:
    double fs = 44100.0;
    std::vector<float> buffer;
    int writePos = 0;
};

// ============ REVERB (Freeverb-подобный) ============
class Reverb {
public:
    void prepare(double sr) {
        fs = sr;
        for (auto& c : combL) c.prepare(sr);
        for (auto& c : combR) c.prepare(sr);
        for (auto& a : allpassL) a.prepare(sr);
        for (auto& a : allpassR) a.prepare(sr);
    }

    void setParameters(float roomSize, float damping, float mix) {
        this->mix = std::clamp(mix, 0.0f, 1.0f);
        float rs = std::clamp(roomSize, 0.0f, 1.0f);
        float d  = std::clamp(damping, 0.0f, 1.0f);
        static const int combTuningsL[] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
        static const int combTuningsR[] = {1139, 1211, 1300, 1379, 1445, 1514, 1580, 1640};
        for (int i = 0; i < 8; ++i) {
            combL[i].setParams(combTuningsL[i], rs, d);
            combR[i].setParams(combTuningsR[i], rs, d);
        }
        for (int i = 0; i < 4; ++i) {
            allpassL[i].setParams(225 + i * 50, 0.5f);
            allpassR[i].setParams(225 + i * 50, 0.5f);
        }
    }

    void process(float& left, float& right) {
        float outL = 0, outR = 0;
        for (auto& c : combL) outL += c.process(left);
        for (auto& c : combR) outR += c.process(right);
        for (auto& a : allpassL) outL = a.process(outL);
        for (auto& a : allpassR) outR = a.process(outR);
        left  = left  * (1.0f - mix) + outL * mix * 0.4f;
        right = right * (1.0f - mix) + outR * mix * 0.4f;
    }

    void reset() {
        for (auto& c : combL) c.reset();
        for (auto& c : combR) c.reset();
        for (auto& a : allpassL) a.reset();
        for (auto& a : allpassR) a.reset();
    }

private:
    struct Comb {
        std::vector<float> buffer;
        int index = 0;
        float last = 0;
        float feedback = 0.8f, damp = 0.5f;

        void prepare(double sr) { buffer.assign((size_t)(sr * 0.05), 0.0f); }
        void setParams(int size, float fb, float d) {
            if ((int)buffer.size() != size) buffer.assign(size, 0.0f);
            feedback = fb; damp = d;
        }
        float process(float in) {
            float out = buffer[index];
            last = out * (1.0f - damp) + last * damp;
            buffer[index] = in + last * feedback;
            index = (index + 1) % (int)buffer.size();
            return out;
        }
        void reset() { std::fill(buffer.begin(), buffer.end(), 0.0f); last = 0; }
    };

    struct Allpass {
        std::vector<float> buffer;
        int index = 0;
        float feedback = 0.5f;

        void prepare(double sr) { buffer.assign((size_t)(sr * 0.01), 0.0f); }
        void setParams(int size, float fb) {
            if ((int)buffer.size() != size) buffer.assign(size, 0.0f);
            feedback = fb;
        }
        float process(float in) {
            float bufOut = buffer[index];
            float out = -in + bufOut;
            buffer[index] = in + bufOut * feedback;
            index = (index + 1) % (int)buffer.size();
            return out;
        }
        void reset() { std::fill(buffer.begin(), buffer.end(), 0.0f); }
    };

    double fs = 44100.0;
    std::array<Comb, 8> combL, combR;
    std::array<Allpass, 4> allpassL, allpassR;
    float mix = 0.3f;
};

// ============ CHORUS ============
class Chorus {
public:
    void prepare(double sr) {
        fs = sr;
        buffer.assign((size_t)(sr * 0.05), 0.0f);
        writePos = 0;
        phase = 0.0;
    }

    float process(float in, float rate, float depth, float mix) {
        buffer[writePos] = in;
        phase += rate / fs;
        if (phase >= 1.0) phase -= 1.0;
        float mod = std::sin(TWO_PI * (float)phase) * depth * 0.01f * (float)fs;
        int baseDelay = (int)(0.01f * fs);
        float readPos = writePos - baseDelay - mod;
        if (readPos < 0) readPos += (float)buffer.size();
        int ri = (int)readPos;
        float frac = readPos - ri;
        if (ri < 0) ri += (int)buffer.size();
        int ri2 = (ri + 1) % (int)buffer.size();
        float delayed = buffer[ri] * (1.0f - frac) + buffer[ri2] * frac;
        writePos = (writePos + 1) % (int)buffer.size();
        return in * (1.0f - mix) + delayed * mix;
    }

    void reset() { std::fill(buffer.begin(), buffer.end(), 0.0f); }

private:
    double fs = 44100.0;
    std::vector<float> buffer;
    int writePos = 0;
    double phase = 0.0;
};

} // namespace hunterice
