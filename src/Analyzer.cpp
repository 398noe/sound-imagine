#include "Analyzer.h"
#include <cmath>

namespace imagine
{
Analyzer::Analyzer()
{
    reset(48000);
}
void Analyzer::reset(double sr, int order, int smoothingMs)
{
    order = std::clamp(order,11,15);
    if (order != currentOrder) { fft=std::make_unique<juce::dsp::FFT>(order); currentOrder=order; }
    size=1<<order; step=size/4;
    result = {}; result.sampleRate = std::isfinite(sr) && sr >= 8000 ? sr : 48000;
    result.fftPoints=size;
    result.averagingMs=std::clamp(smoothingMs,50,1000);
    window.resize(static_cast<size_t>(size)); windowEnergy=0;
    for (int i=0;i<size;++i)
    {
        window[static_cast<size_t>(i)]=static_cast<float>(0.5-0.5*std::cos(juce::MathConstants<double>::twoPi*i/size));
        windowEnergy+=window[static_cast<size_t>(i)]*window[static_cast<size_t>(i)];
    }
    ringL.assign(static_cast<size_t>(size),0); ringR.assign(static_cast<size_t>(size),0);
    spectrumL.assign(static_cast<size_t>(2*size),0); spectrumR.assign(static_cast<size_t>(2*size),0); powers.fill({});
    position = collected = hop = 0;
    retention = std::exp(-step / (result.sampleRate * result.averagingMs*0.001));
    const double upper = std::min(20000.0, result.sampleRate / 2);
    for (int b = 0; b < bandCount; ++b)
    {
        auto& band = result.bands[static_cast<size_t>(b)];
        band.low = static_cast<float>(20 * std::pow(upper / 20, static_cast<double>(b) / bandCount));
        band.high = static_cast<float>(20 * std::pow(upper / 20, static_cast<double>(b + 1) / bandCount));
    }
}
bool Analyzer::push(float l, float r)
{
    ringL[static_cast<size_t>(position)] = std::isfinite(l) ? l : 0;
    ringR[static_cast<size_t>(position)] = std::isfinite(r) ? r : 0;
    position = (position + 1) % size;
    if (collected < size) { ++collected; if (collected < size) return false; hop = step; }
    else ++hop;
    if (hop < step) return false;
    hop = 0; analyse(); return true;
}
void Analyzer::analyse()
{
    std::fill(spectrumL.begin(),spectrumL.end(),0.f); std::fill(spectrumR.begin(),spectrumR.end(),0.f);
    for (int i = 0; i < size; ++i)
    {
        const auto idx = static_cast<size_t>((position + i) % size);
        spectrumL[static_cast<size_t>(i)] = ringL[idx] * window[static_cast<size_t>(i)];
        spectrumR[static_cast<size_t>(i)] = ringR[idx] * window[static_cast<size_t>(i)];
    }
    fft->performRealOnlyForwardTransform(spectrumL.data(), true);
    fft->performRealOnlyForwardTransform(spectrumR.data(), true);
    std::array<Power, bandCount> current {};
    const double binHz = result.sampleRate / size;
    const double scale = 1.0 / (size * windowEnergy);
    for (int k = 1; k <= size / 2; ++k)
    {
        const auto idx = static_cast<size_t>(2 * k);
        const double lr = spectrumL[idx], li = spectrumL[idx + 1], rr = spectrumR[idx], ri = spectrumR[idx + 1];
        const double factor = scale * (k == size / 2 ? 1.0 : 2.0);
        for (int b = 0; b < bandCount; ++b)
        {
            const auto& band = result.bands[static_cast<size_t>(b)];
            const double overlap = std::max(0.0, std::min(static_cast<double>(band.high), (k + 0.5) * binHz)
                - std::max(static_cast<double>(band.low), (k - 0.5) * binHz));
            const double cellWidth = k == size / 2 ? binHz * 0.5 : binHz;
            const double weight = factor * overlap / cellWidth;
            auto& p = current[static_cast<size_t>(b)];
            p.left += (lr * lr + li * li) * weight;
            p.right += (rr * rr + ri * ri) * weight;
            p.cross += (lr * rr + li * ri) * weight;
        }
    }
    for (int b = 0; b < bandCount; ++b)
    {
        const auto idx = static_cast<size_t>(b);
        auto& p = powers[idx]; const auto& c = current[idx]; auto& band = result.bands[idx];
        const double a = result.frames == 0 ? 0 : retention;
        p.left = a * p.left + (1 - a) * c.left;
        p.right = a * p.right + (1 - a) * c.right;
        p.cross = a * p.cross + (1 - a) * c.cross;
        const double sum = p.left + p.right;
        band.levelDb = static_cast<float>(10 * std::log10(std::max(1.0e-9, sum / 2)));
        band.active = sum / 2 > 1.0e-9;
        band.side = band.active ? static_cast<float>(std::clamp((sum - 2 * p.cross) / (2 * sum), 0.0, 1.0)) : 0;
        band.balance = band.active ? static_cast<float>((p.right - p.left) / sum) : 0;
        band.correlationValid = band.active && p.left > 1.0e-12 && p.right > 1.0e-12;
        band.correlation = band.correlationValid ? static_cast<float>(std::clamp(p.cross / std::sqrt(p.left * p.right), -1.0, 1.0)) : 0;
    }
    ++result.frames;
}
}
