#pragma once

#include "lib/Shader.h"
#include <JuceHeader.h>

class GLTest : public juce::Component, public juce::OpenGLRenderer, private juce::Timer {
  public:
    GLTest();
    ~GLTest();

    void newOpenGLContextCreated() override;
    void openGLContextClosing() override;
    void renderOpenGL() override;
    void resized() override;
    void timerCallback() override;

  private:
    std::vector<OpenGLShader::Vertex> vertices;
    std::vector<OpenGLShader::Vertex> axis;
    // std::vector<unsigned int> index_buffer;

    GLuint vbo;
    // GLuint vbo, ibo;

    std::unique_ptr<juce::OpenGLShaderProgram> shader;

    juce::String vertex_shader;
    juce::String fragment_shader;

    juce::OpenGLContext _context;
};
