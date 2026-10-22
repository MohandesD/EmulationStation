#include "es_gl.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TEXTURES 1024
#define MAX_TEXTURE_BYTES (64u * 1024u * 1024u)
#define MAX_VERTICES 65536u
typedef struct { int enabled, size, stride; GLenum type; const unsigned char *data; } Array;
static struct {
    ESGLDriver driver;
    ESGLDrawState draw;
    ESGLTexture *textures[MAX_TEXTURES];
    GLuint bound;
    int textured, unpackAlignment;
    Array position, color, uv;
    float currentColor[4], clearColor[4], model[16], projection[16];
    GLenum matrixMode, error;
    ESGLVertex *vertices;
    size_t capacity;
} state;

static void error(GLenum value) { if (!state.error) state.error = value; }
static void identity(float *m) { memset(m, 0, 16 * sizeof(float)); m[0]=m[5]=m[10]=m[15]=1; }
static float clamp(float x) { return x < 0 ? 0 : x > 1 ? 1 : x; }
static void release(ESGLTexture *t) {
    if (!t) return;
    if (state.driver.destroyTexture) state.driver.destroyTexture(t);
    free(t->rgba); free(t);
}
void esglShutdown(void) {
    for (unsigned i=0; i<MAX_TEXTURES; ++i) release(state.textures[i]);
    free(state.vertices); memset(&state, 0, sizeof state);
}
void esglInit(const ESGLDriver *driver) {
    esglShutdown();
    if (driver) state.driver = *driver;
    identity(state.model); identity(state.projection);
    state.matrixMode = GL_MODELVIEW; state.unpackAlignment=4;
    for (unsigned i=0; i<4; ++i) state.currentColor[i]=1;
    state.draw.sourceBlend=GL_ONE; state.draw.destBlend=GL_ZERO;
}
GLenum glGetError(void) { GLenum result=state.error; state.error=0; return result; }
const GLubyte *glGetString(GLenum name) {
    switch (name) {
    case GL_VENDOR: return (const GLubyte *)"EmulationStation Xbox 360 port";
    case GL_RENDERER: return (const GLubyte *)"Xenos / xsdk fixed-function ES subset";
    case GL_VERSION: return (const GLubyte *)"ESGL subset 0.1 (not full OpenGL)";
    case GL_EXTENSIONS: return (const GLubyte *)"GL_ARB_texture_non_power_of_two";
    default: error(GL_INVALID_ENUM); return NULL;
    }
}
static void enable(GLenum cap, int value) {
    switch(cap) {
    case GL_TEXTURE_2D: state.textured=value; break;
    case GL_BLEND: state.draw.blendEnabled=value; break;
    case GL_SCISSOR_TEST: state.draw.scissorEnabled=value; break;
    default: error(GL_INVALID_ENUM);
    }
}
void glEnable(GLenum cap) { enable(cap,1); }
void glDisable(GLenum cap) { enable(cap,0); }
static Array *array(GLenum cap) {
    switch(cap) {
    case GL_VERTEX_ARRAY: return &state.position;
    case GL_COLOR_ARRAY: return &state.color;
    case GL_TEXTURE_COORD_ARRAY: return &state.uv;
    default: error(GL_INVALID_ENUM); return NULL;
    }
}
void glEnableClientState(GLenum cap) { Array *a=array(cap); if(a) a->enabled=1; }
void glDisableClientState(GLenum cap) { Array *a=array(cap); if(a) a->enabled=0; }
static int blend(GLenum value) {
    return value == GL_ZERO || value == GL_ONE || (value >= GL_SRC_COLOR && value <= GL_ONE_MINUS_DST_COLOR);
}
void glBlendFunc(GLenum source, GLenum dest) {
    if (!blend(source) || !blend(dest)) { error(GL_INVALID_ENUM); return; }
    state.draw.sourceBlend=source; state.draw.destBlend=dest;
}
void glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a) {
    state.currentColor[0]=r/255.f; state.currentColor[1]=g/255.f;
    state.currentColor[2]=b/255.f; state.currentColor[3]=a/255.f;
}
static unsigned typeSize(GLenum type) {
    switch(type) { case GL_FLOAT: case GL_INT: return 4; case GL_SHORT: return 2; case GL_UNSIGNED_BYTE: return 1; default: return 0; }
}
static void pointer(Array *a, GLint size, GLenum type, GLsizei stride, const void *p) {
    unsigned unit=typeSize(type);
    if (!unit) { error(GL_INVALID_ENUM); return; }
    if (size<2 || size>4 || stride<0) { error(GL_INVALID_VALUE); return; }
    a->size=size; a->type=type; a->stride=stride ? stride : size*(int)unit; a->data=p;
}
void glVertexPointer(GLint size, GLenum type, GLsizei stride, const void *p) { pointer(&state.position,size,type,stride,p); }
void glColorPointer(GLint size, GLenum type, GLsizei stride, const void *p) {
    if (size!=4 || (type!=GL_UNSIGNED_BYTE && type!=GL_FLOAT)) { error(GL_INVALID_VALUE); return; }
    pointer(&state.color,size,type,stride,p);
}
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *p) {
    if (size!=2) { error(GL_INVALID_VALUE); return; }
    pointer(&state.uv,size,type,stride,p);
}
static float scalar(const unsigned char *p, GLenum type) {
    // memcpy also avoids unaligned floating point loads on Xenon.
    float f; int32_t i; int16_t s;
    switch(type) {
    case GL_FLOAT: memcpy(&f,p,4); return f;
    case GL_INT: memcpy(&i,p,4); return (float)i;
    case GL_SHORT: memcpy(&s,p,2); return (float)s;
    default: return *p;
    }
}
static void readArray(const Array *a, size_t index, float *out, int normalized) {
    const unsigned char *p=a->data+index*(size_t)a->stride;
    for (int i=0; i<a->size; ++i) out[i]=scalar(p+i*typeSize(a->type),a->type) / (normalized && a->type==GL_UNSIGNED_BYTE ? 255.f : 1.f);
}
static void transform(const float *m, const float *p, float *out) {
    for (unsigned row=0; row<4; ++row) out[row]=m[row]*p[0]+m[4+row]*p[1]+m[8+row]*p[2]+m[12+row]*p[3];
}
void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    if (mode!=GL_TRIANGLES && mode!=GL_TRIANGLE_STRIP && mode!=GL_LINES) { error(GL_INVALID_ENUM); return; }
    if (first<0 || count<0 || (unsigned)count>MAX_VERTICES || (size_t)first+(size_t)count>INT_MAX) { error(GL_INVALID_VALUE); return; }
    if (!count) return;
    if (!state.driver.draw || !state.position.enabled || !state.position.data ||
        (state.color.enabled && !state.color.data) || (state.uv.enabled && !state.uv.data)) { error(GL_INVALID_OPERATION); return; }
    // Prevent index*stride overflow on the console's 32-bit address space.
    const Array *arrays[]={&state.position,&state.color,&state.uv};
    for(unsigned a=0;a<3;++a) if(arrays[a]->enabled &&
        (size_t)first+(size_t)count > SIZE_MAX/(size_t)arrays[a]->stride) { error(GL_INVALID_VALUE); return; }
    if ((size_t)count>state.capacity) {
        ESGLVertex *next=realloc(state.vertices,(size_t)count*sizeof *next);
        if (!next) { error(GL_OUT_OF_MEMORY); return; }
        state.vertices=next; state.capacity=count;
    }
    state.draw.primitive=mode;
    state.draw.texture=state.textured ? state.textures[state.bound] : NULL;
    if (state.textured && (!state.draw.texture || !state.draw.texture->rgba)) { error(GL_INVALID_OPERATION); return; }
    for (int i=0;i<count;++i) {
        ESGLVertex *v=&state.vertices[i]; float p[4]={0,0,0,1}, world[4];
        readArray(&state.position,(size_t)first+i,p,0);
        transform(state.model,p,world); transform(state.projection,world,v->position);
        memcpy(v->color,state.currentColor,sizeof v->color);
        if(state.color.enabled) readArray(&state.color,(size_t)first+i,v->color,1);
        v->uv[0]=v->uv[1]=0;
        if(state.uv.enabled) readArray(&state.uv,(size_t)first+i,v->uv,0);
    }
    if (!state.driver.draw(&state.draw,state.vertices,count)) error(GL_INVALID_OPERATION);
}
static void rect(GLint *r, GLint x, GLint y, GLsizei w, GLsizei h) {
    if(w<0 || h<0) { error(GL_INVALID_VALUE); return; }
    r[0]=x; r[1]=y; r[2]=w; r[3]=h;
}
void glViewport(GLint x, GLint y, GLsizei w, GLsizei h) { rect(state.draw.viewport,x,y,w,h); }
void glScissor(GLint x, GLint y, GLsizei w, GLsizei h) { rect(state.draw.scissor,x,y,w,h); }
void glMatrixMode(GLenum mode) {
    if(mode!=GL_MODELVIEW && mode!=GL_PROJECTION) { error(GL_INVALID_ENUM); return; }
    state.matrixMode=mode;
}
static float *matrix(void) { return state.matrixMode==GL_MODELVIEW ? state.model : state.projection; }
void glLoadMatrixf(const GLfloat *m) { if(m) memcpy(matrix(),m,16*sizeof(float)); else error(GL_INVALID_VALUE); }
void glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t, GLdouble n, GLdouble f) {
    if(l==r || b==t || n==f) { error(GL_INVALID_VALUE); return; }
    float ortho[16], old[16]; identity(ortho); memcpy(old,matrix(),sizeof old);
    ortho[0]=2/(r-l); ortho[5]=2/(t-b); ortho[10]=-2/(f-n);
    ortho[12]=-(r+l)/(r-l); ortho[13]=-(t+b)/(t-b); ortho[14]=-(f+n)/(f-n);
    for(unsigned col=0;col<4;++col) transform(old,ortho+col*4,matrix()+col*4);
}
void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    state.clearColor[0]=clamp(r); state.clearColor[1]=clamp(g);
    state.clearColor[2]=clamp(b); state.clearColor[3]=clamp(a);
}
void glClear(GLbitfield mask) {
    if(mask & ~(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT)) { error(GL_INVALID_VALUE); return; }
    // ES has no depth testing; only its colour buffer needs a clear.
    if((mask&GL_COLOR_BUFFER_BIT) && (!state.driver.clear || !state.driver.clear(&state.draw,state.clearColor))) error(GL_INVALID_OPERATION);
}
static ESGLTexture *makeTexture(void) {
    ESGLTexture *t=calloc(1,sizeof *t);
    if(t) { t->minFilter=t->magFilter=GL_NEAREST; t->wrapS=t->wrapT=GL_REPEAT; }
    else error(GL_OUT_OF_MEMORY);
    return t;
}
void glGenTextures(GLsizei count, GLuint *textures) {
    if(count<0 || (count && !textures)) { error(GL_INVALID_VALUE); return; }
    for(int i=0;i<count;++i) {
        textures[i]=0;
        for(unsigned id=1;id<MAX_TEXTURES;++id) if(!state.textures[id]) {
            state.textures[id]=makeTexture(); if(state.textures[id]) textures[i]=id; break;
        }
        if(!textures[i]) error(GL_OUT_OF_MEMORY);
    }
}
void glDeleteTextures(GLsizei count, const GLuint *textures) {
    if(count<0 || (count && !textures)) { error(GL_INVALID_VALUE); return; }
    for(int i=0;i<count;++i) {
        GLuint id=textures[i]; if(!id || id>=MAX_TEXTURES) continue;
        if(state.bound==id) state.bound=0;
        release(state.textures[id]); state.textures[id]=NULL;
    }
}
void glBindTexture(GLenum target, GLuint texture) {
    if(target!=GL_TEXTURE_2D) { error(GL_INVALID_ENUM); return; }
    if(texture>=MAX_TEXTURES) { error(GL_INVALID_VALUE); return; }
    if(!state.textures[texture]) state.textures[texture]=makeTexture();
    if(state.textures[texture]) state.bound=texture;
}
void glPixelStorei(GLenum pname, GLint value) {
    if(pname!=GL_PACK_ALIGNMENT && pname!=GL_UNPACK_ALIGNMENT) { error(GL_INVALID_ENUM); return; }
    if(value!=1 && value!=2 && value!=4 && value!=8) { error(GL_INVALID_VALUE); return; }
    if(pname==GL_UNPACK_ALIGNMENT) state.unpackAlignment=value;
}
static ESGLTexture *bound(GLenum target) {
    if(target!=GL_TEXTURE_2D) { error(GL_INVALID_ENUM); return NULL; }
    if(!state.textures[state.bound]) state.textures[state.bound]=makeTexture();
    return state.textures[state.bound];
}
void glTexParameteri(GLenum target, GLenum pname, GLint value) {
    ESGLTexture *t=bound(target); if(!t) return;
    switch(pname) {
    case GL_TEXTURE_MIN_FILTER: case GL_TEXTURE_MAG_FILTER:
        if(value!=GL_LINEAR && value!=GL_NEAREST) { error(GL_INVALID_ENUM); return; }
        if(pname==GL_TEXTURE_MIN_FILTER) t->minFilter=value; else t->magFilter=value; break;
    case GL_TEXTURE_WRAP_S: case GL_TEXTURE_WRAP_T:
        if(value!=GL_REPEAT && value!=GL_CLAMP_TO_EDGE) { error(GL_INVALID_ENUM); return; }
        if(pname==GL_TEXTURE_WRAP_S) t->wrapS=value; else t->wrapT=value; break;
    default: error(GL_INVALID_ENUM);
    }
}
void glTexParameterf(GLenum target, GLenum pname, GLfloat value) {
    if(!isfinite(value) || value<0 || value>65535) { error(GL_INVALID_ENUM); return; }
    glTexParameteri(target,pname,(GLint)value);
}
static int imageArgs(GLint level, GLsizei w, GLsizei h, GLenum format, GLenum type) {
    if(level!=0 || w<0 || h<0 || w>8192 || h>8192 || (uint64_t)w*h*4>MAX_TEXTURE_BYTES) { error(GL_INVALID_VALUE); return 0; }
    if((format!=GL_ALPHA && format!=GL_RGBA) || type!=GL_UNSIGNED_BYTE) { error(GL_INVALID_ENUM); return 0; }
    return 1;
}
static void copyPixels(ESGLTexture *t, int x, int y, int w, int h, GLenum format, const void *data) {
    size_t components=format==GL_ALPHA ? 1 : 4;
    size_t row=((size_t)w*components+state.unpackAlignment-1)&~((size_t)state.unpackAlignment-1);
    const unsigned char *source=data;
    for(int j=0;j<h;++j) for(int i=0;i<w;++i) {
        unsigned char *dest=t->rgba+((size_t)(y+j)*t->width+x+i)*4;
        if(format==GL_RGBA) memcpy(dest,source+j*row+(size_t)i*4,4);
        else { dest[0]=dest[1]=dest[2]=255; dest[3]=source[j*row+i]; }
    }
    ++t->revision;
}
void glTexImage2D(GLenum target, GLint level, GLint internal, GLsizei w, GLsizei h, GLint border,
                  GLenum format, GLenum type, const void *pixels) {
    if(!imageArgs(level,w,h,format,type)) return;
    if(border!=0 || (internal!=GL_RGBA && internal!=GL_ALPHA) || internal!=(GLint)format) { error(GL_INVALID_VALUE); return; }
    ESGLTexture *t=bound(target); if(!t) return;
    size_t bytes=(size_t)w*h*4;
    unsigned char *next=calloc(bytes ? bytes : 1,1);
    if(!next) { error(GL_OUT_OF_MEMORY); return; }
    free(t->rgba); t->rgba=next; t->width=w; t->height=h; ++t->revision;
    if(pixels) copyPixels(t,0,0,w,h,format,pixels);
}
void glTexSubImage2D(GLenum target, GLint level, GLint x, GLint y, GLsizei w, GLsizei h,
                     GLenum format, GLenum type, const void *pixels) {
    if(!imageArgs(level,w,h,format,type)) return;
    ESGLTexture *t=bound(target); if(!t) return;
    if(!t->rgba) { error(GL_INVALID_OPERATION); return; }
    if(x<0 || y<0 || (uint64_t)x+w>t->width || (uint64_t)y+h>t->height || (!pixels && w && h)) { error(GL_INVALID_VALUE); return; }
    if(w && h) copyPixels(t,x,y,w,h,format,pixels);
}
