#include "es_gl.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static ESGLDrawState captured;
static ESGLVertex vertices[12];
static size_t vertexCount;
static unsigned draws, clears, destroyed;
static float clearColor[4];
static int driverFails;
static int draw(const ESGLDrawState *s, const ESGLVertex *v, size_t n) {
    assert(n<=12); captured=*s; memcpy(vertices,v,n*sizeof *v); vertexCount=n; ++draws; return !driverFails;
}
static int clear(const ESGLDrawState *s, const float rgba[4]) {
    captured=*s; memcpy(clearColor,rgba,sizeof clearColor); ++clears; return !driverFails;
}
static void destroy(ESGLTexture *t) { assert(t); ++destroyed; }
static void near(float a,float b) { assert(fabsf(a-b)<0.00001f); }
static void ok(void) { assert(glGetError()==GL_NO_ERROR); }
static void setup(void) {
    ESGLDriver driver={draw,clear,destroy}; esglInit(&driver);
    glViewport(0,0,1280,720);
    glMatrixMode(GL_PROJECTION); glOrtho(0,1280,720,0,-1,1); glMatrixMode(GL_MODELVIEW);
    glEnableClientState(GL_VERTEX_ARRAY);
}
int main(void) {
    setup();
    // Rectangle draw format used by upstream Renderer_draw_gl.cpp.
    const GLint positions[]={0,0,0,720,1280,720,0,0,1280,720,1280,0};
    glVertexPointer(2,GL_INT,0,positions);
    glColor4ub(255,128,0,64); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES,0,6); ok();
    assert(vertexCount==6 && draws==1 && !captured.texture && captured.blendEnabled);
    near(vertices[0].position[0],-1); near(vertices[0].position[1],1);
    near(vertices[2].position[0],1); near(vertices[2].position[1],-1);
    near(vertices[0].color[1],128/255.f); near(vertices[0].color[3],64/255.f);
    // ComponentGrid's debug/menu separator lines are also part of upstream ES.
    glDrawArrays(GL_LINES,0,2); ok();
    assert(vertexCount==2 && captured.primitive==GL_LINES);

    // Interleaved, deliberately unaligned floats and a nonzero 'first' (RatingComponent).
    unsigned char packed[1+6*20]; memset(packed,0,sizeof packed);
    for(unsigned i=0;i<6;++i) {
        const float p[4]={(float)i*10,20,0.25f,0.75f}; memcpy(packed+1+i*20,p,sizeof p);
    }
    glVertexPointer(2,GL_FLOAT,20,packed+1);
    glTexCoordPointer(2,GL_FLOAT,20,packed+9); glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    float model[16]={1,0,0,0,0,1,0,0,0,0,1,0,100,50,0,1}; glLoadMatrixf(model);
    glDrawArrays(GL_TRIANGLES,3,3); ok();
    near(vertices[0].position[0],2*130/1280.f-1); near(vertices[0].position[1],1-2*70/720.f);
    near(vertices[0].uv[0],0.25f); near(vertices[0].uv[1],0.75f);
    const unsigned char colors[24]={0,0,0,0, 0,0,0,0, 0,0,0,0, 255,0,128,64, 0,255,0,255, 0,0,255,255};
    glColorPointer(4,GL_UNSIGNED_BYTE,0,colors); glEnableClientState(GL_COLOR_ARRAY);
    glScissor(11,22,33,44); glEnable(GL_SCISSOR_TEST);
    glDrawArrays(GL_TRIANGLES,3,3); ok();
    assert(captured.scissorEnabled && captured.scissor[0]==11 && captured.scissor[3]==44);
    near(vertices[0].color[2],128/255.f); near(vertices[0].color[3],64/255.f);
    glDisableClientState(GL_COLOR_ARRAY); glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    // Font atlas: 3-byte rows padded to 4, then glyph subimage with alignment 1.
    GLuint id; glGenTextures(1,&id); assert(id); glBindTexture(GL_TEXTURE_2D,id);
    const unsigned char alpha[]={1,2,3,99,4,5,6,99};
    glTexImage2D(GL_TEXTURE_2D,0,GL_ALPHA,3,2,0,GL_ALPHA,GL_UNSIGNED_BYTE,alpha);
    glEnable(GL_TEXTURE_2D); glDrawArrays(GL_TRIANGLES,0,3); ok();
    assert(captured.texture && captured.texture->width==3 && captured.texture->height==2);
    assert(captured.texture->rgba[0]==255 && captured.texture->rgba[3]==1 && captured.texture->rgba[15]==4);
    uint64_t revision=captured.texture->revision;
    const unsigned char glyph[]={127,255}; glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexSubImage2D(GL_TEXTURE_2D,0,1,0,1,2,GL_ALPHA,GL_UNSIGNED_BYTE,glyph); ok();
    assert(captured.texture->rgba[7]==127 && captured.texture->rgba[19]==255 && captured.texture->revision>revision);
    glTexSubImage2D(GL_TEXTURE_2D,0,3,0,1,1,GL_ALPHA,GL_UNSIGNED_BYTE,glyph);
    assert(glGetError()==GL_INVALID_VALUE); assert(captured.texture->rgba[19]==255);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); ok();
    assert(captured.texture->wrapS==GL_CLAMP_TO_EDGE && captured.texture->minFilter==GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); ok();
    glDrawArrays(GL_TRIANGLES,0,3); ok();
    assert(captured.texture->magFilter==GL_LINEAR && captured.texture->rgba[7]==127);

    // RGBA bytes must not be swapped by host/console endianness.
    const unsigned char rgba[]={10,20,30,40,50,60,70,80};
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,2,1,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba); ok();
    assert(memcmp(captured.texture->rgba,rgba,sizeof rgba)==0);
    glDeleteTextures(1,&id); assert(destroyed==1); glDisable(GL_TEXTURE_2D);
    glClearColor(-1,.5f,2,.25f); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); ok();
    assert(clears==1 && clearColor[0]==0 && clearColor[1]==.5f && clearColor[2]==1);
    glClear(GL_DEPTH_BUFFER_BIT); ok(); assert(clears==1);

    // Bad input and backend failure surface as errors, rather than successful no-ops.
    unsigned oldDraws=draws;
    glDrawArrays(GL_TRIANGLES,-1,3); assert(glGetError()==GL_INVALID_VALUE && draws==oldDraws);
    glDrawArrays(9999,0,3); assert(glGetError()==GL_INVALID_ENUM && draws==oldDraws);
    glEnable(9999); assert(glGetError()==GL_INVALID_ENUM);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,8192,8192,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
    assert(glGetError()==GL_INVALID_VALUE);
    driverFails=1; glDrawArrays(GL_TRIANGLES,0,3); assert(glGetError()==GL_INVALID_OPERATION);
    driverFails=0; glDrawArrays(GL_TRIANGLES,0,3); ok();
    esglShutdown(); setup(); glDrawArrays(GL_TRIANGLES,0,3); assert(glGetError()==GL_INVALID_OPERATION);
    esglShutdown();
    puts("PASS: ES draw formats, transforms, colours, atlas updates, clipping state, lifecycle and failures");
}
