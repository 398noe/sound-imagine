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

    shader = _context.extensions.glCreateProgram();
    juce::OpenGLShaderProgram::addVertexShader(shader, Shader::vertex_shader);
    juce::OpenGLShaderProgram::addFragmentShader(shader, Shader::fragment_shader);
    juce::OpenGLShaderProgram::link(shader);

    projection_matrix_location = _context.extensions.glGetUniformLocation(shader, "projectionMatrix");
    view_matrix_location = _context.extensions.glGetUniformLocation(shader, "viewMatrix");
    position_attribute = _context.extensions.glGetAttribLocation(shader, "position");
}

ThreeImager::~ThreeImager() {
    stopTimer();
    _context.extensions.glDeleteProgram(shader);
}

void ThreeImager::paint(juce::Graphics &) {
    juce::OpenGLHelpers::clear(juce::Colours::black);

    _context.extensions.glUseProgram(shader);

    juce::Matrix3D<float> projection_matrix = juce::Matrix3D<float>::fromFrustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);
    juce::Matrix3D<float> viewMatrix = juce::Matrix3D<float>::fromTranslation({0.0f, 0.0f, -5.0f});

    _context.extensions.glUniformMatrix4fv(projection_matrix_location, 1, juce::gl::GL_FALSE, projection_matrix.mat);
    _context.extensions.glUniformMatrix4fv(view_matrix_location, 1, juce::gl::GL_FALSE, viewMatrix.mat);

    for (int i = 0; i < FFTConstants::FFT_LENGTH; ++i) {
        float x = fft_data[3][i];
        float y = fft_data[2][i];
        float z = fft_freq[i];

        float verticles[] = {x, y, z};
        _context.extensions.glVertexAttribPointer(position_attribute, 3, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, 0, verticles);
        _context.extensions.glEnableVertexAttribArray(position_attribute);

        _context.extensions.glDrawArrays(juce::gl::GL_POINTS, 0, 1);
    }
}

void ThreeImager::resized() { setBounds(0, 0, getWidth(), getHeight()); }

void ThreeImager::timerCallback() {
    if (_context.isActive() && this->is_next_block_drawable) {
        this->is_next_block_drawable = false;
        getDataForPaint();
        this->is_next_block_drawable = true;
        repaint();
    }
}

void ThreeImager::getDataForPaint() {
    fft_data = manager->getFFTResult();
    fft_freq = manager->getFFTFreqs();
}

void ThreeImager::newOpenGLContextCreated() {}

void ThreeImager::renderOpenGL() {}

void ThreeImager::openGLContextClosing() {}