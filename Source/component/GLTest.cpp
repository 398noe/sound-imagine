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

    shader->setUniformMat4("model", model_matrix.mat, 1, false);
    shader->setUniformMat4("view", view_matrix.mat, 1, false);
    shader->setUniformMat4("projection", projection_matrix.mat, 1, false);

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

void GLTest::mouseDown(const juce::MouseEvent &e) {}

void GLTest::mouseDrag(const juce::MouseEvent &e) {
    auto delta_x = e.getDistanceFromDragStartX();
    auto delta_y = e.getDistanceFromDragStartY();

    yaw += delta_x * 0.001f;
    pitch += delta_y * 0.001f;

    // pitch = juce::jlimit(-juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, pitch);

    float radius = 5.0f;
    camera_position.x = radius * std::cos(yaw) * std::cos(pitch);
    camera_position.y = radius * std::sin(pitch);
    camera_position.z = radius * std::sin(yaw) * std::cos(pitch);

    updateProjectionMatrix();
    repaint();
}

void GLTest::mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) {
    zoom += wheel.deltaY;
    zoom = juce::jlimit(0.1f, 10.0f, zoom);
    updateProjectionMatrix();
    repaint();
}

juce::Vector3D<float> cross(const juce::Vector3D<float> &a, const juce::Vector3D<float> &b) {
    return juce::Vector3D<float>(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

// ドット積を計算する関数
float dot(const juce::Vector3D<float> &a, const juce::Vector3D<float> &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// lookAt行列を作成する関数
juce::Matrix3D<float> GLTest::createLookAtMatrix(const juce::Vector3D<float> &eye, const juce::Vector3D<float> &center,
                                                 const juce::Vector3D<float> &up) {
    juce::Vector3D<float> f = (center - eye).normalised();
    juce::Vector3D<float> s = cross(f, up).normalised();
    juce::Vector3D<float> u = cross(s, f);

    juce::Matrix3D<float> result;
    result.mat[0] = s.x;
    result.mat[4] = s.y;
    result.mat[8] = s.z;
    result.mat[12] = -dot(s, eye);
    result.mat[1] = u.x;
    result.mat[5] = u.y;
    result.mat[9] = u.z;
    result.mat[13] = -dot(u, eye);
    result.mat[2] = -f.x;
    result.mat[6] = -f.y;
    result.mat[10] = -f.z;
    result.mat[14] = dot(f, eye);
    result.mat[3] = 0.0f;
    result.mat[7] = 0.0f;
    result.mat[11] = 0.0f;
    result.mat[15] = 1.0f;

    return result;
}

void GLTest::updateProjectionMatrix() {
    auto aspect = (float)getWidth() / (float)getHeight();
    projection_matrix = juce::Matrix3D<float>::fromFrustum(-aspect, aspect, -1.0f, 1.0f, 1.0f, 100.0f);
    view_matrix = juce::Matrix3D<float>::fromTranslation(juce::Vector3D<float>(0.0f, 0.0f, -5.0f / zoom));
    model_matrix = createLookAtMatrix(camera_position, juce::Vector3D<float>(0.0f, 0.0f, 0.0f), juce::Vector3D<float>(0.0f, 1.0f, 0.0f));
    // model_matrix = juce::Matrix3D<float>::rotation(juce::Vector3D<float>(camera_position.x, camera_position.y, camera_position.z));
}

void GLTest::resized() { updateProjectionMatrix(); }

void GLTest::timerCallback() {
    // ここでFFTのデータを更新する
    // @todo FFTの結果からverticesを更新する必要がある
    // 更新したFFTのデータをverticesに格納する
    _context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vbo);
    _context.extensions.glBufferSubData(juce::gl::GL_ARRAY_BUFFER, sizeof(OpenGLShader::Vertex) * (axis.size() + label.size()),
                                        sizeof(OpenGLShader::Vertex) * vertices.size(), vertices.data());
}

void GLTest::paint(juce::Graphics &g) {
    // zoom, rotationの値をテキストで表示する
    g.setColour(juce::Colours::white);
    g.drawText("Zoom: " + juce::String(zoom), 10, 10, 100, 20, juce::Justification::left);
    g.drawText("Rotation: " + juce::String(rotation.x) + ", " + juce::String(rotation.y), 10, 30, 200, 20, juce::Justification::left);
}
