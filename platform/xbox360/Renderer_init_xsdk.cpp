#include "Renderer.h"
#include "es_gpu.h"
#include "Log.h"
#include <stdexcept>

namespace Renderer {
unsigned int getScreenWidth() { return 1280; }
unsigned int getScreenHeight() { return 720; }
bool init(int, int) {
    if(!esGpuOpen()) return false;
    glMatrixMode(GL_PROJECTION);
    glOrtho(0,1280,720,0,-1,1);
    glMatrixMode(GL_MODELVIEW);
    glClearColor(1,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT);
    return glGetError()==GL_NO_ERROR;
}
void swapBuffers() {
    GLenum error=glGetError();
    if(error || !esGpuPresent()) {
        LOG(LogError)<<"Xbox renderer failed, GL subset error "<<error;
        throw std::runtime_error("Xbox GPU rendering failed; see executable log");
    }
    glClear(GL_COLOR_BUFFER_BIT);
}
void deinit() { esGpuClose(); }
}
