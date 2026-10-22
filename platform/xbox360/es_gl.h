// EmulationStation's fixed-function GL subset. Not a general OpenGL implementation.
#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef unsigned int GLbitfield;
typedef unsigned char GLubyte;
typedef int GLint;
typedef int GLsizei;
typedef short GLshort;
typedef float GLfloat;
typedef double GLdouble;
enum {
    GL_NO_ERROR = 0, GL_INVALID_ENUM = 0x500, GL_INVALID_VALUE = 0x501,
    GL_INVALID_OPERATION = 0x502, GL_OUT_OF_MEMORY = 0x505,
    GL_ZERO = 0, GL_ONE = 1, GL_TRIANGLES = 4, GL_TRIANGLE_STRIP = 5,
    GL_SRC_ALPHA = 0x302, GL_ONE_MINUS_SRC_ALPHA = 0x303,
    GL_SRC_COLOR = 0x300, GL_ONE_MINUS_SRC_COLOR = 0x301,
    GL_DST_ALPHA = 0x304, GL_ONE_MINUS_DST_ALPHA = 0x305,
    GL_DST_COLOR = 0x306, GL_ONE_MINUS_DST_COLOR = 0x307,
    GL_SHORT = 0x1402, GL_INT = 0x1404, GL_FLOAT = 0x1406,
    GL_UNSIGNED_BYTE = 0x1401, GL_TEXTURE_2D = 0xDE1,
    GL_BLEND = 0xBE2, GL_SCISSOR_TEST = 0xC11,
    GL_VERTEX_ARRAY = 0x8074, GL_COLOR_ARRAY = 0x8076, GL_TEXTURE_COORD_ARRAY = 0x8078,
    GL_MODELVIEW = 0x1700, GL_PROJECTION = 0x1701,
    GL_RGBA = 0x1908, GL_ALPHA = 0x1906,
    GL_TEXTURE_MIN_FILTER = 0x2801, GL_TEXTURE_MAG_FILTER = 0x2800,
    GL_TEXTURE_WRAP_S = 0x2802, GL_TEXTURE_WRAP_T = 0x2803,
    GL_NEAREST = 0x2600, GL_LINEAR = 0x2601, GL_REPEAT = 0x2901, GL_CLAMP_TO_EDGE = 0x812F,
    GL_PACK_ALIGNMENT = 0xD05, GL_UNPACK_ALIGNMENT = 0xCF5,
    GL_COLOR_BUFFER_BIT = 0x4000, GL_DEPTH_BUFFER_BIT = 0x100,
    GL_VENDOR = 0x1F00, GL_RENDERER = 0x1F01, GL_VERSION = 0x1F02, GL_EXTENSIONS = 0x1F03
};

// Native-endian floats, explicit RGBA channel order; no packed host-endian colours.
typedef struct { float position[4], color[4], uv[2]; } ESGLVertex;
typedef struct {
    unsigned width, height;
    unsigned char *rgba;
    uint64_t revision;
    GLenum minFilter, magFilter, wrapS, wrapT;
    void *device;
} ESGLTexture;
typedef struct {
    GLint viewport[4], scissor[4]; // OpenGL bottom-left origin.
    int scissorEnabled, blendEnabled;
    GLenum sourceBlend, destBlend, primitive;
    ESGLTexture *texture; // NULL means solid colour.
} ESGLDrawState;
typedef struct {
    int (*draw)(const ESGLDrawState *, const ESGLVertex *, size_t);
    int (*clear)(const ESGLDrawState *, const float rgba[4]);
    void (*destroyTexture)(ESGLTexture *);
} ESGLDriver;

void esglInit(const ESGLDriver *driver);
void esglShutdown(void);
GLenum glGetError(void);
const GLubyte *glGetString(GLenum name);
void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glEnableClientState(GLenum cap);
void glDisableClientState(GLenum cap);
void glBlendFunc(GLenum source, GLenum dest);
void glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a);
void glVertexPointer(GLint size, GLenum type, GLsizei stride, const void *pointer);
void glColorPointer(GLint size, GLenum type, GLsizei stride, const void *pointer);
void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *pointer);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
void glMatrixMode(GLenum mode);
void glLoadMatrixf(const GLfloat *matrix);
void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearZ, GLdouble farZ);
void glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glClear(GLbitfield mask);
void glGenTextures(GLsizei count, GLuint *textures);
void glDeleteTextures(GLsizei count, const GLuint *textures);
void glBindTexture(GLenum target, GLuint texture);
void glPixelStorei(GLenum pname, GLint value);
void glTexParameteri(GLenum target, GLenum pname, GLint value);
void glTexParameterf(GLenum target, GLenum pname, GLfloat value);
void glTexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
                  GLint border, GLenum format, GLenum type, const void *pixels);
void glTexSubImage2D(GLenum target, GLint level, GLint x, GLint y, GLsizei width, GLsizei height,
                     GLenum format, GLenum type, const void *pixels);
#ifdef __cplusplus
}
#endif
