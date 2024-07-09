#include "component/ThreeImager.h"
#include "lib/Shader.h"
#include <mutex>

ThreeImager::ThreeImager(std::shared_ptr<Manager> data) : manager(data) {
    startTimerHz(30);
    setSize(600, 600);
    setOpaque(true);

    _context.setOpenGLVersionRequired(juce::OpenGLContext::OpenGLVersion::openGL3_2);
    _context.setRenderer(this);
    _context.attachTo(*this);
    _context.setContinuousRepainting(false);
}

ThreeImager::~ThreeImager() {
    stopTimer();
    _context.detach();
}

void ThreeImager::newOpenGLContextCreated() {
    initVertices();
    initShaderProgram();
}

void ThreeImager::initShaderProgram() {
    std::unique_ptr<juce::OpenGLShaderProgram> new_shader(new juce::OpenGLShaderProgram(_context));
    if (new_shader->addVertexShader(source.vertex) && new_shader->addFragmentShader(source.fragment) && new_shader->link()) {
        vertex_shape.reset();
        axis_shape.reset();
        shape.reset();
        attributes.reset();
        uniforms.reset();

        shader.reset(new_shader.release());
        shader->use();

        vertex_shape.reset(new OpenGLShader::VertexShape());
        vertex_shape->init(vertices, juce::gl::GL_POINTS);

        axis_shape.reset(new OpenGLShader::VertexShape());
        std::vector<OpenGLShader::Vertex> axis = {
            {{-10.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}}, {{10.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},  // X軸
            {{0.0f, -10.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}}, {{0.0f, 10.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},  // Y軸
            {{0.0f, 0.0f, -10.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}, {{0.0f, 0.0f, 10.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},  // Z軸
            {{0.0f, 0.0f, 0.0f}, {1.0f, 0.5f, 0.0f, 1.0f}},   {{7.07f, 7.07f, 0.0f}, {1.0f, 0.5f, 0.0f, 1.0f}}, // R軸
            {{0.0f, 0.0f, 0.0f}, {0.0f, 0.5f, 1.0f, 1.0f}},   {{-7.07f, 7.07f, 0.0f}, {0.0f, 0.5f, 1.0f, 1.0f}} // L軸
        };
        axis_shape->init(axis, juce::gl::GL_LINES);

        label_shape.reset(new OpenGLShader::VertexShape());
        std::vector<OpenGLShader::Vertex> label = {
            {{11.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}, // X (Mid)
            {{0.0f, 11.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}, // Y (Side)
            {{0.0f, 0.0f, 11.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}, // Z (Hz)
            {{8.0f, 8.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},  // R
            {{-8.0f, 8.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}  // L
        };
        label_shape->init(label, juce::gl::GL_POINTS);

        shape.reset(new OpenGLShader::Shape());
        shape->add(*vertex_shape);
        shape->add(*axis_shape);
        shape->add(*label_shape);

        attributes.reset(new OpenGLShader::Attributes(*shader));
        uniforms.reset(new OpenGLShader::Uniforms(*shader));
    } else {
        jassertfalse;
    }
}

void ThreeImager::updateShader() {
    vertex_shape->update(vertices);
    // shape.reset(new OpenGLShader::Shape());
    // shape->add(*vertex_shape);
}

void ThreeImager::initVertices() {
    for (int i = 0; i < FFTConstants::FFT_LENGTH; i++) {
        vertices.push_back({{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}});
    }
}

void ThreeImager::openGLContextClosing() {
    shape.reset();
    shader.reset();
    attributes.reset();
    uniforms.reset();
}

void ThreeImager::renderOpenGL() {
    const juce::ScopedLock lock(render_mutex);
    jassert(juce::OpenGLHelpers::isContextActive());

    juce::OpenGLHelpers::clear(juce::Colours::black);

    updateShader();

    // enable z buffer
    juce::gl::glEnable(juce::gl::GL_DEPTH_TEST);
    juce::gl::glDepthFunc(juce::gl::GL_LESS);
    juce::gl::glEnable(juce::gl::GL_BLEND);
    juce::gl::glBlendFunc(juce::gl::GL_SRC_ALPHA, juce::gl::GL_ONE_MINUS_SRC_ALPHA);

    shader->use();

    // 行列の設定
    if (uniforms->projection_matrix != nullptr) {
        uniforms->projection_matrix->setMatrix4(projection_matrix.mat, 1, false);
    }

    if (uniforms->view_matrix != nullptr) {
        uniforms->view_matrix->setMatrix4(view_matrix.mat, 1, false);
    }

    if (uniforms->model_matrix != nullptr) {
        uniforms->model_matrix->setMatrix4(model_matrix.mat, 1, false);
    }

    juce::gl::glPointSize(2.0f);
    shape->draw(*attributes);

    juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, 0);
}

void ThreeImager::mouseDown(const juce::MouseEvent &e) {
    juce::ScopedLock lock(matrix_mutex);
    orientation.mouseDown(e.getPosition());
    updateProjectionMatrix();
    repaint();
}

void ThreeImager::mouseDrag(const juce::MouseEvent &e) {
    juce::ScopedLock lock(matrix_mutex);
    orientation.mouseDrag(e.getPosition());
    updateProjectionMatrix();
    repaint();
}

void ThreeImager::mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) {
    juce::ScopedLock lock(matrix_mutex);
    scale += wheel.deltaY;
    scale = juce::jlimit(0.1f, 10.0f, scale);
    updateProjectionMatrix();
    repaint();
}

void ThreeImager::resized() {
    const juce::ScopedLock lock(matrix_mutex);
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

    _context.triggerRepaint();
}

void ThreeImager::setVertices() {
    // レンダリング中に情報を更新できない
    juce::ScopedLock lock(render_mutex);
    // fft_dataからverticesを更新する
    // verticesをリセットする
    vertices.clear();
    // 一つずつ詰めなおす
    for (int i = 0; i < FFTConstants::FFT_LENGTH; i++) {
        // x は side, y は mid, z は Hz, これらを0~1の範囲に収める
        float x = juce::jmap(fft_data[3][i], -1.0f, 1.0f, -5.0f, 5.0f);
        float y = juce::jmap(fft_data[2][i], -1.0f, 1.0f, -5.0f, 5.0f);
        float z = juce::jmap(fft_freq[i], 0.0f, 24000.0f, -5.0f, 5.0f);
        vertices.push_back({{x, y, z}, {1.0f, 1.0f, 1.0f, 1.0f}});
    }
}

void ThreeImager::timerCallback() {
    // fftの結果を取得
    fft_data = manager->getFFTResult();
    fft_freq = manager->getFFTFreqs();
    // verticesの更新
    setVertices();
    _context.triggerRepaint();
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
