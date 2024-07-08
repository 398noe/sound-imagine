#pragma once

#include "lib/AudioUtilities.h"
#include "lib/Constant.h"
#include "lib/Manager.h"
#include "lib/Shader.h"

class ThreeImager : public juce::Component, public juce::OpenGLRenderer, private juce::Timer {
  public:
    ThreeImager(std::shared_ptr<Manager> data);
    ~ThreeImager();

    void newOpenGLContextCreated() override;
    void openGLContextClosing() override;
    void renderOpenGL() override;
    void resized() override;
    void paint(juce::Graphics &g) override;
    juce::Matrix3D<float> getViewMatrix();
    juce::Matrix3D<float> getProjectionMatrix();
    void timerCallback() override;

    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) override;

    void updateProjectionMatrix();

  private:
    std::shared_ptr<Manager> manager;
    std::array<float[FFTConstants::FFT_LENGTH], 4> fft_data = {0.0f};
    std::array<float, FFTConstants::FFT_LENGTH> fft_freq = {0.0f};

    std::vector<OpenGLShader::Vertex> vertices;
    std::vector<OpenGLShader::Vertex> axis;
    std::vector<OpenGLShader::Vertex> grid;
    std::vector<OpenGLShader::Vertex> label;

    juce::CriticalSection render_lock;
    juce::Matrix3D<float> projection_matrix;
    juce::Matrix3D<float> view_matrix;
    juce::Matrix3D<float> model_matrix;

    float scale = 2.0f;
    juce::Rectangle<int> bounds;
    juce::Draggable3DOrientation orientation;

    GLuint vbo;

    std::unique_ptr<juce::OpenGLShaderProgram> shader;

    juce::String vertex_shader;
    juce::String fragment_shader;

    juce::OpenGLContext _context;
};
