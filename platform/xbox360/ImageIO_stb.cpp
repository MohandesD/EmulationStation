#include "ImageIO.h"
#include "Log.h"
#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstring>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MAX_DIMENSIONS 8192
#include <stb_image.h>

std::vector<unsigned char> ImageIO::loadFromMemoryRGBA32(const unsigned char* data,size_t size,size_t& width,size_t& height) {
    width=height=0;
    if(!data || size>INT_MAX) return {};
    int w=0,h=0,channels=0;
    if(!stbi_info_from_memory(data,(int)size,&w,&h,&channels) || w<=0 || h<=0 ||
       uint64_t(w)*h*4>64u*1024u*1024u) return {};
    unsigned char* decoded=stbi_load_from_memory(data,(int)size,&w,&h,&channels,4);
    if(!decoded) { LOG(LogWarning)<<"Image decode failed: "<<stbi_failure_reason(); return {}; }
    std::vector<unsigned char> result(decoded,decoded+size_t(w)*h*4);
    stbi_image_free(decoded); width=w; height=h;
    // FreeImage's upstream contract is bottom row first; preserve texture UVs.
    flipPixelsVert(result.data(),width,height);
    return result;
}
void ImageIO::flipPixelsVert(unsigned char* pixels,const size_t& width,const size_t& height) {
    if(!pixels) return;
    for(size_t y=0;y<height/2;++y) {
        unsigned char* a=pixels+y*width*4;
        unsigned char* b=pixels+(height-y-1)*width*4;
        for(size_t x=0;x<width*4;++x) std::swap(a[x],b[x]);
    }
}
