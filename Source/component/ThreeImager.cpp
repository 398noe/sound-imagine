#include "component/ThreeImager.h"
#include "lib/Shader.h"
#include <mutex>

ThreeImager::ThreeImager(std::shared_ptr<Manager> data) : manager(data) {
    startTimerHz(60);
    setSize(800, 600);

    if (juce::ComponentPeer *peer = getPeer()) {
        peer->setCurrentRenderingEngine(0);
    }
    _context.attachTo(*this); // or attachTo(*getTopLevelComponent())
    _context.setRenderer(this);
    _context.setContinuousRepainting(true);
    _context.setComponentPaintingEnabled(false); // call renderOpenGL() instead of paint()
    // _context.setMultisamplingEnabled(true);
}

ThreeImager::~ThreeImager() {
    stopTimer();
    _context.detach();
}

void ThreeImager::openGLContextClosing() { shader.reset(); }

void ThreeImager::resized() {
    setBounds(0, 0, getWidth(), getHeight());
}

void ThreeImager::timerCallback() {
    if (is_next_block_drawable) {
        is_next_block_drawable = false;
        getDataForPaint();
        is_next_block_drawable = true;

        rotation += 0.01f;
        if (rotation > 2 * juce::MathConstants<float>::pi) {
            rotation -= 2 * juce::MathConstants<float>::pi;
        }
        // repaint();
    }
}

void ThreeImager::getDataForPaint() {
    fft_data = manager->getFFTResult();
    fft_freq = manager->getFFTFreqs();
}

void ThreeImager::newOpenGLContextCreated() {
    // generate buffers
    _context.extensions.glGenBuffers(1, &vbo);
    _context.extensions.glGenBuffers(1, &ibo);

    point_cloud = {
        {{0.5f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, // Red
        {{0.0f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},  // Green
        {{0.0f, 0.0f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},   // Blue
    };

    index_buffer = {0, 1, 2};

    _context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vbo);
    _context.extensions.glBufferData(juce::gl::GL_ARRAY_BUFFER, sizeof(Vertex) * point_cloud.size(), point_cloud.data(),
                                     juce::gl::GL_STATIC_DRAW);

    _context.extensions.glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER, ibo);
    _context.extensions.glBufferData(juce::gl::GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * index_buffer.size(), index_buffer.data(),
                                     juce::gl::GL_STATIC_DRAW);

    vertex_shader = juce::String(R"(
        attribute vec3 position;
        attribute vec4 color;
        uniform mat4 projectionMatrix;
        uniform mat4 viewMatrix;
        varying vec4 vColor;
        void main() {
            vColor = color;
            gl_Position = projectionMatrix * viewMatrix * vec4(position, 1.0);
        }
    )");

    fragment_shader = juce::String(R"(
        varying vec4 vColor;
        void main() {
            gl_FragColor = vColor;
        }
    )");

    shader = std::make_unique<juce::OpenGLShaderProgram>(_context);
    // 追加が問題なければtrueを返すのでshaderを使うように設定する
    if (shader->addVertexShader(vertex_shader) && shader->addFragmentShader(fragment_shader) && shader->link()) {
        shader->use();
        projection_matrix_uniform.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "projectionMatrix"));
        view_matrix_uniform.reset(new juce::OpenGLShaderProgram::Uniform(*shader, "viewMatrix"));
    } else {
        jassertfalse;
    }
}

void ThreeImager::renderOpenGL() {
    const juce::ScopedLock lock(render_mutex);
    jassert(juce::OpenGLHelpers::isContextActive());
    // auto desktop_scale = (float)_context.getRenderingScale();

    juce::OpenGLHelpers::clear(juce::Colours::black);
    shader->use();

    auto projection_matrix = getProjectionMatrix();
    auto view_matrix = getViewMatrix();

    if (projection_matrix_uniform != nullptr) {
        projection_matrix_uniform->setMatrix4(projection_matrix.mat, 1, false);
    }
    if (view_matrix_uniform != nullptr) {
        view_matrix_uniform->setMatrix4(view_matrix.mat, 1, false);
    }

    juce::gl::glEnable(juce::gl::GL_DEPTH_TEST);
    // juce::gl::glDepthFunc(juce::gl::GL_LESS);
    // juce::gl::glEnable(juce::gl::GL_BLEND);
    // juce::gl::glBlendFunc(juce::gl::GL_SRC_ALPHA, juce::gl::GL_ONE_MINUS_SRC_ALPHA);
    // juce::gl::glActiveTexture(juce::gl::GL_TEXTURE0);
    // if (!_context.isCoreProfile()) {
    //     juce::gl::glEnable(juce::gl::GL_TEXTURE_2D);
    // }
    // {
    //     const juce::ScopedLock lock(render_mutex);
    //     juce::gl::glViewport(0, 0, juce::roundToInt(desktop_scale * getWidth()), juce::roundToInt(desktop_scale * getHeight()));
    // }

    juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vbo);
    juce::gl::glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER, ibo);

    _context.extensions.glVertexAttribPointer(0, 3, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, sizeof(Vertex), nullptr);
    _context.extensions.glEnableVertexAttribArray(0);

    _context.extensions.glVertexAttribPointer(1, 4, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, sizeof(Vertex), (GLvoid *)(sizeof(float) * 3));
    _context.extensions.glEnableVertexAttribArray(1);

    juce::gl::glDrawElements(juce::gl::GL_TRIANGLES, index_buffer.size(), juce::gl::GL_UNSIGNED_INT, nullptr);

    _context.extensions.glDisableVertexAttribArray(0);
    _context.extensions.glDisableVertexAttribArray(1);
}

juce::Matrix3D<float> ThreeImager::getProjectionMatrix() {
    auto w = 1.0f / (0.5f + 0.1f);
    auto h = w * getLocalBounds().toFloat().getAspectRatio(false);

    return juce::Matrix3D<float>::fromFrustum(-w, w, -h, h, 4.0f, 30.0f);
}

juce::Matrix3D<float> ThreeImager::getViewMatrix() {
    const juce::ScopedLock lock(render_mutex);
    auto view_matrix = juce::Matrix3D<float>::fromTranslation({0.0f, 0.0f, -3.0f});
    auto rotation_matrix = juce::Matrix3D<float>::rotation({rotation, rotation, 0.0f});
    return view_matrix * rotation_matrix;
}

void ThreeImager::paint(juce::Graphics &) {}

void ThreeImager::mouseDrag(const juce::MouseEvent &e) {}

void ThreeImager::mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &d) {}
