/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SoundImagineAudioProcessor::SoundImagineAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
                         )
#endif
    , manager(std::make_shared<Manager>())
{
}

SoundImagineAudioProcessor::~SoundImagineAudioProcessor()
{
}

//==============================================================================
const juce::String SoundImagineAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SoundImagineAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool SoundImagineAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool SoundImagineAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double SoundImagineAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SoundImagineAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int SoundImagineAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SoundImagineAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String SoundImagineAudioProcessor::getProgramName(int index)
{
    return {};
}

void SoundImagineAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

//==============================================================================
void SoundImagineAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void SoundImagineAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SoundImagineAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

        // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void SoundImagineAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto *channelData = buffer.getReadPointer(channel);

        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            manager->addAudioSample(channelData[i], channel);
            if (manager->isAudioBufferReady()) {
                manager->calculateFFT();
                manager->setFFTResult();
            }
        }
    }
}

//==============================================================================
bool SoundImagineAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *SoundImagineAudioProcessor::createEditor()
{
    return new SoundImagineAudioProcessorEditor(*this);
}

//==============================================================================
void SoundImagineAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SoundImagineAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new SoundImagineAudioProcessor();
}

// void SoundImagineAudioProcessor::pushSample(float sample)
// {
//     if (buffer_index == SharedAudioData::FFT_SIZE) {
//         if (!SharedAudioData::is_next_fft_block_ready) {
//             juce::zeromem(fft_buffer, sizeof(fft_buffer));
//             memcpy(fft_buffer, audio_buffer, sizeof(audio_buffer));
//             SharedAudioData::is_next_fft_block_ready = true;
//         }
//         buffer_index = 0;
//     }
//     audio_buffer[buffer_index++] = sample;
// }

// void SoundImagineAudioProcessor::performFFT()
// {
//     try
//     {
//     }
//     catch (std::exception &e)
//     {
//         std::cerr << "Error: " << e.what() << std::endl;
//     }
// }

std::shared_ptr<Manager> SoundImagineAudioProcessor::getManager() const noexcept
{
    return manager;
}
