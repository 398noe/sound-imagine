#include "PluginProcessor.h"
#include "PluginEditor.h"

SoundImagineProcessor::SoundImagineProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)), Thread("SoundImagine analysis") {}
SoundImagineProcessor::~SoundImagineProcessor() { stopThread(-1); }
void SoundImagineProcessor::prepareToPlay(double sr, int)
{
    stopThread(-1); fifo.reset(); dropped.store(0); analyzer.reset(sr);
    { std::lock_guard lock(snapshotMutex); published = analyzer.snapshot(); }
    setLatencySamples(0); startThread();
}
void SoundImagineProcessor::releaseResources() { stopThread(-1); }
bool SoundImagineProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto channels = layout.getMainInputChannelSet();
    return (channels == juce::AudioChannelSet::mono() || channels == juce::AudioChannelSet::stereo())
        && channels == layout.getMainOutputChannelSet();
}
void SoundImagineProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals guard;
    if (buffer.getNumChannels() == 0) return;
    const auto* l = buffer.getReadPointer(0);
    const auto* r = buffer.getReadPointer(buffer.getNumChannels() > 1 ? 1 : 0);
    int start1, size1, start2, size2;
    fifo.prepareToWrite(buffer.getNumSamples(), start1, size1, start2, size2);
    std::copy_n(l, size1, left.data() + start1); std::copy_n(r, size1, right.data() + start1);
    std::copy_n(l + size1, size2, left.data() + start2); std::copy_n(r + size1, size2, right.data() + start2);
    fifo.finishedWrite(size1 + size2);
    dropped.fetch_add(static_cast<std::uint64_t>(buffer.getNumSamples() - size1 - size2), std::memory_order_relaxed);
}
void SoundImagineProcessor::run()
{
    juce::ScopedNoDenormals guard;
    std::uint64_t previousDrops = 0;
    while (!threadShouldExit())
    {
        const auto losses = dropped.load();
        if (losses != previousDrops)
        {
            fifo.finishedRead(fifo.getNumReady()); analyzer.reset(analyzer.snapshot().sampleRate); previousDrops = losses;
            std::lock_guard lock(snapshotMutex); published = analyzer.snapshot();
        }
        int start1, size1, start2, size2;
        fifo.prepareToRead(2048, start1, size1, start2, size2);
        bool changed = false;
        for (int i = 0; i < size1; ++i) changed = analyzer.push(left[static_cast<size_t>(start1 + i)], right[static_cast<size_t>(start1 + i)]) || changed;
        for (int i = 0; i < size2; ++i) changed = analyzer.push(left[static_cast<size_t>(start2 + i)], right[static_cast<size_t>(start2 + i)]) || changed;
        fifo.finishedRead(size1 + size2);
        if (changed)
        {
            std::lock_guard lock(snapshotMutex); published = analyzer.snapshot();
            published.capturedMs = juce::Time::getMillisecondCounterHiRes();
        }
        if (size1 + size2 == 0) wait(5);
    }
}
imagine::Snapshot SoundImagineProcessor::readSnapshot() { std::lock_guard lock(snapshotMutex); return published; }
imagine::Camera SoundImagineProcessor::readCamera() { std::lock_guard lock(cameraMutex); return camera; }
void SoundImagineProcessor::saveCamera(imagine::Camera c) { c.normalise(); std::lock_guard lock(cameraMutex); camera=c; }
juce::AudioProcessorEditor* SoundImagineProcessor::createEditor() { return new SoundImagineEditor(*this); }
void SoundImagineProcessor::getStateInformation(juce::MemoryBlock& data)
{
    juce::XmlElement xml("SoundImagine"); xml.setAttribute("version", 2);
    xml.setAttribute("view", view.load()); xml.setAttribute("floor", floorDb.load()); xml.setAttribute("language",language.load());
    const auto c=readCamera();
    xml.setAttribute("qw",c.w); xml.setAttribute("qx",c.x); xml.setAttribute("qy",c.y); xml.setAttribute("qz",c.z); xml.setAttribute("zoom",c.zoom);
    copyXmlToBinary(xml, data);
}
void SoundImagineProcessor::setStateInformation(const void* data, int size)
{
    const auto xml = getXmlFromBinary(data, size);
    if (xml && xml->hasTagName("SoundImagine"))
    {
        view.store(juce::jlimit(0, 1, xml->getIntAttribute("view", 0)));
        const int floor = xml->getIntAttribute("floor", -72);
        floorDb.store(floor == -48 || floor == -90 ? floor : -72);
        language.store(juce::jlimit(0,1,xml->getIntAttribute("language",1)));
        auto c=imagine::Camera::home();
        c.w=static_cast<float>(xml->getDoubleAttribute("qw",c.w)); c.x=static_cast<float>(xml->getDoubleAttribute("qx",c.x));
        c.y=static_cast<float>(xml->getDoubleAttribute("qy",c.y)); c.z=static_cast<float>(xml->getDoubleAttribute("qz",c.z));
        c.zoom=static_cast<float>(xml->getDoubleAttribute("zoom",1)); saveCamera(c);
    }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SoundImagineProcessor(); }
