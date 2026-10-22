#include "ImageIO.h"
#include <stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <deprecated/stb_image_resize2.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

bool esResizeImage(const std::string& path,int maxWidth,int maxHeight) {
    if(maxWidth==0 && maxHeight==0) return true;
    if(maxWidth<0 || maxHeight<0 || maxWidth>8192 || maxHeight>8192) return false;
    int width=0,height=0,channels=0;
    if(!stbi_info(path.c_str(),&width,&height,&channels) || width<=0 || height<=0 ||
       uint64_t(width)*height*4>64u*1024u*1024u) return false;
    if(!maxWidth) maxWidth=std::max(1,int(double(maxHeight)*width/height));
    if(!maxHeight) maxHeight=std::max(1,int(double(maxWidth)*height/width));
    if(maxWidth>8192 || maxHeight>8192 || uint64_t(maxWidth)*maxHeight*4>64u*1024u*1024u) return false;
    unsigned char* pixels=stbi_load(path.c_str(),&width,&height,&channels,4);
    if(!pixels) return false;
    std::vector<unsigned char> output(size_t(maxWidth)*maxHeight*4);
    bool resized=stbir_resize_uint8_linear(pixels,width,height,0,output.data(),maxWidth,maxHeight,0,STBIR_RGBA)!=nullptr;
    stbi_image_free(pixels);
    if(!resized) return false;
    std::string suffix=path.substr(path.find_last_of('.')==std::string::npos ? path.size() : path.find_last_of('.'));
    std::transform(suffix.begin(),suffix.end(),suffix.begin(),[](unsigned char c){return char(std::tolower(c));});
    // Keep the original download intact unless both encode and replacement succeed.
    const std::string temporary=path+".tmp";
    int saved=(suffix==".jpg" || suffix==".jpeg") ?
        stbi_write_jpg(temporary.c_str(),maxWidth,maxHeight,4,output.data(),90) :
        stbi_write_png(temporary.c_str(),maxWidth,maxHeight,4,output.data(),maxWidth*4);
    if(!saved) { std::remove(temporary.c_str()); return false; }
    if(std::rename(temporary.c_str(),path.c_str())) { std::remove(temporary.c_str()); return false; }
    return true;
}
