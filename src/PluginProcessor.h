#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Analyzer.h"
#include <mutex>

class SoundImagineProcessor final : public juce::AudioProcessor, private juce::Thread
{
public:
    SoundImagineProcessor();
    ~SoundImagineProcessor() override;
    void prepareToPlay(double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "SoundImagine"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    imagine::Snapshot readSnapshot();
    std::atomic<int> view { 0 }, floorDb { -72 };
    std::atomic<std::uint64_t> dropped { 0 };
private:
    void run() override;
    static constexpr int capacity = 32768;
    juce::AbstractFifo fifo { capacity };
    std::array<float, capacity> left {}, right {};
    imagine::Analyzer analyzer;
    std::mutex snapshotMutex;
    imagine::Snapshot published;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundImagineProcessor)
};
