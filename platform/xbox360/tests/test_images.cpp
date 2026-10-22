#include "ImageIO.h"
#include <stb_image.h>
#include <stb_image_write.h>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

bool esResizeImage(const std::string&,int,int);
std::string getHomePath() { return "."; } // Log.cpp's path provider; no console APIs in this test.
static std::vector<unsigned char> read(const std::string& name) {
    std::ifstream input(name,std::ios::binary);
    return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}
int main() {
    const unsigned char pixels[]={255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,128};
    const char* file="image-fixture.png";
    assert(stbi_write_png(file,2,2,4,pixels,8));
    auto source=read(file); size_t w=7,h=8;
    auto image=ImageIO::loadFromMemoryRGBA32(source.data(),source.size(),w,h);
    assert(w==2 && h==2 && image.size()==16);
    // Bottom-up texture contract: blue/white first; original RGBA bytes retained.
    assert(image[0]==0 && image[2]==255 && image[7]==128 && image[8]==255 && image[9]==0);
    ImageIO::flipPixelsVert(image.data(),w,h);
    assert(std::equal(image.begin(),image.end(),pixels));
    assert(esResizeImage(file,4,0));
    int width,height,channels; assert(stbi_info(file,&width,&height,&channels));
    assert(width==4 && height==4);
    auto resized=read(file);
    assert(!esResizeImage(file,9000,0)); assert(read(file)==resized);
    const unsigned char bad[]={1,2,3};
    assert(ImageIO::loadFromMemoryRGBA32(bad,sizeof bad,w,h).empty()); assert(w==0 && h==0);
    assert(ImageIO::loadFromMemoryRGBA32(nullptr,0,w,h).empty());
    std::remove(file);
}
