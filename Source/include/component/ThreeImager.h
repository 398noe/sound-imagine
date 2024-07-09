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
    void timerCallback() override;

    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel) override;

    struct ShaderSource {
        juce::String vertex;
        juce::String fragment;
    };

    struct Vertex {
        float position[3];
        float colour[4];
    };

    void initShaderProgram(const ShaderSource &source);
    void initVertices();
    void setVertices();
    void updateBuffer();
    void updateProjectionMatrix();

  private:
    std::shared_ptr<Manager> manager;
    std::array<float[FFTConstants::FFT_LENGTH], 4> fft_data = {0.0f};
    std::array<float, FFTConstants::FFT_LENGTH> fft_freq = {0.0f};

    juce::CriticalSection render_lock;
    juce::Matrix3D<float> projection_matrix;
    juce::Matrix3D<float> view_matrix;
    juce::Matrix3D<float> model_matrix;

    float scale = 2.0f;
    juce::Rectangle<int> bounds;
    juce::Draggable3DOrientation orientation;

    juce::String vertex_shader;
    juce::String fragment_shader;

    juce::OpenGLContext _context;
    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    std::vector<Vertex> vertices;
    GLuint vao, vbo;
};
