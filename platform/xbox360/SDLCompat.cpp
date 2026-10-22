#include "SDLCompat.h"
#include "Log.h"

static SDL_Window* textWindow=nullptr;
static SDL_AudioStream* audioStream=nullptr;
static int audioVolume=100;

int ES_JoystickCount() {
    int count=0;
    SDL_JoystickID* ids=SDL_GetJoysticks(&count);
    SDL_free(ids);
    return count;
}
SDL_Joystick* ES_OpenJoystickIndex(int index) {
    int count=0;
    SDL_JoystickID* ids=SDL_GetJoysticks(&count);
    SDL_Joystick* result=index>=0 && index<count ? SDL_OpenJoystick(ids[index]) : nullptr;
    SDL_free(ids);
    return result;
}
int ES_AddedJoystickIndex(SDL_JoystickID id) {
    int count=0,index=-1;
    SDL_JoystickID* ids=SDL_GetJoysticks(&count);
    for(int i=0;i<count;++i) if(ids[i]==id) { index=i; break; }
    SDL_free(ids);
    return index;
}
void ES_SetJoystickEvents(bool enabled) { SDL_SetJoystickEventsEnabled(enabled); }
bool ES_InitInput() { return SDL_InitSubSystem(SDL_INIT_JOYSTICK); }
bool ES_TextInputActive() { return textWindow && SDL_TextInputActive(textWindow); }
void ES_StartTextInput() {
    if(!textWindow) {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER,"dummy");
        if(SDL_InitSubSystem(SDL_INIT_VIDEO)) textWindow=SDL_CreateWindow("ES text input",1280,720,SDL_WINDOW_HIDDEN);
    }
    if(!textWindow || !SDL_StartTextInput(textWindow)) LOG(LogWarning)<<"Text input unavailable: "<<SDL_GetError();
}
void ES_StopTextInput() { if(textWindow) SDL_StopTextInput(textWindow); }
void ES_CloseTextInput() {
    if(textWindow) { SDL_DestroyWindow(textWindow); textWindow=nullptr; SDL_QuitSubSystem(SDL_INIT_VIDEO); }
}
void ES_AudioSetStream(SDL_AudioStream* stream) {
    audioStream=stream;
    if(stream) SDL_SetAudioStreamGain(stream,audioVolume/100.0f);
}
int ES_AudioVolume() { return audioVolume; }
void ES_SetAudioVolume(int value) {
    audioVolume=value<0 ? 0 : value>100 ? 100 : value;
    if(audioStream) SDL_SetAudioStreamGain(audioStream,audioVolume/100.0f);
}
void ES_AudioLock() { if(audioStream) SDL_LockAudioStream(audioStream); }
void ES_AudioUnlock() { if(audioStream) SDL_UnlockAudioStream(audioStream); }
