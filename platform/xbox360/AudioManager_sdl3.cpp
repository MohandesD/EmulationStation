#include "AudioManager.h"
#include "Log.h"
#include <algorithm>
#include <cstring>

std::vector<std::shared_ptr<Sound>> AudioManager::sSoundVector;
SDL_AudioSpec AudioManager::sAudioFormat;
std::shared_ptr<AudioManager> AudioManager::sInstance;
SDL_AudioStream* AudioManager::sAudioStream=nullptr;

AudioManager::AudioManager() { init(); }
AudioManager::~AudioManager() { deinit(); }
std::shared_ptr<AudioManager>& AudioManager::getInstance() {
    if(!sInstance) sInstance=std::shared_ptr<AudioManager>(new AudioManager);
    return sInstance;
}
void AudioManager::mixAudio(void*, Uint8* output,int length) {
    memset(output,0,length);
    for(const auto& sound:sSoundVector) {
        if(!sound->isPlaying()) continue;
        Uint32 remaining=sound->getLength()-sound->getPosition();
        Uint32 bytes=std::min(remaining,static_cast<Uint32>(length));
        SDL_MixAudio(output,sound->getData()+sound->getPosition(),SDL_AUDIO_S16LE,bytes,1.0f);
        sound->setPosition(sound->getPosition()+bytes);
    }
}
void AudioManager::streamCallback(void*,SDL_AudioStream* stream,int additional,int) {
    // Called with SDL's recursive stream lock held. Fixed scratch space bounds
    // callback memory, and complete stereo frames keep conversions aligned.
    Uint8 buffer[4096];
    while(additional>0) {
        int bytes=std::min(additional,static_cast<int>(sizeof buffer));
        bytes=(bytes+3)&~3;
        mixAudio(nullptr,buffer,bytes);
        if(!SDL_PutAudioStreamData(stream,buffer,bytes)) return;
        additional-=bytes;
    }
}
void AudioManager::init() {
    if(sAudioStream) return;
    if(!SDL_InitSubSystem(SDL_INIT_AUDIO)) { LOG(LogError)<<"SDL audio init: "<<SDL_GetError(); return; }
    sAudioFormat.format=SDL_AUDIO_S16LE; sAudioFormat.channels=2; sAudioFormat.freq=44100;
    sAudioStream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&sAudioFormat,streamCallback,nullptr);
    if(!sAudioStream) { LOG(LogError)<<"SDL audio stream: "<<SDL_GetError(); SDL_QuitSubSystem(SDL_INIT_AUDIO); return; }
    ES_AudioSetStream(sAudioStream);
}
void AudioManager::deinit() {
    if(!sAudioStream) return;
    stop();
    SDL_DestroyAudioStream(sAudioStream); sAudioStream=nullptr;
    ES_AudioSetStream(nullptr); SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
void AudioManager::registerSound(std::shared_ptr<Sound>& sound) {
    getInstance(); ES_AudioLock(); sSoundVector.push_back(sound); ES_AudioUnlock();
}
void AudioManager::unregisterSound(std::shared_ptr<Sound>& sound) {
    ES_AudioLock();
    const auto it=std::find(sSoundVector.begin(),sSoundVector.end(),sound);
    if(it!=sSoundVector.end()) { (*it)->stop(); sSoundVector.erase(it); }
    ES_AudioUnlock();
}
void AudioManager::play() {
    getInstance(); if(sAudioStream) SDL_ResumeAudioStreamDevice(sAudioStream);
}
void AudioManager::stop() {
    if(sAudioStream) SDL_PauseAudioStreamDevice(sAudioStream);
    ES_AudioLock();
    for(const auto& sound:sSoundVector) sound->stop();
    ES_AudioUnlock();
    if(sAudioStream) SDL_ClearAudioStream(sAudioStream);
}
