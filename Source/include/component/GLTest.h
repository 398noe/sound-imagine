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
    void paint(juce::Graphics &g) override;
    void timerCallback() override;

    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) override;

    void updateProjectionMatrix();

  private:
    std::vector<OpenGLShader::Vertex> vertices;
    std::vector<OpenGLShader::Vertex> axis;
    std::vector<OpenGLShader::Vertex> grid;
    std::vector<OpenGLShader::Vertex> label;

    juce::Matrix3D<float> projection_matrix;
    juce::Matrix3D<float> view_matrix;
    juce::Matrix3D<float> model_matrix;
    float zoom = 2.0f;
    juce::Vector3D<float> rotation;

    GLuint vbo;

    std::unique_ptr<juce::OpenGLShaderProgram> shader;

    juce::String vertex_shader;
    juce::String fragment_shader;

    juce::OpenGLContext _context;
};
