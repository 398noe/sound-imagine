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
                                                (GLvoid *)(sizeof(float) * 3));
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

    struct VertexShape {
        explicit VertexShape() {
            juce::gl::glGenVertexArrays(1, &vao);
            juce::gl::glGenBuffers(1, &vbo);
        };
        ~VertexShape() {
            juce::gl::glDeleteVertexArrays(1, &vao);
            juce::gl::glDeleteBuffers(1, &vbo);
        };

        void init(std::vector<Vertex> v, GLenum m) {
            this->vertices = v;
            this->mode = m;

            juce::gl::glBindVertexArray(vao);
            juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vbo);
            juce::gl::glBufferData(juce::gl::GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), juce::gl::GL_DYNAMIC_DRAW);
        }

        void update(std::vector<Vertex> v) {
            this->vertices = v;

            juce::gl::glBindVertexArray(vao);
            juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vbo);
            juce::gl::glBufferData(juce::gl::GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), juce::gl::GL_DYNAMIC_DRAW);
        }

        void bind() {
            juce::gl::glBindVertexArray(vao);
            juce::gl::glBindBuffer(juce::gl::GL_ARRAY_BUFFER, vbo);
        }

        GLenum getMode() { return mode; }

        std::vector<Vertex> getVertices() { return vertices; }

      private:
        std::vector<Vertex> vertices;
        GLenum mode;
        GLuint vao, vbo;
    };

    struct Shape {
        explicit Shape() {};

        void add(VertexShape &shape) { shapes.add(new VertexShape(shape)); }

        void draw(Attributes &attributes) {
            for (auto *sp : shapes) {
                sp->bind();
                attributes.enable();
                juce::gl::glDrawArrays(sp->getMode(), 0, sp->getVertices().size());
                attributes.disable();
            }
        }

        juce::OwnedArray<VertexShape> shapes;
    };
};
