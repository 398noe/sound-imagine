#pragma once

#include <juce_opengl/juce_opengl.h>

struct OpenGLShader {
    struct Vertex {
        float position[3];
        float colour[4];
    };

    struct Attributes {
        explicit Attributes(juce::OpenGLShaderProgram &shader) {
            position.reset(createAttribute(shader, "position"));
            colour.reset(createAttribute(shader, "colour"));
        }

        void enable() {
            if (position.get() != nullptr) {
                juce::gl::glVertexAttribPointer(position->attributeID, 3, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, sizeof(Vertex), nullptr);
                juce::gl::glEnableVertexAttribArray(position->attributeID);
            }

            if (colour.get() != nullptr) {
                juce::gl::glVertexAttribPointer(colour->attributeID, 4, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, sizeof(Vertex),
                                                (GLvoid *)(sizeof(float) * 6));
                juce::gl::glEnableVertexAttribArray(colour->attributeID);
            }
        }

        void disable() {
            if (position.get() != nullptr) {
                juce::gl::glDisableVertexAttribArray(position->attributeID);
            }
            if (colour.get() != nullptr) {
                juce::gl::glDisableVertexAttribArray(colour->attributeID);
            }
        }

        std::unique_ptr<juce::OpenGLShaderProgram::Attribute> position, colour;

      private:
        static juce::OpenGLShaderProgram::Attribute *createAttribute(juce::OpenGLShaderProgram &shader, const char *attributeName) {
            if (juce::gl::glGetAttribLocation(shader.getProgramID(), attributeName) < 0) {
                return nullptr;
            }

            return new juce::OpenGLShaderProgram::Attribute(shader, attributeName);
        }
    };

    struct Uniforms {
        explicit Uniforms(juce::OpenGLShaderProgram &shader) {
            projection_matrix.reset(createUniform(shader, "projectionMatrix"));
            view_matrix.reset(createUniform(shader, "viewMatrix"));
        }

        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> projection_matrix, view_matrix;

      private:
        static juce::OpenGLShaderProgram::Uniform *createUniform(juce::OpenGLShaderProgram &shader, const char *uniformName) {
            if (juce::gl::glGetUniformLocation(shader.getProgramID(), uniformName) < 0) {
                return nullptr;
            }

            return new juce::OpenGLShaderProgram::Uniform(shader, uniformName);
        }
    };
};
