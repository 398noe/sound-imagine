#pragma once

#include "lib/AudioUtilities.h"
#include "lib/Constant.h"
#include "lib/Manager.h"
#include "lib/Shader.h"

class ThreeImager : public juce::Component, private juce::Timer, public juce::OpenGLRenderer {
  public:
    ThreeImager(std::shared_ptr<Manager> data);
    ~ThreeImager() override;

    void paint(juce::Graphics &) override;
    void resized() override;

    void timerCallback() override;

    // OpenGL
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    juce::Matrix3D<float> getProjectionMatrix();
    juce::Matrix3D<float> getViewMatrix();

    void getDataForPaint();

    // mouse / wheel events
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseWheelMove(const juce::MouseEvent &e, const juce::MouseWheelDetails &d) override;

  private:
    juce::CriticalSection render_mutex;
    bool is_next_block_drawable = true;
    std::shared_ptr<Manager> manager;
    std::array<float[FFTConstants::FFT_LENGTH], 4> fft_data = {0.0f};
    std::array<float, FFTConstants::FFT_LENGTH> fft_freq = {0.0f};

    juce::OpenGLContext _context;
    std::unique_ptr<juce::OpenGLShaderProgram> shader;

    struct Vertex {
        float position[3]; // x, y, z
        float colour[4];   // r, g, b, a
    };

    std::vector<Vertex> point_cloud;
    std::vector<unsigned int> index_buffer;

    GLuint vbo; // vertex buffer object
    GLuint ibo; // index buffer object

    // opengl shader location
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> projection_matrix_uniform;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> view_matrix_uniform;

    // opengl shader;
    juce::String vertex_shader;
    juce::String fragment_shader;

    // opengl view rotation
    juce::Draggable3DOrientation orientation;
    float rotation = 0.0f;
    float scale = 0.5f;

    juce::Rectangle<int> bounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThreeImager)
};
