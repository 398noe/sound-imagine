#include "component/Imager.h"

Imager::Imager(std::shared_ptr<SharedAudioData> data) : shared_audio_data(data)
{
    // shared_audio_data = std::make_shared<SharedAudioData>();
    startTimerHz(60);
}

Imager::~Imager()
{
    stopTimer();
}

void Imager::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.drawRect(getLocalBounds(), 1);
}

void Imager::resized()
{
}

void Imager::timerCallback()
{
    drawFFTData();
}

void Imager::drawFFTData()
{
    if (shared_audio_data->is_next_fft_block_ready)
    {
        float *fft_data = shared_audio_data->getFFTData();
        // shared_audio_data->is_next_fft_block_ready = false;
    }
}

