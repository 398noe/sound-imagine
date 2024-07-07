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

    vertices = {{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
                {{0.5f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                {{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
                {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}}};

    // index_buffer = {0, 1, 2, 3};

    _context.extensions.glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vbo);
    _context.extensions.glBufferData(juce::gl::GL_ARRAY_BUFFER, sizeof(OpenGLShader::Vertex) * vertices.size(), vertices.data(),
                                     juce::gl::GL_STATIC_DRAW);

    // _context.extensions.glBindBuffer(juce::gl::GL_ELEMENT_ARRAY_BUFFER, ibo);
    // _context.extensions.glBufferData(juce::gl::GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * index_buffer.size(), index_buffer.data(),
    //                                  juce::gl::GL_STATIC_DRAW);

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

    // GL_POINTSモードで点を描画
    juce::gl::glDrawArrays(juce::gl::GL_POINTS, 0, vertices.size());

    attributes.disable();
}

void GLTest::resized() {}

void GLTest::timerCallback() {}