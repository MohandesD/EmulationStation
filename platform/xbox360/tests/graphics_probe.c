// Component diagnostic, not the EmulationStation application. No ROM launching.
#include "es_gl.h"
#include "es_gpu.h"
#include <xsdk/kernel.h>
#include <xsdk/log.h>
#include <xsdk/xam.h>

static void rect(int x,int y,int w,int h,unsigned char r,unsigned char g,unsigned char b,unsigned char a) {
    GLint points[]={x,y,x,y+h,x+w,y,x+w,y,x,y+h,x+w,y+h};
    glColor4ub(r,g,b,a); glVertexPointer(2,GL_INT,0,points);
    glDrawArrays(GL_TRIANGLES,0,6);
}
int main(void) {
    xsdk_log_write("ES graphics component probe: NOT full EmulationStation; B returns to Aurora");
    if(!esGpuOpen()) return 1;
    glMatrixMode(GL_PROJECTION); glOrtho(0,1280,720,0,-1,1); glMatrixMode(GL_MODELVIEW);
    glEnableClientState(GL_VERTEX_ARRAY); glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    GLuint checker; glGenTextures(1,&checker); glBindTexture(GL_TEXTURE_2D,checker);
    const unsigned char pixels[]={255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,128};
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,2,2,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    unsigned frames=0; int failed=0;
    for(;;) {
        INPUT_STATE input={0};
        if(XamInputGetState(0,0,&input)==ERROR_SUCCESS && (input.gamepad.buttons&INPUT_GAMEPAD_B)) break;
        glClearColor(.04f,.06f,.10f,1); glClear(GL_COLOR_BUFFER_BIT);
        rect(60,60,300,160,255,0,0,255); rect(380,60,300,160,0,255,0,255); rect(700,60,300,160,0,0,255,255);
        rect(60,260,500,160,255,0,0,255); rect(310,300,500,160,0,0,255,128);
        glScissor(900,260,200,160); glEnable(GL_SCISSOR_TEST);
        rect(800,250,400,300,255,255,255,255); glDisable(GL_SCISSOR_TEST);
        glEnable(GL_TEXTURE_2D); glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        const float uv[]={0,0,0,1,1,0,1,0,0,1,1,1}; glTexCoordPointer(2,GL_FLOAT,0,uv);
        rect(60,500,200,160,255,255,255,255);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY); glDisable(GL_TEXTURE_2D);
        GLenum error=glGetError();
        if(error || !esGpuPresent()) { xsdk_log_write("FAIL: graphics frame %u error=0x%X",frames,error); failed=1; break; }
        if(++frames==120) xsdk_log_write("PASS: 120 component frames submitted; visual verification still required");
    }
    glDeleteTextures(1,&checker); esGpuClose();
    xsdk_log_write("ES graphics probe exit: %u frames, failed=%d",frames,failed);
    if(!failed) return xsdk_launch("hdd:/Apps/Aurora/Aurora.xex",NULL,0);
    return 1;
}
