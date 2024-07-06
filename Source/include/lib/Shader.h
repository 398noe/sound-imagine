#pragma once

#include <fstream>
#include <juce_opengl/juce_opengl.h>
#include <sstream>
#include <string>

std::string loadShaderCode(const std::string &file);
GLuint loadShader(const std::string &vertex_path, const std::string &fragment_path);