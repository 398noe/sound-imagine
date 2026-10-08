#include "PluginEditor.h"
#include <cmath>

namespace
{
const juce::Colour background { 0xff11191f }, ink { 0xffedf3f4 }, muted { 0xff93a8b0 }, grid { 0xff2c3b43 };
const juce::Colour mint { 0xff79d9bc }, coral { 0xffff8e80 }, amber { 0xffe1c68c };
juce::String hz(float f) { return f >= 1000 ? juce::String(f / 1000, f < 10000 ? 1 : 0) + "k" : juce::String(juce::roundToInt(f)); }
void text(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size = 13, juce::Colour colour = muted,
          juce::Justification align = juce::Justification::centredLeft)
{
    g.setColour(colour); g.setFont(juce::FontOptions(size)); g.drawText(s, r, align);
}
juce::Colour bandColour(const imagine::Band& b)
{
    if (!b.correlationValid) return muted;
    return b.correlation >= 0 ? amber.interpolatedWith(mint, b.correlation) : amber.interpolatedWith(coral, -b.correlation);
}
}
SoundImagineEditor::SoundImagineEditor(SoundImagineProcessor& p) : AudioProcessorEditor(&p), processor(p)
{
    setOpaque(true); setResizable(true, true); setResizeLimits(720, 520, 1440, 1000);
    for (auto* button : { &threeD, &map, &freeze, &help })
    {
        addAndMakeVisible(button);
        button->setColour(juce::TextButton::buttonColourId, grid);
        button->setColour(juce::TextButton::buttonOnColourId, mint.withAlpha(0.25f));
        button->setColour(juce::TextButton::textColourOffId, ink);
        button->setColour(juce::TextButton::textColourOnId, mint);
    }
    threeD.onClick = [this] { processor.view.store(0); repaint(); };
    map.onClick = [this] { processor.view.store(1); repaint(); };
    freeze.onClick = [this] { frozen = !frozen; freeze.setToggleState(frozen, juce::dontSendNotification); repaint(); };
    help.onClick = [this] { showHelp = !showHelp; repaint(); };
    addAndMakeVisible(range); range.addItem("Floor -48 dB", 1); range.addItem("Floor -72 dB", 2); range.addItem("Floor -90 dB", 3);
    range.setColour(juce::ComboBox::backgroundColourId, grid); range.setColour(juce::ComboBox::textColourId, ink);
    range.setColour(juce::ComboBox::outlineColourId, grid);
    range.onChange = [this] { processor.floorDb.store(range.getSelectedId() == 1 ? -48 : range.getSelectedId() == 3 ? -90 : -72); repaint(); };
    data = processor.readSnapshot(); timerCallback(); setSize(900, 640); startTimerHz(30);
}
SoundImagineEditor::~SoundImagineEditor() { stopTimer(); }
void SoundImagineEditor::resized()
{
    const int x = getWidth() - 352;
    threeD.setBounds(x, 24, 44, 28); map.setBounds(x + 48, 24, 48, 28);
    range.setBounds(x + 104, 24, 124, 28); freeze.setBounds(x + 236, 24, 72, 28); help.setBounds(x + 316, 24, 28, 28);
    plot = { 78.0f, 140.0f, static_cast<float>(getWidth() - 164), static_cast<float>(getHeight() - 324) };
}
void SoundImagineEditor::timerCallback()
{
    if (!frozen)
    {
        const auto next = processor.readSnapshot();
        data = next;
        stale = data.frames == 0 || juce::Time::getMillisecondCounterHiRes() - data.capturedMs > 500;
    }
    threeD.setToggleState(processor.view.load() == 0, juce::dontSendNotification);
    map.setToggleState(processor.view.load() == 1, juce::dontSendNotification);
    range.setSelectedId(processor.floorDb.load() == -48 ? 1 : processor.floorDb.load() == -90 ? 3 : 2, juce::dontSendNotification);
    repaint();
}
juce::Point<float> SoundImagineEditor::project(float f, float side, float level) const
{
    const float upper = data.bands.back().high;
    const float x = std::clamp(std::log(f / 20) / std::log(upper / 20), 0.0f, 1.0f);
    const float floor = static_cast<float>(processor.floorDb.load());
    const float y = std::clamp((level - floor) / -floor, 0.0f, 1.0f);
    if (processor.view.load() == 1) return { plot.getX() + x * plot.getWidth(), plot.getBottom() - side * plot.getHeight() };
    return { plot.getX() + (x * 0.80f + side * 0.16f) * plot.getWidth(),
        plot.getBottom() - (y * 0.70f + side * 0.25f) * plot.getHeight() };
}
void SoundImagineEditor::drawPlot(juce::Graphics& g)
{
    const bool isMap = processor.view.load() == 1;
    const float floor = static_cast<float>(processor.floorDb.load()), upper = data.bands.back().high;
    auto line = [&g](juce::Point<float> a, juce::Point<float> b, juce::Colour c, float thickness = 1) { g.setColour(c); g.drawLine({ a, b }, thickness); };
    for (float f : { 20.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f, 20000.f })
    {
        if (f > upper) continue;
        const auto a = project(f, 0, floor), b = project(f, 1, floor);
        line(a, b, grid);
        if (!isMap) line(b, project(f, 1, 0), grid.withAlpha(0.6f));
        text(g, hz(f), { a.x - 24, a.y + 12, 48, 18 }, 11, muted, juce::Justification::centred);
    }
    for (float s : { 0.f, 0.25f, 0.5f, 0.75f, 1.f })
    {
        const auto a = project(20, s, floor), b = project(upper, s, floor);
        line(a, b, grid);
        text(g, juce::String(juce::roundToInt(s * 100)) + "%", { b.x + 8, b.y - 9, 44, 18 }, 11);
    }
    if (!isMap)
    {
        for (float db = floor; db <= 0; db += 12)
        {
            const auto a = project(20, 0, db), b = project(upper, 0, db);
            line(a, b, grid.withAlpha(0.7f));
            text(g, juce::String(juce::roundToInt(db)), { a.x - 46, a.y - 9, 36, 18 }, 11, muted, juce::Justification::centredRight);
        }
        line(project(20, 0, floor), project(20, 0, 0), muted);
        text(g, "dBFS", { plot.getX() - 54, plot.getY() + plot.getHeight() * 0.3f - 28, 48, 20 }, 11);
    }
    text(g, "FREQUENCY / Hz", { plot.getX(), plot.getBottom() + 38, 150, 20 }, 11);
    text(g, "SIDE ENERGY", { plot.getRight() - 95, plot.getY() - 26, 160, 20 }, 11);
    juce::Point<float> previous;
    bool havePrevious = false;
    for (int i = 0; i < imagine::bandCount; ++i)
    {
        const auto& b = data.bands[static_cast<size_t>(i)];
        if (!b.active || b.levelDb < floor) { havePrevious = false; continue; }
        const float centre = std::sqrt(b.low * b.high);
        const auto p = project(centre, b.side, b.levelDb);
        const auto colour = bandColour(b);
        if (!isMap) line(project(centre, b.side, floor), p, colour.withAlpha(0.22f));
        if (havePrevious) line(previous, p, colour.withAlpha(0.5f), 1.5f);
        const float radius = isMap ? 3.0f + 7.0f * std::clamp((b.levelDb - floor) / -floor, 0.f, 1.f) : 4.f;
        g.setColour(colour); g.fillEllipse(p.x - radius, p.y - radius, radius * 2, radius * 2);
        if (i == selected) { g.setColour(ink); g.drawEllipse(p.x - radius - 4, p.y - radius - 4, radius * 2 + 8, radius * 2 + 8, 1.5f); }
        previous = p; havePrevious = true;
    }
}
void SoundImagineEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    text(g, "sound imagine", { 26, 22, 240, 34 }, 26, ink);
    text(g, "See the spectrum. Understand the stereo.", { 27, 60, 350, 22 }, 13);
    text(g, processor.view.load() == 0 ? "Frequency / Side energy / Band RMS" : "Frequency / Side energy   -   Dot size = band RMS",
        { 27, 104, 470, 20 }, 13, ink);
    const juce::String status = frozen ? "FROZEN" : stale ? "NO RECENT AUDIO" : "LIVE";
    text(g, status, { static_cast<float>(getWidth() - 218), 65, 190, 20 }, 11, frozen ? amber : stale ? muted : mint, juce::Justification::centredRight);
    drawPlot(g);
    const float y = static_cast<float>(getHeight() - 114);
    g.setColour(grid); g.drawHorizontalLine(static_cast<int>(y - 16), 26, static_cast<float>(getWidth() - 26));
    const auto& band = data.bands[static_cast<size_t>(selected)];
    text(g, hz(band.low) + " - " + hz(band.high) + " Hz", { 27, y, 210, 26 }, 20, ink);
    const float column = static_cast<float>(getWidth() - 270) / 4;
    const bool valid = band.active && data.frames > 0;
    const std::array<juce::String, 4> labels { "BAND RMS", "SIDE ENERGY", "L/R CORRELATION", "L/R BALANCE" };
    const std::array<juce::String, 4> values {
        valid ? juce::String(band.levelDb, 1) + " dBFS" : "--",
        valid ? juce::String(band.side * 100, 1) + "%" : "--",
        valid && band.correlationValid ? juce::String(band.correlation, 2) : "--",
        !valid ? "--" : std::abs(band.balance) < 0.01f ? "Centre" : juce::String(std::abs(band.balance) * 100, 0) + "% " + (band.balance < 0 ? "L" : "R") };
    for (int i = 0; i < 4; ++i)
    {
        const float x = 250 + static_cast<float>(i) * column;
        text(g, labels[static_cast<size_t>(i)], { x, y, column, 18 }, 10);
        text(g, values[static_cast<size_t>(i)], { x, y + 22, column, 24 }, 17, i == 2 ? bandColour(band) : ink);
    }
    text(g, "Point / hover to inspect", { 27, y + 32, 215, 20 }, 11);
    const juce::String mono = !valid ? "Mono sum: --" : band.side > 0.9999f ? "Mono sum: cancellation" : "Mono sum: " + juce::String(10 * std::log10(1 - band.side), 1) + " dB";
    text(g, mono, { 27, y + 55, 230, 20 }, 11, amber);
    text(g, "CORRELATION", { 250, y + 59, 90, 20 }, 10);
    text(g, "-1 inverse", { 346, y + 59, 95, 20 }, 11, coral);
    text(g, "0 unrelated", { 446, y + 59, 110, 20 }, 11, amber);
    text(g, "+1 aligned", { 560, y + 59, 110, 20 }, 11, mint);
    text(g, juce::String(data.sampleRate / 1000, 1) + " kHz / " + juce::String(data.sampleRate / imagine::fftSize, 1) + " Hz bins",
        { static_cast<float>(getWidth() - 240), static_cast<float>(getHeight() - 28), 213, 18 }, 10, muted, juce::Justification::centredRight);
    if (processor.dropped.load() != 0)
        text(g, "Analysis overload: " + juce::String(static_cast<juce::int64>(processor.dropped.load())) + " samples skipped", { 27, static_cast<float>(getHeight() - 28), 400, 18 }, 11, coral);
    if (showHelp)
    {
        auto box = getLocalBounds().toFloat().reduced(50, 95);
        g.setColour(background.withAlpha(0.98f)); g.fillRoundedRectangle(box, 10);
        g.setColour(grid); g.drawRoundedRectangle(box, 10, 1);
        box = box.reduced(24);
        text(g, "READING THE STEREO", box.removeFromTop(34), 20, ink);
        const std::array<juce::String, 9> lines {
            "0% Side: identical L/R. 50%: equal Mid and Side. 100%: pure inverse L/R.",
            "Side energy is not a stereo / mono switch. One-sided audio also reads 50%.",
            "Colour shows L/R correlation: green +1, sand 0, coral -1.",
            "Negative correlation indicates possible loss when summed to mono.",
            "Balance shows the difference in L/R power. One silent channel: correlation --.",
            "Band RMS is integrated energy, not loudness or the height of an FFT peak.",
            "3D: height = RMS. Map: larger dots = stronger bands. Freeze holds the view.",
            "8192-point Hann FFT / 75% overlap / 250 ms power averaging.",
            "Low bands share FFT resolution. Audio passes through unchanged." };
        for (const auto& s : lines) text(g, s, box.removeFromTop(25), 13);
    }
}
void SoundImagineEditor::selectAt(juce::Point<float> p)
{
    float nearest = 28.f;
    for (int i = 0; i < imagine::bandCount; ++i)
    {
        const auto& b = data.bands[static_cast<size_t>(i)];
        const float distance = p.getDistanceFrom(project(std::sqrt(b.low * b.high), b.side, b.levelDb));
        if (distance < nearest) { nearest = distance; selected = i; }
    }
    repaint();
}
void SoundImagineEditor::mouseMove(const juce::MouseEvent& e) { if (!showHelp) selectAt(e.position); }
void SoundImagineEditor::mouseDown(const juce::MouseEvent& e) { if (!showHelp) selectAt(e.position); }
