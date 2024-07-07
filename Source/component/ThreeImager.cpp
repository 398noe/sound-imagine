#include "component/ThreeImager.h"
#include "lib/Shader.h"

ThreeImager::ThreeImager(std::shared_ptr<Manager> data) : manager(data) {
    startTimerHz(60);
    setSize(800, 600);

#if JUCE_OPENGL
    _context.setRenderer(this);
    _context.setMultisamplingEnabled(true);
    _context.attachTo(*getTopLevelComponent());

    if (juce::ComponentPeer *peer = getPeer()) {
        peer->setCurrentRenderingEngine(0);
    }
#endif
}

ThreeImager::~ThreeImager() { stopTimer(); }

void ThreeImager::paint(juce::Graphics &g) {}

void ThreeImager::resized() { setBounds(0, 0, getWidth(), getHeight()); }

void ThreeImager::timerCallback() {
    if (this->is_next_block_drawable) {
        this->is_next_block_drawable = false;
        getDataForPaint();
        this->is_next_block_drawable = true;
        repaint();
    }
}

void ThreeImager::getDataForPaint() { fft_data = manager->getFFTResult(); }

void ThreeImager::newOpenGLContextCreated() {}

void ThreeImager::renderOpenGL() {}

void ThreeImager::openGLContextClosing() {}