#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
void check(bool ok, const char* message)
{
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
const imagine::Band& strongest(const imagine::Snapshot& s)
{
    return *std::max_element(s.bands.begin(), s.bands.begin()+s.numBands, [](const auto& a, const auto& b) { return a.levelDb < b.levelDb; });
}
double totalPower(const imagine::Snapshot& s)
{
    double p = 0;
    for (const auto& b : s.bands) if (b.active) p += std::pow(10.0, b.levelDb / 10.0);
    return p;
}
imagine::Snapshot tone(double sr, float gainR, double phase = 0, double frequency = 1000)
{
    imagine::Analyzer a; a.reset(sr);
    for (int i = 0; i < static_cast<int>(sr); ++i)
    {
        const double t = juce::MathConstants<double>::twoPi * frequency * i / sr;
        a.push(static_cast<float>(0.5 * std::sin(t)), static_cast<float>(gainR * 0.5 * std::sin(t + phase)));
    }
    return a.snapshot();
}
void coreTests()
{
    for (const int count : {32,64,128})
    {
        imagine::Analyzer a; a.reset(48000,13,250,count);
        for (int i=0;i<24000;++i) {const float v=0.5f*std::sin(static_cast<float>(juce::MathConstants<double>::twoPi*1000*i/48000)); a.push(v,v);}
        check(a.snapshot().numBands==count && std::abs(10*std::log10(totalPower(a.snapshot()))+9.0309)<0.02,"Band densities preserve calibrated integrated power");
        for (int i=1;i<count;++i) check(a.snapshot().bands[static_cast<size_t>(i-1)].high==a.snapshot().bands[static_cast<size_t>(i)].low,"Band boundaries remain contiguous at every density");
    }
    for (int order=11;order<=15;++order)
    {
        imagine::Analyzer configurable; configurable.reset(48000,order,50);
        const int n=1<<order;
        for (int i=0;i<n*3;++i)
        {
            const float sample=static_cast<float>(0.5*std::sin(juce::MathConstants<double>::twoPi*1000*i/48000));
            configurable.push(sample,sample);
        }
        check(configurable.snapshot().fftPoints==n && configurable.snapshot().frames==9,"Every FFT size uses its complete window and 75% overlap");
        check(std::abs(10*std::log10(totalPower(configurable.snapshot()))+9.0309)<0.02,"All selectable FFT sizes preserve calibrated RMS power");
        configurable.reset(96000,order,1000);
        check(configurable.snapshot().frames==0 && configurable.snapshot().fftPoints==n,"FFT reconfiguration clears measurement history");
    }
    for (int axis=0;axis<3;++axis)
    {
        const auto c=imagine::Camera::aligned(axis);
        const std::array<imagine::Vec3,3> directions {{{1,0,0},{0,1,0},{0,0,1}}};
        const auto v=c.transform(directions[static_cast<size_t>(axis)]);
        check(std::abs(v.x)<1.e-6f && std::abs(v.y)<1.e-6f && std::abs(v.z)>0.9999f,
            "Axis alignment removes exactly the selected data dimension from the screen");
        check(c.alignedAxis()==axis,"Restored camera identifies its alignment");
    }
    auto orbit=imagine::Camera::home();
    const auto original=orbit.transform({0.3f,0.7f,-0.4f});
    orbit.orbit(0,0,0.4f,0.2f);
    const auto rotated=orbit.transform({0.3f,0.7f,-0.4f});
    check(std::abs(rotated.x-original.x)>0.01f,"Trackball changes the view");
    check(std::abs(rotated.x*rotated.x+rotated.y*rotated.y+rotated.z*rotated.z-0.74f)<1.e-5f,"Rotation preserves spatial lengths");
    for (int i=0;i<1000;++i) orbit.orbit(0.2f,-0.2f,0.21f,-0.19f);
    check(std::abs(orbit.w*orbit.w+orbit.x*orbit.x+orbit.y*orbit.y+orbit.z*orbit.z-1)<1.e-5f,"Repeated orbit stays normalized");
    orbit.orbit(-2,0,2,0); check(std::isfinite(orbit.w),"Antipodal trackball drag is finite");
    for (const double sr : { 32000., 44100., 48000., 96000., 192000. })
    {
        const auto mono = tone(sr, 1); const auto& m = strongest(mono);
        check(m.active && m.low < 1000 && m.high > 1000, "Tone is in the correct logarithmic band");
        check(std::abs(m.side) < 0.0001f && std::abs(m.correlation - 1) < 0.0001f, "Identical L/R gives zero Side and +1 correlation");
        check(std::abs(10 * std::log10(totalPower(mono)) + 9.0309) < 0.02, "Hann-normalized half-amplitude sine RMS is -9.03 dBFS");
        check(mono.bands[static_cast<size_t>(mono.numBands-1)].high <= sr / 2 + 0.01, "Band limits follow Nyquist");
        const auto inverse = tone(sr, -1); const auto& inv = strongest(inverse);
        check(inv.side > 0.9999f && inv.correlation < -0.9999f, "Inverse stereo gives 100% Side and -1 correlation");
        const auto oneSide = tone(sr, 0); const auto& o = strongest(oneSide);
        check(std::abs(o.side - 0.5f) < 0.0001f && o.balance < -0.9999f && !o.correlationValid, "One-sided signal is distinguished from unrelated stereo");
        const auto quarter = tone(sr, 1, juce::MathConstants<double>::halfPi); const auto& q = strongest(quarter);
        check(std::abs(q.side - 0.5f) < 0.001f && std::abs(q.correlation) < 0.001f, "Quadrature tone gives zero real correlation");
    }
    const auto unequal = tone(48000, 0.5f); const auto& u = strongest(unequal);
    check(std::abs(u.side - 0.1f) < 0.001f && std::abs(u.balance + 0.6f) < 0.001f && u.correlation > 0.999f, "Unequal aligned channels keep correlation +1 with nonzero Side");
    imagine::Analyzer noise; std::mt19937 rng(398); std::uniform_real_distribution<float> dist(-0.25f, 0.25f);
    for (int i = 0; i < 144000; ++i) noise.push(dist(rng), dist(rng));
    double side = 0, corr = 0;
    for (const auto& b : noise.snapshot().bands) { side += b.side; corr += b.correlation; }
    check(std::abs(side / imagine::bandCount - 0.5) < 0.03 && std::abs(corr / imagine::bandCount) < 0.06, "Independent noise approaches 50% Side and zero correlation");
    for (int i=18;i<imagine::bandCount;++i)
    {
        const auto& b=noise.snapshot().bands[static_cast<size_t>(i)];
        const double psd=b.levelDb-10*std::log10(b.high-b.low);
        check(std::abs(psd-10*std::log10(2*0.25*0.25/3/48000))<1.5,"White-noise PSD is calibrated and flat across logarithmic band widths");
    }
    noise.reset(48000);
    for (int i = 0; i < imagine::fftSize; ++i) noise.push(0, 0);
    check(noise.snapshot().frames == 1, "First frame waits for a complete FFT window");
    for (const auto& b : noise.snapshot().bands) check(!b.active && !b.correlationValid && std::isfinite(b.levelDb), "Silence has no invented correlation or invalid level");
    for (int i = 0; i < 16384; ++i) noise.push(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity());
    check(!strongest(noise.snapshot()).active, "Nonfinite input cannot poison analysis");
    noise.reset(44100); check(noise.snapshot().frames == 0 && !strongest(noise.snapshot()).active, "Reset clears previous measurements");
    auto& decay = noise;
    for (int i = 0; i < 44100; ++i) decay.push(std::sin(static_cast<float>(i) * 0.2f), 0);
    for (int i = 0; i < 44100 * 6; ++i) decay.push(0, 0);
    check(!strongest(decay.snapshot()).active, "Silent input releases averaged power to the measurement floor");
}
void save(juce::Component& e, const juce::File& file)
{
    const auto image = e.createComponentSnapshot(e.getLocalBounds());
    auto out = file.createOutputStream(); check(out != nullptr, "Screenshot opens"); out->setPosition(0); out->truncate();
    check(juce::PNGImageFormat().writeImageToStream(image, *out), "Screenshot renders");
}
void pluginTests(const juce::File& directory)
{
    auto storage = std::make_unique<SoundImagineProcessor>(); auto& p = *storage; p.prepareToPlay(48000, 257);
    check(p.getLatencySamples() == 0, "Audio latency is zero");
    juce::AudioBuffer<float> audio(2, 257); juce::MidiBuffer midi;
    std::mt19937 rng(15); std::uniform_real_distribution<float> dist(-1, 1);
    for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < audio.getNumSamples(); ++i) audio.setSample(ch, i, dist(rng));
    const juce::AudioBuffer<float> copy(audio); p.processBlock(audio, midi);
    for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < audio.getNumSamples(); ++i)
        check(audio.getSample(ch, i) == copy.getSample(ch, i), "Stereo is bit-exact passthrough");
    juce::AudioProcessor::BusesLayout mono; mono.inputBuses.add(juce::AudioChannelSet::mono()); mono.outputBuses.add(juce::AudioChannelSet::mono());
    check(p.isBusesLayoutSupported(mono), "Mono buses supported"); mono.outputBuses.set(0, juce::AudioChannelSet::stereo());
    check(!p.isBusesLayoutSupported(mono), "Mismatched buses rejected");
    check(p.language.load()==1,"Japanese is the initial language");
    p.view.store(0); p.floorDb.store(-48); p.language.store(0);
    p.fftOrderSetting.store(12); p.smoothingMs.store(50); p.levelMode.store(1); p.displayBands.store(64);
    auto savedCamera=imagine::Camera::aligned(0); savedCamera.zoom=1.4f; p.saveCamera(savedCamera);
    juce::MemoryBlock state; p.getStateInformation(state);
    auto restoredStorage = std::make_unique<SoundImagineProcessor>(); auto& restored = *restoredStorage;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    check(restored.view.load() == 0 && restored.floorDb.load() == -48, "Settings survive session save/load");
    check(restored.fftOrderSetting.load()==12 && restored.smoothingMs.load()==50 && restored.levelMode.load()==1,"Analysis settings survive session save/load");
    check(restored.displayBands.load()==64,"Display band count survives session save/load");
    check(restored.language.load()==0 && restored.readCamera().alignedAxis()==0 && std::abs(restored.readCamera().zoom-1.4f)<0.001f,
        "Language, orientation and zoom survive session save/load");
    const char invalid[] = "invalid"; restored.setStateInformation(invalid, sizeof(invalid));
    check(restored.view.load() == 0 && restored.floorDb.load() == -48, "Malformed state is ignored");
    p.view.store(1); p.getStateInformation(state); restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    check(restored.view.load()==0 && restored.readCamera().alignedAxis()==2,"Legacy Map state migrates to Z alignment");
    p.view.store(0); p.fftOrderSetting.store(13); p.smoothingMs.store(250); p.levelMode.store(0); p.displayBands.store(32);
    p.prepareToPlay(96000, 257); check(p.readSnapshot().frames == 0 && p.readSnapshot().sampleRate == 96000, "Reprepare clears FIFO and sample rate");
    p.prepareToPlay(48000, 257); p.view.store(0); p.floorDb.store(-72); p.saveCamera(imagine::Camera::home());
    for (int block = 0; block < 130; ++block)
    {
        for (int i = 0; i < 257; ++i)
        {
            const double t = static_cast<double>(block * 257 + i) / 48000;
            const float mid = static_cast<float>(0.25 * std::sin(juce::MathConstants<double>::twoPi * 80 * t) + 0.13 * std::sin(juce::MathConstants<double>::twoPi * 1000 * t));
            const float side = static_cast<float>(0.20 * std::sin(juce::MathConstants<double>::twoPi * 4000 * t));
            audio.setSample(0, i, mid + side + 0.04f * dist(rng));
            audio.setSample(1, i, mid - side + 0.04f * dist(rng));
        }
        p.processBlock(audio, midi); juce::Thread::sleep(2);
    }
    for (int i = 0; i < 200 && p.readSnapshot().frames < 10; ++i) juce::Thread::sleep(5);
    check(p.readSnapshot().frames >= 10 && p.dropped.load() == 0, "Worker analyzes continuous audio without loss");
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    directory.createDirectory();
    for (const auto& size : { juce::Point<int>(900,640), juce::Point<int>(720,520), juce::Point<int>(560,360), juce::Point<int>(1440,1000) })
    {
        editor->setSize(size.x, size.y); juce::MessageManager::getInstance()->runDispatchLoopUntil(40);
        save(*editor, directory.getChildFile("3d-" + juce::String(size.x) + ".png"));
    }
    for (const auto& size : {juce::Point<int>(720,520),juce::Point<int>(560,360)})
    {
        editor->setSize(size.x,size.y); p.saveCamera(imagine::Camera::aligned(1));
        juce::MessageManager::getInstance()->runDispatchLoopUntil(40);
        save(*editor,directory.getChildFile("spectrum-"+juce::String(size.x)+".png"));
    }
    p.saveCamera(imagine::Camera::home()); juce::MessageManager::getInstance()->runDispatchLoopUntil(40);
    editor->setSize(900,700);
    auto* interactive=dynamic_cast<SoundImagineEditor*>(editor.get());
    check(interactive!=nullptr,"Interactive editor is available");
    const auto makeMouse=[&](juce::Point<float> position)
    {
        return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),position,
            juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier),1,0,0,0,0,editor.get(),editor.get(),
            juce::Time::getCurrentTime(),{450,340},juce::Time::getCurrentTime(),1,true);
    };
    for (auto* child : editor->getChildren())
        if (auto* button=dynamic_cast<OverlayButton*>(child); button && button->getButtonText()=="X")
        {
            check(std::abs(button->getAlpha()-0.4f)<0.001f,"Axis controls start translucent");
            button->mouseEnter(makeMouse({10,10})); check(button->getAlpha()==1,"Axis controls become opaque on hover");
            button->mouseExit(makeMouse({10,10})); check(std::abs(button->getAlpha()-0.4f)<0.001f,"Axis controls fade after hover");
        }
    interactive->mouseDown(makeMouse({450,340})); interactive->mouseDrag(makeMouse({510,370}));
    check(std::abs(p.readCamera().x-imagine::Camera::home().x)>0.01f,"Dragging the plot rotates the actual editor camera");
    juce::MouseWheelDetails wheel {}; wheel.deltaY=0.3f;
    interactive->mouseWheelMove(makeMouse({450,340}),wheel);
    check(p.readCamera().zoom>1.2f,"Wheel zoom updates the actual editor camera");
    save(*editor,directory.getChildFile("rotated.png"));
    interactive->mouseDoubleClick(makeMouse({450,340}));
    check(std::abs(p.readCamera().zoom-1)<0.001f && std::abs(p.readCamera().x-imagine::Camera::home().x)<0.001f,
        "Double-click resets orientation and zoom");
    for (auto* child : editor->getChildren())
        if (auto* button=dynamic_cast<juce::TextButton*>(child); button && button->getButtonText()=="X")
        { button->onClick(); check(p.readCamera().alignedAxis()==0,"X alignment control changes the persisted camera"); }
    for (int axis=0;axis<3;++axis)
    {
        p.saveCamera(imagine::Camera::aligned(axis)); p.language.store(1);
        juce::MessageManager::getInstance()->runDispatchLoopUntil(40);
        save(*editor,directory.getChildFile("axis-"+juce::String(axis)+"-ja.png"));
    }
    p.saveCamera(imagine::Camera::home());
    juce::MessageManager::getInstance()->runDispatchLoopUntil(40);
    save(*editor,directory.getChildFile("3d-ja.png"));
    for (auto* child : editor->getChildren())
        if (auto* button=dynamic_cast<juce::TextButton*>(child))
            check(button->getButtonText()!="3D" && button->getButtonText()!="Map","Redundant view switches are removed");
    for (auto* child : editor->getChildren())
        if (auto* button=dynamic_cast<juce::TextButton*>(child); button && button->getButtonText()=="?") button->onClick();
    editor->setSize(720,520); save(*editor,directory.getChildFile("help-ja.png"));
    for (auto* child : editor->getChildren())
        if (auto* button=dynamic_cast<juce::TextButton*>(child); button && button->getButtonText()=="?") button->onClick();
    p.language.store(0); juce::MessageManager::getInstance()->runDispatchLoopUntil(40);
    editor->setSize(900,640); p.saveCamera(imagine::Camera::aligned(2)); juce::MessageManager::getInstance()->runDispatchLoopUntil(40);
    save(*editor, directory.getChildFile("z-view.png"));
    p.levelMode.store(1); juce::MessageManager::getInstance()->runDispatchLoopUntil(40);
    save(*editor,directory.getChildFile("psd.png")); p.levelMode.store(0);
    juce::TextButton* freezeButton = nullptr;
    for (auto* child : editor->getChildren())
        if (auto* button = dynamic_cast<juce::TextButton*>(child); button && button->getButtonText() == "||") freezeButton = button;
    check(freezeButton != nullptr, "Freeze control exists"); freezeButton->onClick();
    const auto graphSize=editor->getBounds();
    for (auto* child : editor->getChildren())
        if (auto* button=dynamic_cast<juce::TextButton*>(child); button && button->getButtonText()=="=") button->onClick();
    juce::DocumentWindow* measurements=nullptr;
    for (int i=0;i<juce::Desktop::getInstance().getNumComponents();++i)
        if (auto* window=dynamic_cast<juce::DocumentWindow*>(juce::Desktop::getInstance().getComponent(i)); window && window->getName()=="All bands") measurements=window;
    check(measurements!=nullptr && editor->getBounds()==graphSize,"All-band window opens without resizing the graph");
    juce::MessageManager::getInstance()->runDispatchLoopUntil(40);
    save(*measurements->getContentComponent(),directory.getChildFile("all-bands.png"));
    p.language.store(1); juce::MessageManager::getInstance()->runDispatchLoopUntil(40);
    save(*measurements->getContentComponent(),directory.getChildFile("all-bands-ja.png"));
    p.language.store(0); juce::MessageManager::getInstance()->runDispatchLoopUntil(40);
    juce::TableListBox* measurementTable=nullptr;
    for (auto* child : measurements->getContentComponent()->getChildren())
        if (auto* table=dynamic_cast<juce::TableListBox*>(child)) measurementTable=table;
    check(measurementTable && measurementTable->getTableListBoxModel()->getNumRows()==p.readSnapshot().numBands,"All-band table contains every measurement point");
    const auto frozenTable=measurements->getContentComponent()->createComponentSnapshot(measurements->getContentComponent()->getLocalBounds());
    const auto frozenImage = editor->createComponentSnapshot(editor->getLocalBounds());
    juce::AudioBuffer<float> silence(2,257); silence.clear();
    for (int block = 0; block < 80; ++block) { p.processBlock(silence,midi); juce::Thread::sleep(2); }
    juce::MessageManager::getInstance()->runDispatchLoopUntil(40);
    const auto heldImage = editor->createComponentSnapshot(editor->getLocalBounds());
    const auto heldTable=measurements->getContentComponent()->createComponentSnapshot(measurements->getContentComponent()->getLocalBounds());
    for (int y=30;y<heldTable.getHeight()-26;++y) for (int x=0;x<heldTable.getWidth()-20;++x)
        check(frozenTable.getPixelAt(x,y)==heldTable.getPixelAt(x,y),"Freeze holds all-band measurements together with the graph");
    measurements->closeButtonPressed(); check(!measurements->isVisible(),"All-band window can be closed independently");
    for (int y = 100; y < 490; ++y) for (int x = 30; x < 860; ++x)
        check(frozenImage.getPixelAt(x,y) == heldImage.getPixelAt(x,y), "Freeze holds measured plot while new audio is analyzed");
    freezeButton->onClick(); juce::MessageManager::getInstance()->runDispatchLoopUntil(40);
    for (auto* child : editor->getChildren())
        if (auto* button = dynamic_cast<juce::TextButton*>(child); button && button->getButtonText() == "?") button->onClick();
    editor->setSize(720,520); save(*editor, directory.getChildFile("help.png"));
    editor.reset();
    juce::AudioBuffer<float> huge(2,70000); huge.clear(); huge.setSample(0,69999,0.8f);
    p.processBlock(huge,midi);
    check(p.dropped.load() > 0 && huge.getSample(0,69999) == 0.8f, "Overload drops analysis samples without touching audio");
    juce::Thread::sleep(50);
    for (int block = 0; block < 80; ++block) { p.processBlock(audio,midi); juce::Thread::sleep(2); }
    juce::Thread::sleep(50);
    check(p.readSnapshot().frames > 0 && strongest(p.readSnapshot()).active, "Analysis resumes after FIFO overload");
    for (const int order : {11,15})
    {
        p.fftOrderSetting.store(order); p.smoothingMs.store(50); juce::Thread::sleep(30);
        for (int block=0;block<180;++block) {p.processBlock(audio,midi); juce::Thread::sleep(2);}
        juce::Thread::sleep(30);
        check(p.readSnapshot().fftPoints==(1<<order) && p.readSnapshot().frames>0,"Worker applies FFT size changes during playback and resumes analysis");
    }
    for (const int bands : {64,128,32})
    {
        p.fftOrderSetting.store(11); p.displayBands.store(bands); juce::Thread::sleep(30);
        for (int block=0;block<40;++block) {p.processBlock(audio,midi); juce::Thread::sleep(2);}
        juce::Thread::sleep(30);
        check(p.readSnapshot().numBands==bands && p.readSnapshot().frames>0,"Worker applies band density changes during playback");
    }
    p.releaseResources();
    auto monoStorage = std::make_unique<SoundImagineProcessor>(); auto& monoProcessor = *monoStorage;
    mono.inputBuses.set(0, juce::AudioChannelSet::mono()); mono.outputBuses.set(0, juce::AudioChannelSet::mono());
    check(monoProcessor.setBusesLayout(mono), "Mono layout applies"); monoProcessor.prepareToPlay(44100, 33);
    juce::AudioBuffer<float> monoAudio(1,33); monoAudio.clear(); monoAudio.setSample(0,2,0.7f); monoProcessor.processBlock(monoAudio,midi);
    check(monoAudio.getSample(0,2) == 0.7f, "Mono is bit-exact passthrough");
    monoProcessor.releaseResources();
}
void vstTests(const juce::String& path)
{
    juce::AudioPluginFormatManager formats;
    auto format = std::make_unique<juce::VST3PluginFormat>();
    juce::OwnedArray<juce::PluginDescription> types;
    format->findAllTypesForFile(types, juce::File(path).getFullPathName());
    check(types.size() == 1 && types[0]->name == "SoundImagine", "Built VST3 scans with the correct identity");
    formats.addFormat(std::move(format)); juce::String error;
    auto instance = formats.createPluginInstance(*types[0],48000,256,error);
    if (!instance) std::cerr << error << '\n';
    check(instance != nullptr, "Built VST3 loads in a plugin host");
    instance->prepareToPlay(48000,256);
    juce::AudioBuffer<float> audio(2,256); audio.clear(); audio.setSample(0,10,0.75f); audio.setSample(1,13,-0.2f);
    juce::MidiBuffer midi; instance->processBlock(audio,midi);
    check(audio.getSample(0,10) == 0.75f && audio.getSample(1,13) == -0.2f && audio.getSample(0,13) == 0,
        "Actual VST3 wrapper passes stereo audio unchanged");
    check(instance->getLatencySamples() == 0, "Actual VST3 reports zero latency");
    juce::MemoryBlock state; instance->getStateInformation(state);
    check(state.getSize() > 0, "Actual VST3 saves its state"); instance->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    for (int i = 0; i < 2; ++i)
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor(instance->createEditorAndMakeActive());
        check(editor != nullptr, "Actual VST3 editor opens and reopens");
        editor->setVisible(false); editor->addToDesktop(0);
        juce::MessageManager::getInstance()->runDispatchLoopUntil(30);
        check(editor->getPeer() != nullptr && editor->getWidth() >= 720 && editor->getHeight() >= 520,
            "VST3 view attaches to a native host window with a valid size");
    }
    instance->releaseResources(); instance->prepareToPlay(44100,64); instance->releaseResources();
}
}
int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    coreTests(); std::cout << "DSP signal checks passed." << std::endl;
    pluginTests(juce::File(argc > 1 ? argv[1] : "verification"));
    if (argc > 2) vstTests(argv[2]);
    std::cout << "All analysis, passthrough, state, lifecycle, freeze, render and VST3 host checks passed.\n";
}
