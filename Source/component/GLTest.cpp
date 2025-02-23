#include "component/GLTest.h"

GLTest::GLTest() {
    startTimerHz(60);
    setSize(400, 400);
    setOpaque(true);

    _context.attachTo(*this);
    _context.setRenderer(this);
    _context.setContinuousRepainting(true);

    // enable z buffer
    juce::gl::glEnable(juce::gl::GL_DEPTH_TEST);
}

GLTest::~GLTest() {
    stopTimer();
    _context.detach();
}

void GLTest::newOpenGLContextCreated() {
    // generate buffer for vertex
    _context.extensions.glGenBuffers(1, &vbo);

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
    vertices = {{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
                {{0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
                {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}};

    _context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vbo);
    _context.extensions.glBufferData(juce::gl::GL_ARRAY_BUFFER,
                                     sizeof(OpenGLShader::Vertex) * (axis.size() + label.size() + vertices.size()), nullptr,
                                     juce::gl::GL_DYNAMIC_DRAW);
    _context.extensions.glBufferSubData(juce::gl::GL_ARRAY_BUFFER, 0, sizeof(OpenGLShader::Vertex) * axis.size(), axis.data());
    _context.extensions.glBufferSubData(juce::gl::GL_ARRAY_BUFFER, sizeof(OpenGLShader::Vertex) * axis.size(),
                                        sizeof(OpenGLShader::Vertex) * label.size(), label.data());
    _context.extensions.glBufferSubData(juce::gl::GL_ARRAY_BUFFER, sizeof(OpenGLShader::Vertex) * (axis.size() + label.size()),
                                        sizeof(OpenGLShader::Vertex) * vertices.size(), vertices.data());

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

void GLTest::openGLContextClosing() {}

void GLTest::renderOpenGL() {
    juce::OpenGLHelpers::clear(juce::Colours::black);

    shader->use();

    shader->setUniformMat4("model", model_matrix.mat, 1, false);           // 回転行列
    shader->setUniformMat4("view", view_matrix.mat, 1, false);             // 表示行列
    shader->setUniformMat4("projection", projection_matrix.mat, 1, false); // 射影行列

    OpenGLShader::Attributes attributes(*shader);
    attributes.enable();

    _context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vbo);

    _context.extensions.glVertexAttribPointer(0, 3, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, sizeof(OpenGLShader::Vertex), nullptr);
    _context.extensions.glEnableVertexAttribArray(0);

    _context.extensions.glVertexAttribPointer(1, 4, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, sizeof(OpenGLShader::Vertex),
                                              (GLvoid *)(sizeof(float) * 3));
    _context.extensions.glEnableVertexAttribArray(1);

    juce::gl::glLineWidth(2.0f);
    juce::gl::glDrawArrays(juce::gl::GL_LINES, 0, axis.size());

    juce::gl::glPointSize(10.0f);
    juce::gl::glDrawArrays(juce::gl::GL_POINTS, axis.size(), label.size());

    juce::gl::glPointSize(5.0f);
    juce::gl::glDrawArrays(juce::gl::GL_POINTS, axis.size() + label.size(), vertices.size());

    attributes.disable();
}

void GLTest::mouseDown(const juce::MouseEvent &e) {
    juce::ScopedLock lock(render_lock);
    orientation.mouseDown(e.getPosition());
    updateProjectionMatrix();
    repaint();
}

void GLTest::mouseDrag(const juce::MouseEvent &e) {
    juce::ScopedLock lock(render_lock);
    orientation.mouseDrag(e.getPosition());
    updateProjectionMatrix();
    repaint();
}

void GLTest::mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) {
    scale += wheel.deltaY;
    scale = juce::jlimit(0.1f, 10.0f, scale);
    updateProjectionMatrix();
    repaint();
}

void GLTest::updateProjectionMatrix() {
    auto aspect = (float)getWidth() / (float)getHeight();
    projection_matrix = juce::Matrix3D<float>::fromFrustum(-aspect, aspect, -1.0f, 1.0f, 1.0f, 100.0f);
    view_matrix = juce::Matrix3D<float>::fromTranslation(juce::Vector3D<float>(0.0f, 0.0f, -5.0f / scale));
    model_matrix = orientation.getRotationMatrix();
}

void GLTest::resized() {
    const juce::ScopedLock lock(render_lock);
    bounds = getLocalBounds();
    orientation.setViewport(bounds);
    updateProjectionMatrix();
}

void GLTest::timerCallback() {
    // ここでFFTのデータを更新する
    // @todo FFTの結果からverticesを更新する必要がある
    // 更新したFFTのデータをverticesに格納する
    _context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vbo);
    _context.extensions.glBufferSubData(juce::gl::GL_ARRAY_BUFFER, sizeof(OpenGLShader::Vertex) * (axis.size() + label.size()),
                                        sizeof(OpenGLShader::Vertex) * vertices.size(), vertices.data());
}

void GLTest::paint(juce::Graphics &g) {
    // zoom, orientationの値をテキストで表示する
    g.setColour(juce::Colours::white);
    g.drawText("Zoom: " + juce::String(scale), 10, 10, 100, 20, juce::Justification::left);
    auto rotation = orientation.getRotationMatrix();
    g.drawText("Orientation: " + juce::String(rotation.mat[0]) + ", " + juce::String(rotation.mat[1]) + ", " +
                   juce::String(rotation.mat[2]),
               10, 30, 300, 20, juce::Justification::left);
}

juce::Matrix3D<float> GLTest::getProjectionMatrix() {
    const juce::ScopedLock lock(render_lock);

    auto w = 1.0f / (scale + 0.1f);
    auto h = w * bounds.toFloat().getAspectRatio(false);

    return juce::Matrix3D<float>::fromFrustum(-w, w, -h, h, 4.0f, 30.0f);
}

juce::Matrix3D<float> GLTest::getViewMatrix() {
    const juce::ScopedLock lock(render_lock);

    // 表示行列
    auto vm = juce::Matrix3D<float>::fromTranslation({0.0f, 1.0f, -10.0f});
    // 回転行列
    auto rm = orientation.getRotationMatrix();

    return vm * rm;
}
