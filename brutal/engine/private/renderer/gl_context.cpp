#include "brutal/renderer/gl_context.h"
#include "brutal/core/logging.h"
#include <glad/glad.h>

namespace brutal {

bool gl_init() {
    if (!gladLoadGL()) {
        LOG_ERROR("Failed to load OpenGL");
        return false;
    }
    LOG_INFO("OpenGL: %s", glGetString(GL_VERSION));
    LOG_INFO("Renderer: %s", glGetString(GL_RENDERER));
    return true;
}

}
