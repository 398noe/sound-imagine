#include "component/ThreeImager.h"
#include "lib/Shader.h"

ThreeImager::ThreeImager(std::shared_ptr<Manager> data) : manager(data) {
    startTimerHz(60);
    setSize(800, 600);

    // initialize shader
    shader = loadShader(VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH);
    projection_matrix_location = juce::gl::glGetUniformLocation(shader, "projectionMatrix");
    view_matrix_location = juce::gl::glGetUniformLocation(shader, "viewMatrix");
    position_attribute = juce::gl::glGetAttribLocation(shader, "position");
}

ThreeImager::~ThreeImager() {
    stopTimer();
    juce::gl::glDeleteProgram(shader);
}

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
