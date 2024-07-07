#include "component/GLTest.h"

GLTest::GLTest() {
    startTimerHz(60);
    setSize(400, 400);

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
    // _context.extensions.glGenBuffers(1, &ibo);

    // set axis
    axis = {{{-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},                                                  // X軸
            {{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},  {{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}}, // Y軸
            {{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},  {{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}, // Z軸
            {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}};

    // set vertices
    vertices = {{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
                {{0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
                {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}};

    _context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vbo);
    _context.extensions.glBufferData(juce::gl::GL_ARRAY_BUFFER, sizeof(OpenGLShader::Vertex) * (vertices.size() + axis.size()), nullptr,
                                     juce::gl::GL_STATIC_DRAW);
    _context.extensions.glBufferSubData(juce::gl::GL_ARRAY_BUFFER, 0, sizeof(OpenGLShader::Vertex) * axis.size(),
                                        axis.data()); // 軸の描画に使用するバッファ, 視点変更によって変更されることはあるのだろうか。
    _context.extensions.glBufferSubData(
        juce::gl::GL_ARRAY_BUFFER, sizeof(OpenGLShader::Vertex) * axis.size(), sizeof(OpenGLShader::Vertex) * vertices.size(),
        vertices
            .data()); // このバッファは頂点データを保持するが、今後FFTの結果を保持するバッファに変更するため、可変的に変更することになる。

    vertex_shader = R"(
        #version 330 core
        in vec4 position;
        in vec4 colour;

        out vec4 frag_colour;

        void main() {
            gl_Position = position;
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
}

void GLTest::openGLContextClosing() {}

void GLTest::renderOpenGL() {
    juce::OpenGLHelpers::clear(juce::Colours::black);

    shader->use();

    OpenGLShader::Attributes attributes(*shader);
    attributes.enable();

    _context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vbo);
    // _context.extensions.glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER, ibo);

    _context.extensions.glVertexAttribPointer(0, 3, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, sizeof(OpenGLShader::Vertex), nullptr);
    _context.extensions.glEnableVertexAttribArray(0);

    _context.extensions.glVertexAttribPointer(1, 4, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, sizeof(OpenGLShader::Vertex),
                                              (GLvoid *)(sizeof(float) * 3));
    _context.extensions.glEnableVertexAttribArray(1);

    // 点のサイズを設定 (例: 5.0f)
    juce::gl::glPointSize(5.0f);

    // X, Y, Z軸を描画
    juce::gl::glDrawArrays(juce::gl::GL_LINES, 0, axis.size());

    // GL_POINTSモードで点を描画
    juce::gl::glDrawArrays(juce::gl::GL_POINTS, axis.size(), vertices.size());

    attributes.disable();
}

void GLTest::resized() {}

void GLTest::timerCallback() {}