#include "component/ThreeImager.h"
#include "lib/Shader.h"
#include <mutex>

ThreeImager::ThreeImager(std::shared_ptr<Manager> data) : manager(data) {
    startTimerHz(60);
    setSize(600, 600);
    setOpaque(true);

    _context.attachTo(*this); // or attachTo(*getTopLevelComponent())
    _context.setRenderer(this);
    _context.setContinuousRepainting(true);

    // enable z buffer
    juce::gl::glEnable(juce::gl::GL_DEPTH_TEST);
}

ThreeImager::~ThreeImager() {
    stopTimer();
    _context.detach();
}

void ThreeImager::newOpenGLContextCreated() {
    // set axis
    axis = {
        {{-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, {{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},     // X軸
        {{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}}, {{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},     // Y軸
        {{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}, {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},     // Z軸
        {{0.0f, 0.0f, 0.0f}, {1.0f, 0.5f, 0.0f, 1.0f}},  {{0.707f, 0.707f, 0.0f}, {1.0f, 0.5f, 0.0f, 1.0f}}, // R軸
        {{0.0f, 0.0f, 0.0f}, {0.0f, 0.5f, 1.0f, 1.0f}},  {{-0.707f, 0.707f, 0.0f}, {0.0f, 0.5f, 1.0f, 1.0f}} // L軸
    };

    // ラベルの設定
    label = {
        {{1.1f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}, // X (Mid)
        {{0.0f, 1.1f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}, // Y (Side)
        {{0.0f, 0.0f, 1.1f}, {1.0f, 1.0f, 1.0f, 1.0f}}, // Z (Hz)
        {{0.8f, 0.8f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}, // R
        {{-0.8f, 0.8f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}} // L
    };

    // set vertices
    vertices.reserve(FFTConstants::FFT_LENGTH);
    vertices = {{{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}}};

    this->vertex_buffer = OpenGLShader::VertexBuffer();
    this->vertex_buffer.init(vertices.data(), juce::gl::GL_POINTS);

    vertex_shader = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec4 colour;

        out vec4 frag_colour;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            gl_Position = projection * view * model * vec4(position, 1.0);
            frag_colour = colour;
        }
    )";
    fragment_shader = R"(
        #version 330 core

        in vec4 frag_colour;

        void main() {
            gl_FragColor = frag_colour;
        }
    )";

    shader.reset(new juce::OpenGLShaderProgram(_context));
    if (shader->addVertexShader(vertex_shader) && shader->addFragmentShader(fragment_shader) && shader->link()) {
        shader->use();
    } else {
        jassertfalse;
    }

    updateProjectionMatrix();
}

void ThreeImager::openGLContextClosing() {
    _context.extensions.glDeleteVertexArrays(1, &vao);
    _context.extensions.glDeleteBuffers(1, &vbo);
}

void ThreeImager::renderOpenGL() {
    if (fft_data.empty()) {
        return;
    }
    juce::OpenGLHelpers::clear(juce::Colours::black);

    shader->use();

    shader->setUniformMat4("model", model_matrix.mat, 1, false);           // 回転行列
    shader->setUniformMat4("view", view_matrix.mat, 1, false);             // 表示行列
    shader->setUniformMat4("projection", projection_matrix.mat, 1, false); // 射影行列

    // _context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vao);
    vertex_buffer.bind();
    vertex_buffer.draw();
    // OpenGLShader::Attributes attributes(*shader);
    // attributes.enable();

    // juce::gl::glLineWidth(2.0f);
    // juce::gl::glDrawArrays(juce::gl::GL_LINES, 0, axis.size());

    // juce::gl::glPointSize(10.0f);
    // juce::gl::glDrawArrays(juce::gl::GL_POINTS, axis.size(), label.size());

    // juce::gl::glPointSize(5.0f);
    // juce::gl::glDrawArrays(juce::gl::GL_POINTS, axis.size() + label.size(), vertices.size());

    // _context.extensions.glBindVertexArray(0);
    // attributes.disable();
}

void ThreeImager::mouseDown(const juce::MouseEvent &e) {
    juce::ScopedLock lock(render_lock);
    orientation.mouseDown(e.getPosition());
    updateProjectionMatrix();
    repaint();
}

void ThreeImager::mouseDrag(const juce::MouseEvent &e) {
    juce::ScopedLock lock(render_lock);
    orientation.mouseDrag(e.getPosition());
    updateProjectionMatrix();
    repaint();
}

void ThreeImager::mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) {
    scale += wheel.deltaY;
    scale = juce::jlimit(0.1f, 10.0f, scale);
    updateProjectionMatrix();
    repaint();
}

void ThreeImager::updateProjectionMatrix() {
    auto aspect = (float)getWidth() / (float)getHeight();
    projection_matrix = juce::Matrix3D<float>::fromFrustum(-aspect, aspect, -1.0f, 1.0f, 1.0f, 100.0f);
    view_matrix = juce::Matrix3D<float>::fromTranslation(juce::Vector3D<float>(0.0f, 0.0f, -5.0f / scale));
    model_matrix = orientation.getRotationMatrix();
}

void ThreeImager::resized() {
    const juce::ScopedLock lock(render_lock);
    bounds = getLocalBounds();
    orientation.setViewport(bounds);
    updateProjectionMatrix();
}

void ThreeImager::timerCallback() {
    // ここでFFTのデータを更新する
    fft_data = manager->getFFTResult();
    fft_freq = manager->getFFTFreqs();
    // FFTの結果からverticesを更新する必要がある
    setVertices();
    // openglのバッファを再生成する
    // updateVBO();
    updateProjectionMatrix();
    repaint();
}

void ThreeImager::setVertices() {
    // fft_dataからverticesを更新する
    // verticesをリセットする
    vertices.clear();
    // 一つずつ詰めなおす
    for (int i = 0; i < FFTConstants::FFT_LENGTH; i++) {
        // x は mid, y は side, z は Hz, これらを0~1の範囲に収める
        float x = juce::jmap(fft_data[2][i], -0.5f, 0.5f, -2.0f, 2.0f);
        float y = juce::jmap(fft_data[3][i], -0.5f, 0.5f, -2.0f, 2.0f);
        float z = juce::jmap(fft_freq[i], 0.0f, 24000.0f, -1.0f, 1.0f);
        vertices.push_back({{x, y, z}, {1.0f, 1.0f, 1.0f, 1.0f}});
    }
}

void ThreeImager::setupVBO() {
    _context.extensions.glGenVertexArrays(1, &vao);
    _context.extensions.glBindVertexArray(vao);

    _context.extensions.glGenBuffers(1, &vbo);
    _context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vbo);
    _context.extensions.glBufferData(juce::gl::GL_ARRAY_BUFFER,
                                     sizeof(OpenGLShader::Vertex) * (axis.size() + label.size() + FFTConstants::FFT_LENGTH), nullptr,
                                     juce::gl::GL_DYNAMIC_DRAW);

    _context.extensions.glVertexAttribPointer(0, 3, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, sizeof(OpenGLShader::Vertex), nullptr);
    _context.extensions.glVertexAttribPointer(1, 4, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, sizeof(OpenGLShader::Vertex),
                                              (GLvoid *)(sizeof(float) * 3));

    _context.extensions.glEnableVertexAttribArray(0);
    _context.extensions.glEnableVertexAttribArray(1);

    _context.extensions.glBindVertexArray(0);
}

void ThreeImager::updateVBO() {
    _context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vbo);
    _context.extensions.glBufferSubData(juce::gl::GL_ARRAY_BUFFER, sizeof(OpenGLShader::Vertex) * (axis.size() + label.size()),
                                        sizeof(OpenGLShader::Vertex) * vertices.size(), vertices.data());
}

void ThreeImager::paint(juce::Graphics &g) {
    // zoom, orientationの値をテキストで表示する
    g.setColour(juce::Colours::white);
    g.drawText("Zoom: " + juce::String(scale), 10, 10, 100, 20, juce::Justification::left);
    auto rotation = orientation.getRotationMatrix();
    g.drawText("Orientation: " + juce::String(rotation.mat[0]) + ", " + juce::String(rotation.mat[1]) + ", " +
                   juce::String(rotation.mat[2]),
               10, 30, 300, 20, juce::Justification::left);
}

juce::Matrix3D<float> ThreeImager::getProjectionMatrix() {
    const juce::ScopedLock lock(render_lock);

    auto w = 1.0f / (scale + 0.1f);
    auto h = w * bounds.toFloat().getAspectRatio(false);

    return juce::Matrix3D<float>::fromFrustum(-w, w, -h, h, 4.0f, 30.0f);
}

juce::Matrix3D<float> ThreeImager::getViewMatrix() {
    const juce::ScopedLock lock(render_lock);

    // 表示行列
    auto vm = juce::Matrix3D<float>::fromTranslation({0.0f, 1.0f, -10.0f});
    // 回転行列
    auto rm = orientation.getRotationMatrix();

    return vm * rm;
}

/// shaper
Shaper::Shaper() {}
Shaper::~Shaper() {
    juce::gl::glDeleteBuffers(1, &v_vbo);
    juce::gl::glDeleteVertexArrays(1, &vao);
}

void Shaper::setup(GLenum mode = juce::gl::GL_POINTS) {
    this->mode = mode;
}

void Shaper::addVertex(Shaper::Vertex v) {
    vertices.push_back(v);
}

void Shaper::clearVertex() {
    vertices.clear();
}

void Shaper::updateBuffer() {
    juce::gl::glBindVertexArray(vao);
    juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, v_vbo);
    juce::gl::glBufferData(juce::gl::GL_ARRAY_BUFFER, sizeof(Shaper::Vertex) * vertices.size(), vertices.data(), juce::gl::GL_STATIC_DRAW);

    juce::gl::glVertexAttribPointer(0, 3, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, sizeof(Shaper::Vertex), (GLvoid *)0);
    juce::gl::glEnableVertexAttribArray(0);

    juce::gl::glVertexAttribPointer(1, 4, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, sizeof(Shaper::Vertex), (GLvoid *)(3 * sizeof(GLfloat)));
    juce::gl::glEnableVertexAttribArray(1);

    juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, 0);
    juce::gl::glBindVertexArray(0);
}

void Shaper::draw() {
    juce::gl::glBindVertexArray(vao);
    juce::gl::glDrawArrays(mode, 0, vertices.size());
    juce::gl::glBindVertexArray(0);
}