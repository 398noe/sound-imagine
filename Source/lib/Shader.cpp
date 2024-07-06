#include "lib/Shader.h"

std::string loadShaderCode(const std::string &file) {
    std::ifstream ifs(file);
    if (!ifs) {
        throw std::runtime_error("Failed to open file: " + file);
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

GLuint loadShader(const std::string &vertex_path, const std::string &fragment_path) {
    std::string vertexCode = loadShaderCode(vertex_path);
    std::string fragmentCode = loadShaderCode(fragment_path);

    const char *vertexShaderSource = vertexCode.c_str();
    const char *fragmentShaderSource = fragmentCode.c_str();

    GLuint vertexShader = juce::gl::glCreateShader(juce::gl::GL_VERTEX_SHADER);
    juce::gl::glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    juce::gl::glCompileShader(vertexShader);

    GLuint fragmentShader = juce::gl::glCreateShader(juce::gl::GL_FRAGMENT_SHADER);
    juce::gl::glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    juce::gl::glCompileShader(fragmentShader);

    GLuint shaderProgram = juce::gl::glCreateProgram();
    juce::gl::glAttachShader(shaderProgram, vertexShader);
    juce::gl::glAttachShader(shaderProgram, fragmentShader);
    juce::gl::glLinkProgram(shaderProgram);

    juce::gl::glDeleteShader(vertexShader);
    juce::gl::glDeleteShader(fragmentShader);

    return shaderProgram;
}