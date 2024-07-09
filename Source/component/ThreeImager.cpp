#include "component/ThreeImager.h"
#include "lib/Shader.h"
#include <mutex>

ThreeImager::ThreeImager(std::shared_ptr<Manager> data) : manager(data) {
    startTimerHz(60);
    setSize(600, 600);
    setOpaque(true);

    _context.setRenderer(this);
    _context.setContinuousRepainting(true);
    _context.attachTo(*this); // or attachTo(*getTopLevelComponent())

    // enable z buffer
    juce::gl::glEnable(juce::gl::GL_DEPTH_TEST);
    juce::gl::glDepthFunc(juce::gl::GL_LESS);
}

ThreeImager::~ThreeImager() {
    stopTimer();
    _context.detach();
}

void ThreeImager::newOpenGLContextCreated() {
    // compile shader and attach to the context
    ShaderSource source = {};
    source.vertex = R"(
        #version 330 core
        in vec3 position;
        in vec4 colour;

        out vec4 frag_colour;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            gl_Position = projection * view * model * vec4(position, 1.0);
            frag_colour = colour;
        }
    )";
    source.fragment = R"(
        #version 330 core

        in vec4 frag_colour;

        void main() {
            gl_FragColor = frag_colour;
        }
    )";

    initShaderProgram(source);
    initVertices();
}

void ThreeImager::initShaderProgram(const ShaderSource &source) {
    shader.reset(new juce::OpenGLShaderProgram(_context));
    if (shader->addVertexShader(source.vertex) && shader->addFragmentShader(source.fragment) && shader->link()) {
        shader->use();
    } else {
        jassertfalse;
    }
}

void ThreeImager::initVertices() {
    auto id = shader.get()->getProgramID();
    juce::gl::glUseProgram(id);

    // generate and set buffers
    juce::gl::glGenVertexArrays(1, &vao);
    juce::gl::glBindVertexArray(vao);

    juce::gl::glGenBuffers(1, &vbo);
    juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vbo);
    juce::gl::glBufferData(juce::gl::GL_ARRAY_BUFFER, sizeof(Vertex) * FFTConstants::FFT_LENGTH, nullptr, juce::gl::GL_DYNAMIC_DRAW);
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

    _context.extensions.glBindVertexArray(vao);
    _context.extensions.glVertexAttribPointer(0, 3, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, sizeof(Vertex), nullptr); // x, y, z
    _context.extensions.glEnableVertexAttribArray(0);
    _context.extensions.glVertexAttribPointer(1, 4, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, sizeof(Vertex),
                                              (GLvoid *)(sizeof(float) * 3)); // r, g, b, a
    _context.extensions.glEnableVertexAttribArray(1);

    juce::gl::glPointSize(2.0f);
    juce::gl::glDrawArrays(juce::gl::GL_POINTS, 0, FFTConstants::FFT_LENGTH);

    _context.extensions.glDisableVertexAttribArray(0);
    _context.extensions.glDisableVertexAttribArray(1);
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
    juce::ScopedLock lock(render_lock);
    scale += wheel.deltaY;
    scale = juce::jlimit(0.1f, 10.0f, scale);
    updateProjectionMatrix();
    repaint();
}

void ThreeImager::resized() {
    const juce::ScopedLock lock(render_lock);
    bounds = getLocalBounds();
    orientation.setViewport(bounds);
    updateProjectionMatrix();
    repaint();
}

void ThreeImager::updateProjectionMatrix() {
    auto aspect = (float)getWidth() / (float)getHeight();
    projection_matrix = juce::Matrix3D<float>::fromFrustum(-aspect, aspect, -1.0f, 1.0f, 1.0f, 100.0f);
    view_matrix = juce::Matrix3D<float>::fromTranslation(juce::Vector3D<float>(0.0f, 0.0f, -20.0f / scale));
    model_matrix = orientation.getRotationMatrix();
}

void ThreeImager::setVertices() {
    // fft_dataからverticesを更新する
    // verticesをリセットする
    vertices.clear();
    // 一つずつ詰めなおす
    for (int i = 0; i < FFTConstants::FFT_LENGTH; i++) {
        // x は mid, y は side, z は Hz, これらを0~1の範囲に収める
        float x = juce::jmap(fft_data[2][i], -1.0f, 1.0f, -5.0f, 5.0f);
        float y = juce::jmap(fft_data[3][i], -1.0f, 1.0f, -5.0f, 5.0f);
        float z = juce::jmap(fft_freq[i], 0.0f, 24000.0f, -5.0f, 5.0f);
        vertices.push_back({{x, y, z}, {1.0f, 1.0f, 1.0f, 1.0f}});
    }
}

void ThreeImager::updateBuffer() {
    // 更新されたverticesをもとにbufferを更新する
    // _context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vbo);
    juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vbo);
    // _context.extensions.glBufferData(juce::gl::GL_ARRAY_BUFFER, sizeof(Vertex) * FFTConstants::FFT_LENGTH, nullptr,
    // juce::gl::GL_DYNAMIC_DRAW);
    // _context.extensions.glBufferSubData(juce::gl::GL_ARRAY_BUFFER, 0, sizeof(Vertex) * FFTConstants::FFT_LENGTH, vertices.data());
    // juce::gl::glBufferData(juce::gl::GL_ARRAY_BUFFER, sizeof(Vertex) * FFTConstants::FFT_LENGTH, nullptr, juce::gl::GL_DYNAMIC_DRAW);
    juce::gl::glBufferSubData(juce::gl::GL_ARRAY_BUFFER, 0, sizeof(Vertex) * FFTConstants::FFT_LENGTH, vertices.data());
}

void ThreeImager::timerCallback() {
    // fftの結果を取得
    fft_data = manager->getFFTResult();
    fft_freq = manager->getFFTFreqs();
    // verticesの更新
    setVertices();
    updateBuffer();
    repaint();
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
