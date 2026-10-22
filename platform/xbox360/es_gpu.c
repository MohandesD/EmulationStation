#include "es_gpu.h"
#include "es_gl.h"
#include "es_shaders.h"
#include <xsdk/gpu.h>
#include <xsdk/log.h>
#include <xsdk/present.h>
#include <xsdk/shader.h>
#include <xsdk/texture.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

enum { WIDTH=1280, HEIGHT=720, VERTICES=65536 };
typedef struct { XsdkTexture texture; uint64_t revision; } DeviceTexture;
static struct {
    int opened, presented, inFrame, failed;
    XsdkShader vertex, fragment;
    XsdkTexture white;
    volatile uint32_t *vertices;
    uint32_t front;
    size_t used;
} gpu;
_Static_assert(sizeof(ESGLVertex)==40,"Vertex shader layout must match CPU vertices");
_Static_assert(ES_FRAG_SAMPLER_0_0==0,"Texture shader binding must match the GPU slot");

static int checked(const char *operation, NTSTATUS status) {
    if(NT_SUCCESS(status)) return 1;
    xsdk_log_write("ES GPU %s failed: 0x%08X",operation,(unsigned)status);
    gpu.failed=1; return 0;
}
static int waitGpu(void) { return checked("fence",xsdk_gpu_wait(xsdk_gpu_fence(),5000)); }
static int beginFrame(void) {
    if(gpu.failed || !gpu.opened) return 0;
    if(gpu.inFrame) return 1;
    if(!waitGpu() || !checked("present begin",xsdk_present_begin(&gpu.front,5000))) return 0;
    gpu.used=0; gpu.inFrame=1;
    xsdk_gpu_set_target(WIDTH,HEIGHT);
    return 1;
}
static void destroyTexture(ESGLTexture *source) {
    DeviceTexture *d=source->device;
    if(!d) return;
    // Do not free memory still referenced by commands after a GPU failure.
    if(gpu.failed || !waitGpu()) return;
    xsdk_texture_free(&d->texture); free(d); source->device=NULL;
}
static XsdkTexture *texture(ESGLTexture *source) {
    if(!source) return &gpu.white;
    DeviceTexture *d=source->device;
    if(!d) {
        d=calloc(1,sizeof *d);
        if(!d) { gpu.failed=1; xsdk_log_write("ES GPU texture allocation failed"); return NULL; }
        source->device=d;
    }
    if(d->revision!=source->revision) {
        if(!waitGpu()) return NULL;
        if(d->texture.width!=source->width || d->texture.height!=source->height) {
            if(d->texture.texels) xsdk_texture_free(&d->texture);
            memset(&d->texture,0,sizeof d->texture);
            if(!checked("texture create",xsdk_texture_create(&d->texture,XSDK_FORMAT_R8G8B8A8,
                          source->width,source->height,1,false))) return NULL;
        }
        xsdk_texture_upload(&d->texture,0,source->rgba,source->width*4);
        xsdk_gpu_invalidate(d->texture.address,d->texture.size_bytes);
        d->revision=source->revision;
    }
    return &d->texture;
}
static uint32_t blend(GLenum value) {
    switch(value) {
    case GL_ZERO:return XSDK_BLEND_ZERO; case GL_ONE:return XSDK_BLEND_ONE;
    case GL_SRC_ALPHA:return XSDK_BLEND_SRC_ALPHA; case GL_ONE_MINUS_SRC_ALPHA:return XSDK_BLEND_ONE_MINUS_SRC_ALPHA;
    case GL_SRC_COLOR:return XSDK_BLEND_SRC_COLOR; case GL_ONE_MINUS_SRC_COLOR:return XSDK_BLEND_ONE_MINUS_SRC_COLOR;
    case GL_DST_ALPHA:return XSDK_BLEND_DST_ALPHA; case GL_ONE_MINUS_DST_ALPHA:return XSDK_BLEND_ONE_MINUS_DST_ALPHA;
    case GL_DST_COLOR:return XSDK_BLEND_DST_COLOR; default:return XSDK_BLEND_ONE_MINUS_DST_COLOR;
    }
}
static int64_t bounded(int64_t x, int64_t max) { return x<0 ? 0 : x>max ? max : x; }
static int draw(const ESGLDrawState *s, const ESGLVertex *vertices, size_t count) {
    if(count>VERTICES || !beginFrame()) return 0;
    XsdkTexture *t=texture(s->texture); if(!t) return 0;
    if(gpu.used+count>VERTICES) { if(!waitGpu()) return 0; gpu.used=0; }
    volatile uint32_t *destination=gpu.vertices+gpu.used*(sizeof *vertices/4);
    // xsdk's write-combined GPU allocation requires volatile 32-bit stores.
    const unsigned char *source=(const unsigned char *)vertices;
    for(size_t i=0;i<count*sizeof *vertices/4;++i) {
        uint32_t word; memcpy(&word,source+i*4,4); destination[i]=word;
    }
    uint32_t address=xsdk_gpu_address(destination);
    xsdk_gpu_invalidate(address,(uint32_t)(count*sizeof *vertices));
    gpu.used+=count;
    xsdk_gpu_set_shaders(&gpu.vertex,&gpu.fragment);
    xsdk_built_shader_set_literals(&es_vert); xsdk_built_shader_set_literals(&es_frag);
    xsdk_gpu_set_vertices(0,address,(uint32_t)(count*sizeof *vertices));
    XsdkSampler sampler={.min_filter=XSDK_FILTER_POINT,.mag_filter=XSDK_FILTER_POINT,
                          .address_u=XSDK_ADDRESS_CLAMP,.address_v=XSDK_ADDRESS_CLAMP};
    if(s->texture) {
        sampler.min_filter=s->texture->minFilter==GL_LINEAR ? XSDK_FILTER_LINEAR : XSDK_FILTER_POINT;
        sampler.mag_filter=s->texture->magFilter==GL_LINEAR ? XSDK_FILTER_LINEAR : XSDK_FILTER_POINT;
        sampler.address_u=s->texture->wrapS==GL_REPEAT ? XSDK_ADDRESS_WRAP : XSDK_ADDRESS_CLAMP;
        sampler.address_v=s->texture->wrapT==GL_REPEAT ? XSDK_ADDRESS_WRAP : XSDK_ADDRESS_CLAMP;
    }
    XsdkSamplerState samplerState;
    xsdk_texture_sampler_state(&samplerState,&sampler);
    xsdk_gpu_set_texture(0,t,&samplerState);
    XsdkBlend setting={.color_source=s->blendEnabled ? blend(s->sourceBlend) : XSDK_BLEND_ONE,
        .color_dest=s->blendEnabled ? blend(s->destBlend) : XSDK_BLEND_ZERO,
        .alpha_source=s->blendEnabled ? blend(s->sourceBlend) : XSDK_BLEND_ONE,
        .alpha_dest=s->blendEnabled ? blend(s->destBlend) : XSDK_BLEND_ZERO,
        .write_mask=XSDK_WRITE_ALL};
    XsdkBlendState blendState; xsdk_gpu_blend_state(&blendState,&setting); xsdk_gpu_set_blend(&blendState);
    xsdk_gpu_set_viewport(s->viewport[0],HEIGHT-((float)s->viewport[1]+s->viewport[3]),
                          s->viewport[2],s->viewport[3],0,1);
    if(s->scissorEnabled) {
        int64_t left=bounded(s->scissor[0],WIDTH), bottom=bounded(s->scissor[1],HEIGHT);
        int64_t right=bounded((int64_t)s->scissor[0]+s->scissor[2],WIDTH);
        int64_t top=bounded((int64_t)s->scissor[1]+s->scissor[3],HEIGHT);
        if(right<=left || top<=bottom) return 1;
        xsdk_gpu_set_scissor(left,HEIGHT-top,right-left,top-bottom);
    } else xsdk_gpu_set_scissor(0,0,WIDTH,HEIGHT);
    uint32_t primitive=s->primitive==GL_LINES ? XSDK_PRIMITIVE_LINE_LIST :
        s->primitive==GL_TRIANGLES ? XSDK_PRIMITIVE_TRIANGLE_LIST : XSDK_PRIMITIVE_TRIANGLE_STRIP;
    xsdk_gpu_draw(primitive,count);
    return 1;
}
static int clear(const ESGLDrawState *s, const float rgba[4]) {
    ESGLDrawState full=*s;
    full.viewport[0]=full.viewport[1]=0; full.viewport[2]=WIDTH; full.viewport[3]=HEIGHT;
    full.texture=NULL; full.blendEnabled=0; full.primitive=GL_TRIANGLES;
    const float xy[6][2]={{-1,-1},{1,-1},{-1,1},{1,-1},{1,1},{-1,1}};
    ESGLVertex vertices[6]={0};
    for(unsigned i=0;i<6;++i) {
        vertices[i].position[0]=xy[i][0]; vertices[i].position[1]=xy[i][1]; vertices[i].position[3]=1;
        memcpy(vertices[i].color,rgba,sizeof vertices[i].color);
    }
    return draw(&full,vertices,6);
}
int esGpuOpen(void) {
    memset(&gpu,0,sizeof gpu);
    if(!checked("open",xsdk_gpu_open())) return 0;
    gpu.opened=1;
    uint32_t width=0,height=0;
    if(!checked("display",xsdk_present_open(XSDK_DISPLAY_720P,2,1,&width,&height))) goto fail;
    gpu.presented=1;
    if(width!=WIDTH || height!=HEIGHT) { xsdk_log_write("ES GPU unexpected display size"); goto fail; }
    if(!checked("vertex shader",xsdk_shader_load(&gpu.vertex,es_vert.container,es_vert.container_bytes)) ||
       !checked("fragment shader",xsdk_shader_load(&gpu.fragment,es_frag.container,es_frag.container_bytes))) goto fail;
    const XsdkVertexElement layout[]={
        {.usage=XSDK_USAGE_POSITION,.format=XSDK_VERTEX_FLOAT4,.offset_bytes=0},
        {.usage=XSDK_USAGE_COLOR,.format=XSDK_VERTEX_FLOAT4,.offset_bytes=16},
        {.usage=XSDK_USAGE_TEXCOORD,.format=XSDK_VERTEX_FLOAT2,.offset_bytes=32}};
    const uint32_t stride=sizeof(ESGLVertex);
    if(!checked("vertex layout",xsdk_shader_set_vertex_layout(&gpu.vertex,layout,3,&stride))) goto fail;
    gpu.vertices=xsdk_gpu_alloc(VERTICES*sizeof(ESGLVertex));
    if(!gpu.vertices) { xsdk_log_write("ES GPU vertex allocation failed"); goto fail; }
    if(!checked("white texture",xsdk_texture_create(&gpu.white,XSDK_FORMAT_R8G8B8A8,1,1,1,false))) goto fail;
    const unsigned char white[4]={255,255,255,255};
    xsdk_texture_upload(&gpu.white,0,white,4); xsdk_gpu_invalidate(gpu.white.address,gpu.white.size_bytes);
    const ESGLDriver driver={draw,clear,destroyTexture}; esglInit(&driver);
    glViewport(0,0,WIDTH,HEIGHT);
    return 1;
fail:
    esGpuClose(); return 0;
}
int esGpuPresent(void) {
    if(!beginFrame()) return 0;
    xsdk_gpu_resolve(gpu.front,WIDTH,HEIGHT,0xFF000000u);
    xsdk_present_end(); gpu.inFrame=0;
    return !gpu.failed;
}
int esGpuCapture(const char *path) {
    if(!gpu.opened || gpu.inFrame || !gpu.front || !waitGpu()) return 0;
    const volatile uint32_t *pixels=xsdk_present_pixels(gpu.front);
    const size_t bytes=xsdk_gpu_resolve_bytes(WIDTH,HEIGHT);
    FILE *file=fopen(path,"wb"); if(!file) return 0;
    // Capture only our own 1280x720 tiled target; no XBDM/D3D surface metadata.
    int ok=fwrite((const void *)pixels,1,bytes,file)==bytes;
    if(fclose(file)!=0) ok=0;
    xsdk_log_write("ES GPU capture: %u bytes, ok=%d",(unsigned)bytes,ok);
    return ok;
}
void esGpuClose(void) {
    if(!gpu.opened) return;
    // A failed fence is not permission to free memory the GPU may still read.
    if(!waitGpu()) { xsdk_log_write("ES GPU close retains allocations after fence failure"); return; }
    gpu.failed=0;
    esglShutdown();
    if(gpu.presented) xsdk_present_close();
    if(gpu.white.texels) xsdk_texture_free(&gpu.white);
    if(gpu.vertices) xsdk_gpu_free(gpu.vertices);
    if(gpu.fragment.microcode) xsdk_shader_free(&gpu.fragment);
    if(gpu.vertex.microcode) xsdk_shader_free(&gpu.vertex);
    (void)checked("close",xsdk_gpu_close()); gpu.opened=0;
}
