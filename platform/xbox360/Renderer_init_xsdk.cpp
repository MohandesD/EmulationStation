#include "Renderer.h"
#include "es_gpu.h"
#include "Log.h"
#include "SDLCompat.h"
#include "Settings.h"
#include <stdexcept>

namespace Renderer {
static unsigned frameCount;
unsigned int getScreenWidth() { return 1280; }
unsigned int getScreenHeight() { return 720; }
bool init(int, int) {
    frameCount=0;
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
    if(++frameCount == 120 && Settings::getInstance()->getInt("XboxDiagnosticFrames") > 0)
        LOG(LogInfo) << "Native diagnostic capture: " << esGpuCapture("game:/es-frame.raw");
    glClear(GL_COLOR_BUFFER_BIT);
}
void deinit() { ES_CloseTextInput(); esGpuClose(); }
}
