#include "component/ThreeImager.h"
#include "lib/Shader.h"
#include <mutex>

ThreeImager::ThreeImager(std::shared_ptr<Manager> data) : manager(data) {
    startTimerHz(60);
    setSize(800, 600);

#if JUCE_OPENGL
    if (juce::ComponentPeer *peer = getPeer()) {
        peer->setCurrentRenderingEngine(0);
    }
    _context.setOpenGLVersionRequired(juce::OpenGLContext::openGL4_3);
    _context.setRenderer(this);
    _context.attachTo(*this); // or attachTo(*getTopLevelComponent())
    _context.setContinuousRepainting(true);
#endif

    // shader_program = _context.extensions.glCreateProgram();
    // _context.
    // // juce::OpenGLShaderProgram::addVertexShader(shader_program, Shader::vertex_shader);
    // juce::OpenGLShaderProgram::addFragmentShader(shader_program, Shader::fragment_shader);
    // juce::OpenGLShaderProgram::link(shader_program);

    // projection_matrix_location = _context.extensions.glGetUniformLocation(shader_program, "projectionMatrix");
    // view_matrix_location = _context.extensions.glGetUniformLocation(shader_program, "viewMatrix");
    // position_attribute = _context.extensions.glGetAttribLocation(shader_program, "position");
}

ThreeImager::~ThreeImager() {
    stopTimer();
    _context.detach();
}

void ThreeImager::openGLContextClosing() { shader.reset(); }

void ThreeImager::renderOpenGL() {
    const juce::ScopedLock lock(std::mutex);
    jassert(juce::OpenGLHelpers::isContextActive());

    auto desktopScale = (float)_context.getRenderingScale();

    juce::OpenGLHelpers::clear(juce::Colours::black);

    if (shader.get() == nullptr) {
        return;
    }

    juce::gl::glEnable(juce::gl::GL_DEPTH_TEST);
    juce::gl::glDepthFunc(juce::gl::GL_LESS);
    juce::gl::glEnable(juce::gl::GL_BLEND);
    juce::gl::glBlendFunc(juce::gl::GL_SRC_ALPHA, juce::gl::GL_ONE_MINUS_SRC_ALPHA);
    juce::gl::glActiveTexture(juce::gl::GL_TEXTURE0);

    if (!_context.isCoreProfile()) {
        juce::gl::glEnable(juce::gl::GL_TEXTURE_2D);
    }

    juce::gl::glViewport(0, 0, juce::roundToInt(desktopScale * getWidth()), juce::roundToInt(desktopScale * getHeight()));

    _context.extensions.glUseProgram(shader_program);

    juce::Matrix3D<float> projection_matrix = juce::Matrix3D<float>::fromFrustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);
    juce::Matrix3D<float> view_matrix = juce::Matrix3D<float>::fromTranslation({0.0f, 0.0f, -5.0f});
    _context.extensions.glUniformMatrix4fv(projection_matrix_location, 1, juce::gl::GL_FALSE, projection_matrix.mat);
    _context.extensions.glUniformMatrix4fv(view_matrix_location, 1, juce::gl::GL_FALSE, view_matrix.mat);

    for (int i = 0; i < FFTConstants::FFT_LENGTH; ++i) {
        float x = fft_data[3][i];
        float y = fft_data[2][i];
        float z = fft_freq[i];

        float vertices[] = {x, y, z};
        _context.extensions.glVertexAttribPointer(position_attribute, 3, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, 0, vertices);
        _context.extensions.glEnableVertexAttribArray(position_attribute);

        // _context.extensions.glDrawArrays(juce::gl::GL_POINTS, 0, 1);
    }
}

juce::Matrix3D<float> ThreeImager::getViewMatrix() {
    const juce::ScopedLock lock(std::mutex);
    auto view_matrix = juce::Matrix3D<float>::fromTranslation({0.0f, 0.0f, -5.0f}) * orientation.getRotationMatrix();
    auto rotation_matrix = juce::Matrix3D<float>::rotation({rotation, rotation, -0.3f});

    return view_matrix * rotation_matrix;
}

void ThreeImager::paint(juce::Graphics &) {}

void ThreeImager::resized() { setBounds(0, 0, getWidth(), getHeight()); }

void ThreeImager::timerCallback() {
    if (is_next_block_drawable) {
        is_next_block_drawable = false;
        getDataForPaint();
        is_next_block_drawable = true;
        repaint();
    }
}

void ThreeImager::getDataForPaint() {
    fft_data = manager->getFFTResult();
    fft_freq = manager->getFFTFreqs();
}

void ThreeImager::newOpenGLContextCreated() {}
