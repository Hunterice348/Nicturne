#pragma once
#include "DSP.h"
#include <vector>

namespace hunterice {

class Voice {
public:
    void prepare(double sr) {
        fs = sr;
        osc1.prepare(sr); osc2.prepare(sr); sub.prepare(sr);
        ampEnv.prepare(sr); filterEnv.prepare(sr);
        filter.prepare(sr); lfo1.prepare(sr); lfo2.prepare(sr);
        portamentoCurrent = 0.0f;
    }
    void noteOn(float freq, float vel) {
        targetFreq = freq;
        if (portamentoRate > 0.0f && portamentoCurrent > 0.0f) {} 
        else { portamentoCurrent = freq; }
        velocity = vel;
        ampEnv.noteOn(); filterEnv.noteOn();
        gate = true; active = true;
    }
    void noteOff() { gate = false; ampEnv.noteOff(); filterEnv.noteOff(); }
    bool isActive() const { return active && ampEnv.isActive(); }

    void setOsc1(int wf, float detune, float pw) { osc1.setWaveform(wf); osc1.setDetune(detune); osc1.setPulseWidth(pw); }
    void setOsc2(int wf, float detune, float pw, float mix) { osc2.setWaveform(wf); osc2.setDetune(detune); osc2.setPulseWidth(pw); osc2Mix = mix; }
    void setSub(float mix, int octave) { subMix = mix; subOctave = octave; }
    void setNoise(float mix) { noiseMix = mix; }
    void setAmpADSR(float a, float d, float s, float r) { ampEnv.setParameters(a, d, s, r); }
    void setFilterADSR(float a, float d, float s, float r) { filterEnv.setParameters(a, d, s, r); }
    void setFilter(float cutoff, float reso, int mode, float envAmt) { baseCutoff = cutoff; filterReso = reso; filterMode = mode; filterEnvAmt = envAmt; }
    void setLFO1(int shape, float rate, float depth) { lfo1.setShape(shape); lfo1.setRate(rate); lfo1.setDepth(depth); }
    void setLFO2(int shape, float rate, float depth) { lfo2.setShape(shape); lfo2.setRate(rate); lfo2.setDepth(depth); }
    void setPitchMod(float amt) { pitchModAmt = amt; }
    void setFilterMod(float amt) { filterModAmt = amt; }
    void setUnison(int voices, float detune) { unisonVoices = std::clamp(voices, 1, 8); unisonDetune = detune; }
    void setPortamento(float time) { portamentoRate = time; }

    float render() {
        if (!active) return 0.0f;
        if (portamentoRate > 0.0f && std::abs(portamentoCurrent - targetFreq) > 0.01f) {
            float coef = 1.0f - std::exp(-1.0f / (portamentoRate * fs));
            portamentoCurrent += (targetFreq - portamentoCurrent) * coef;
        } else { portamentoCurrent = targetFreq; }

        float env = ampEnv.process();
        float fenv = filterEnv.process();
        float l1 = lfo1.process();
        float l2 = lfo2.process();
        float pitchMod = 1.0f + l1 * pitchModAmt;
        float freq = portamentoCurrent * pitchMod;

        float sample = 0.0f;
        float unisonGain = 1.0f / std::sqrt((float)unisonVoices);
        for (int u = 0; u < unisonVoices; ++u) {
            float detuneOffset = (unisonVoices > 1) ? (u - (unisonVoices - 1) * 0.5f) * unisonDetune : 0.0f;
            osc1.setDetune(detuneOffset); osc1.setFrequency(freq);
            osc2.setDetune(detuneOffset * 0.5f); osc2.setFrequency(freq);
            sub.setFrequency(freq / (float)(1 << subOctave)); sub.setWaveform(Oscillator::Sine);

            float s = osc1.render() * (1.0f - osc2Mix) + osc2.render() * osc2Mix;
            s += sub.render() * subMix;
            s += ((float)(rand() / (float)RAND_MAX) * 2.0f - 1.0f) * noiseMix;
            sample += s * unisonGain;
        }

        float cutoffMod = baseCutoff * std::pow(2.0f, l2 * filterModAmt + fenv * filterEnvAmt);
        filter.setParameters(cutoffMod, filterReso, filterMode);
        sample = filter.process(sample);
        return sample * env * velocity * 0.3f;
    }

private:
    double fs = 44100.0;
    Oscillator osc1, osc2, sub;
    ADSR ampEnv, filterEnv;
    SVFFilter filter;
    LFO lfo1, lfo2;
    float targetFreq = 440.0f, portamentoCurrent = 440.0f, portamentoRate = 0.0f, velocity = 1.0f;
    bool gate = false, active = false;
    float osc2Mix = 0.0f, subMix = 0.0f, noiseMix = 0.0f;
    int subOctave = 1;
    float baseCutoff = 5000.0f, filterReso = 1.0f, filterEnvAmt = 0.0f;
    int filterMode = 0;
    float pitchModAmt = 0.0f, filterModAmt = 0.0f;
    int unisonVoices = 1;
    float unisonDetune = 10.0f;
};

class SynthEngine {
public:
    void prepare(double sr, int maxVoices = 16) {
        voices.resize(maxVoices);
        for (auto& v : voices) v.prepare(sr);
        delayL.prepare(sr); delayR.prepare(sr);
        reverb.prepare(sr); chorusL.prepare(sr); chorusR.prepare(sr);
    }

    void noteOn(int note, float vel) {
        // ИСПРАВЛЕНО: явное приведение типа double к float
        float freq = 440.0f * (float)std::pow(2.0, (note - 69) / 12.0);
        for (auto& v : voices) {
            if (!v.isActive()) { v.noteOn(freq, vel); return; }
        }
        voices[stealIndex].noteOn(freq, vel);
        stealIndex = (stealIndex + 1) % voices.size();
    }

    void noteOff(int) { for (auto& v : voices) v.noteOff(); }
    void allNotesOff() { for (auto& v : voices) v.noteOff(); }
    template<typename F> void applyToAll(F f) { for (auto& v : voices) f(v); }

    void setDelay(float time, float fb, float mix) { delayTime = time; delayFb = fb; delayMix = mix; }
    void setReverb(float room, float damp, float mix) { reverbRoom = room; reverbDamp = damp; reverbMix = mix; }
    void setChorus(float rate, float depth, float mix) { chorusRate = rate; chorusDepth = depth; chorusMix = mix; }
    void setDistortion(int mode, float drive, float mix) { distMode = mode; distDrive = drive; distMix = mix; }
    void setMaster(float gain) { masterGain = gain; }

    void process(float* left, float* right, int numSamples) {
        reverb.setParameters(reverbRoom, reverbDamp, reverbMix);
        for (int i = 0; i < numSamples; ++i) {
            float s = 0.0f;
            for (auto& v : voices) s += v.render();
            float dry = s;
            s = distortion.process(s, distDrive);
            s = dry * (1.0f - distMix) + s * distMix;

            float cL = chorusL.process(s, chorusRate, chorusDepth, chorusMix);
            float cR = chorusR.process(s, chorusRate * 1.03f, chorusDepth, chorusMix);
            float dL = delayL.process(cL, delayTime, delayFb, delayMix);
            float dR = delayR.process(cR, delayTime * 1.02f, delayFb, delayMix);
            float rL = dL, rR = dR;
            reverb.process(rL, rR);
            left[i]  = rL * masterGain;
            right[i] = rR * masterGain;
        }
    }
    void reset() { delayL.reset(); delayR.reset(); reverb.reset(); chorusL.reset(); chorusR.reset(); }

private:
    std::vector<Voice> voices;
    int stealIndex = 0;
    Delay delayL, delayR;
    Reverb reverb;
    Chorus chorusL, chorusR;
    Distortion distortion;
    float delayTime = 0.3f, delayFb = 0.4f, delayMix = 0.0f;
    float reverbRoom = 0.5f, reverbDamp = 0.5f, reverbMix = 0.0f;
    float chorusRate = 1.0f, chorusDepth = 0.3f, chorusMix = 0.0f;
    int distMode = 1;
    float distDrive = 1.0f, distMix = 0.0f;
    float masterGain = 0.8f;
};

} // namespace hunterice
