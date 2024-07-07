#include "lib/Shader.h"

const char* vertex_shader =
    "attribute vec3 position;"
    "uniform mat4 projectionMatrix;"
    "uniform mat4 viewMatrix;"
    "void main() {"
    "  gl_Position = projectionMatrix * viewMatrix * vec4(position, 1.0);"
    "}";

const char* fragment_shader =
    "void main() {"
    "  gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);"
    "}";
