#pragma once
#include <cmath>
#include <vector>
#include <array>
#include <algorithm>

class SynthVoice {
public:
    enum Waveform { Sine = 0, Saw, Square, Triangle, NumWaveforms };

    void prepare(double sampleRate) {
        fs = sampleRate;
        phase1 = 0.0;
        phase2 = 0.0;
        env = 0.0;
        filterState[0] = filterState[1] = filterState[2] = 0.0f;
    }

    void noteOn(float freq, float vel) {
        frequency = freq;
        velocity = vel;
        phase1 = 0.0;
        phase2 = 0.0;
        envStage = Attack;
        gate = true;
    }

    void noteOff() {
        gate = false;
        envStage = Release;
    }

    void setADSR(float a, float d, float s, float r) {
        attack  = std::max(0.001f, a);
        decay   = std::max(0.001f, d);
        sustain = std::clamp(s, 0.0f, 1.0f);
        release = std::max(0.001f, r);
    }

    void setOsc1(Waveform wf, float detune) {
        wave1 = wf;
        detune1 = detune;
    }

    void setOsc2(Waveform wf, float detune, float mix) {
        wave2 = wf;
        detune2 = detune;
        osc2Mix = std::clamp(mix, 0.0f, 1.0f);
    }

    void setFilter(float cutoff, float reso) {
        filterCutoff = std::clamp(cutoff, 20.0f, 20000.0f);
        filterReso = std::clamp(reso, 0.5f, 10.0f);
    }

    bool isActive() const { return env > 0.0001f || gate; }

    float render() {
        // 1. Envelope
        switch (envStage) {
            case Attack:
                env += (1.0f / (attack * fs));
                if (env >= 1.0f) { env = 1.0f; envStage = Decay; }
                break;
            case Decay:
                env -= ((1.0f - sustain) / (decay * fs));
                if (env <= sustain) { env = sustain; envStage = SustainStage; }
                break;
            case SustainStage:
                env = sustain;
                if (!gate) envStage = Release;
                break;
            case Release:
                env -= (env / (release * fs));
                if (env < 0.0001f) { env = 0.0f; return 0.0f; }
                break;
        }

        // 2. Oscillator 1
        double inc1 = (frequency * std::pow(2.0, detune1 / 1200.0)) / fs;
        phase1 += inc1;
        if (phase1 >= 1.0) phase1 -= 1.0;
        float osc1 = renderOscillator(wave1, phase1, inc1);

        // 3. Oscillator 2
        double inc2 = (frequency * std::pow(2.0, detune2 / 1200.0)) / fs;
        phase2 += inc2;
        if (phase2 >= 1.0) phase2 -= 1.0;
        float osc2 = renderOscillator(wave2, phase2, inc2);

        // 4. Mix oscillators
        float mixed = osc1 * (1.0f - osc2Mix) + osc2 * osc2Mix;

        // 5. Filter (2-pole SVF)
        float wc = 2.0f * 3.14159265f * filterCutoff / (float)fs;
        float g = std::tan(wc * 0.5f);
        float k = 1.0f / filterReso;
        float a1 = 1.0f / (1.0f + g * (g + k));
        float a2 = g * a1;
        float a3 = g * a2;

        float v3 = mixed - filterState[2];
        float v1 = filterState[0] + a2 * v3;
        float v2 = filterState[1] + a2 * v1;
        filterState[0] = v1 + a1 * v3;
        filterState[1] = v2 + a2 * v1;
        filterState[2] = v2 + a3 * v1;

        return v2 * env * velocity * 0.3f;
    }

private:
    float renderOscillator(Waveform wf, double phase, double inc) {
        float t = (float)phase;
        switch (wf) {
            case Sine:
                return (float)std::sin(2.0 * 3.14159265 * phase);
            case Saw:
                return 2.0f * t - 1.0f - polyBlep(t, (float)inc);
            case Square:
                return (t < 0.5f ? 1.0f : -1.0f) + 
                       polyBlep(t, (float)inc) - 
                       polyBlep(std::fmod(t + 0.5f, 1.0f), (float)inc);
            case Triangle:
                return 2.0f * std::abs(2.0f * t - 1.0f) - 1.0f;
            default:
                return 0.0f;
        }
    }

    float polyBlep(float t, float dt) {
        if (t < dt) {
            t /= dt;
            return t + t - t * t - 1.0f;
        } else if (t > 1.0f - dt) {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    enum EnvStage { Attack, Decay, SustainStage, Release };
    EnvStage envStage = Attack;
    double fs = 44100.0;
    double phase1 = 0.0, phase2 = 0.0;
    float frequency = 440.0f;
    float velocity = 1.0f;
    float env = 0.0f;
    bool gate = false;
    float attack = 0.01f, decay = 0.2f, sustain = 0.7f, release = 0.3f;
    Waveform wave1 = Saw, wave2 = Square;
    float detune1 = 0.0f, detune2 = 0.0f;
    float osc2Mix = 0.5f;
    float filterCutoff = 5000.0f, filterReso = 1.0f;
    std::array<float, 3> filterState{0, 0, 0};
};

class SynthEngine {
public:
    void prepare(double sampleRate, int maxVoices = 8) {
        voices.resize(maxVoices);
        for (auto& v : voices) v.prepare(sampleRate);
    }

    void noteOn(int note, float velocity) {
        float freq = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
        for (auto& v : voices) {
            if (!v.isActive()) {
                v.noteOn(freq, velocity);
                return;
            }
        }
    }

    void noteOff(int note) {
        for (auto& v : voices) {
            v.noteOff();
        }
    }

    void setADSR(float a, float d, float s, float r) {
        for (auto& v : voices) v.setADSR(a, d, s, r);
    }

    void setOsc1(int wf, float detune) {
        for (auto& v : voices) v.setOsc1((SynthVoice::Waveform)wf, detune);
    }

    void setOsc2(int wf, float detune, float mix) {
        for (auto& v : voices) v.setOsc2((SynthVoice::Waveform)wf, detune, mix);
    }

    void setFilter(float cutoff, float reso) {
        for (auto& v : voices) v.setFilter(cutoff, reso);
    }

    float render() {
        float sample = 0.0f;
        for (auto& v : voices) sample += v.render();
        return sample;
    }

private:
    std::vector<SynthVoice> voices;
};
