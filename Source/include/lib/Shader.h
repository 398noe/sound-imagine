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
            projection_matrix.reset(createUniform(shader, "projection"));
            view_matrix.reset(createUniform(shader, "view"));
            model_matrix.reset(createUniform(shader, "model"));
        }

        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> projection_matrix, view_matrix, model_matrix;

      private:
        static juce::OpenGLShaderProgram::Uniform *createUniform(juce::OpenGLShaderProgram &shader, const char *uniformName) {
            if (juce::gl::glGetUniformLocation(shader.getProgramID(), uniformName) < 0) {
                return nullptr;
            }

            return new juce::OpenGLShaderProgram::Uniform(shader, uniformName);
        }
    };

    struct VertexBuffer {
        VertexBuffer() {};

        ~VertexBuffer() { juce::gl::glDeleteBuffers(1, &v_vbo); };

        void init(Vertex *vertices, GLenum mode) {
            this->vertices = vertices;
            this->mode = mode;
            juce::gl::glGenBuffers(1, &v_vbo);
            juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, v_vbo);
            juce::gl::glBufferData(juce::gl::GL_ARRAY_BUFFER, sizeof(Vertex), vertices, juce::gl::GL_STATIC_DRAW);            
        }

        void bind() {
            // bind buffer, 0 is position, 1 is colour
            juce::gl::glGenVertexArrays(1, &vao);
            juce::gl::glBindVertexArray(vao);

            // position attribute
            juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, v_vbo);
            juce::gl::glEnableVertexAttribArray(0);
            juce::gl::glVertexAttribPointer(0, 3, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);

            // colour attribute
            juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, c_vbo);
            juce::gl::glEnableVertexAttribArray(1);
            juce::gl::glVertexAttribPointer(1, 4, juce::gl::GL_FLOAT, juce::gl::GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)0); // rbga

            juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, 0);
            juce::gl::glBindVertexArray(0);
        };

        void draw() {
            juce::gl::glBindVertexArray(vao);
            juce::gl::glPointSize(10.0f);
            juce::gl::glDrawArrays(mode, 0, sizeof(vertices) / sizeof(Vertex));
            juce::gl::glBindVertexArray(0);

            juce::gl::glDisableVertexAttribArray(0);
            juce::gl::glDisableVertexAttribArray(1);
        }

        void update(Vertex *vertices) {
            // update buffer
            juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, v_vbo);
            juce::gl::glBufferData(juce::gl::GL_ARRAY_BUFFER, sizeof(Vertex), vertices, juce::gl::GL_STATIC_DRAW);
        }

      private:
        GLuint vao, v_vbo, c_vbo;
        Vertex *vertices;
        GLenum mode;
    };
};
