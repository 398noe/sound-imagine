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

    // set shaders
    createShader();
#endif
}

ThreeImager::~ThreeImager() {
    stopTimer();
    shader.reset();
    _context.detach();
}

void ThreeImager::openGLContextClosing() { shader.reset(); }

void ThreeImager::renderOpenGL() {
    const juce::ScopedLock lock(std::mutex);
    jassert(juce::OpenGLHelpers::isContextActive());

    auto desktop_scale = (float)_context.getRenderingScale();

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

    {
        const juce::ScopedLock lock(std::mutex);
        juce::gl::glViewport(0, 0, juce::roundToInt(desktop_scale * getWidth()), juce::roundToInt(desktop_scale * getHeight()));
    }

    shader->use();

    // reset element buffers
    juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, 0);
    juce::gl::glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER, 0);

    // 描画する座標と視点を設定
    juce::Matrix3D<float> projection_matrix = juce::Matrix3D<float>::fromFrustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);
    juce::Matrix3D<float> view_matrix = juce::Matrix3D<float>::fromTranslation({0.0f, 0.0f, -5.0f});

    _context.extensions.glUniformMatrix4fv(projection_matrix_location, 1, juce::gl::GL_FALSE, projection_matrix.mat);
    _context.extensions.glUniformMatrix4fv(view_matrix_location, 1, juce::gl::GL_FALSE, view_matrix.mat);

    // 点群を描画する
    for (int i = 0; i < FFTConstants::FFT_LENGTH; ++i) {
        float x = fft_data[3][i];
        float y = fft_data[2][i];
        float z = fft_freq[i];

        float vertices[] = {x, y, z};
        _context.extensions.glVertexAttribPointer(position_attribute, 3, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, 0, vertices);
        _context.extensions.glEnableVertexAttribArray(position_attribute);

        juce::gl::glDrawArrays(juce::gl::GL_POINTS, 0, 1);
    }
}

juce::Matrix3D<float> ThreeImager::getProjectionMatrix() {
    auto w = 1.0f / (0.5f + 0.1f);
    auto h = w * getLocalBounds().toFloat().getAspectRatio(false);

    return juce::Matrix3D<float>::fromFrustum(-w, w, -h, h, 4.0f, 30.0f);
}

juce::Matrix3D<float> ThreeImager::getViewMatrix() {
    const juce::ScopedLock lock(std::mutex);
    auto view_matrix = juce::Matrix3D<float>::fromTranslation({0.0f, 0.0f, -5.0f}) * orientation.getRotationMatrix();
    auto rotation_matrix = juce::Matrix3D<float>::rotation({rotation, rotation, -0.3f});

    return view_matrix * rotation_matrix;
}

void ThreeImager::paint(juce::Graphics &) {}

void ThreeImager::resized() {
    const juce::ScopedLock lock(std::mutex);
    bounds = getLocalBounds();
    // setBounds(0, 0, getWidth(), getHeight());
}

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

void ThreeImager::createShader() {
    vertex_shader = juce::String(R"(
        attribute vec3 position;
        uniform mat4 projectionMatrix;
        uniform mat4 viewMatrix;
        void main() {
            gl_Position = projectionMatrix * viewMatrix * vec4(position, 1.0);
        }
    )");

    fragment_shader = juce::String(R"(
        void main() {
            gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        }
    )");

    std::unique_ptr<juce::OpenGLShaderProgram> new_shader(new juce::OpenGLShaderProgram(_context));
    new_shader->addVertexShader(this->vertex_shader);
    new_shader->addFragmentShader(this->fragment_shader);
    new_shader->link();

    shader = std::move(new_shader);
    shader->use();
}

void ThreeImager::mouseDrag(const juce::MouseEvent &e) {}

void ThreeImager::mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &d) {}

void ThreeImager::reloadShader() {}