#pragma once

#ifdef ES_USE_SDL3
#define SDL_ENABLE_OLD_NAMES
#include <SDL3/SDL.h>
// Only spelling aliases come from SDL. Changed return values, event fields,
// device IDs, text input and audio require explicit adapters below.
int ES_JoystickCount();
SDL_Joystick* ES_OpenJoystickIndex(int index);
int ES_AddedJoystickIndex(SDL_JoystickID id);
void ES_SetJoystickEvents(bool enabled);
bool ES_InitInput();
bool ES_TextInputActive();
void ES_StartTextInput();
void ES_StopTextInput();
void ES_CloseTextInput();
void ES_AudioLock();
void ES_AudioUnlock();
void ES_AudioSetStream(SDL_AudioStream* stream);
int ES_AudioVolume();
void ES_SetAudioVolume(int value);
inline SDL_Keycode ES_EventKey(const SDL_Event& event) { return event.key.key; }
inline bool ES_ButtonPressed(const SDL_Event& event) { return event.jbutton.down; }
inline void ES_GUIDString(SDL_GUID guid, char* text, int size) { SDL_GUIDToString(guid,text,size); }
#else
#include <SDL.h>
inline int ES_JoystickCount() { return SDL_NumJoysticks(); }
inline SDL_Joystick* ES_OpenJoystickIndex(int index) { return SDL_JoystickOpen(index); }
inline int ES_AddedJoystickIndex(SDL_JoystickID index) { return index; }
inline void ES_SetJoystickEvents(bool enabled) { SDL_JoystickEventState(enabled ? SDL_ENABLE : SDL_DISABLE); }
inline bool ES_InitInput() { return SDL_InitSubSystem(SDL_INIT_JOYSTICK)==0; }
inline bool ES_TextInputActive() { return SDL_IsTextInputActive(); }
inline void ES_StartTextInput() { SDL_StartTextInput(); }
inline void ES_StopTextInput() { SDL_StopTextInput(); }
inline void ES_CloseTextInput() {}
inline void ES_AudioLock() { SDL_LockAudio(); }
inline void ES_AudioUnlock() { SDL_UnlockAudio(); }
inline SDL_Keycode ES_EventKey(const SDL_Event& event) { return event.key.keysym.sym; }
inline bool ES_ButtonPressed(const SDL_Event& event) { return event.jbutton.state==SDL_PRESSED; }
inline void ES_GUIDString(SDL_JoystickGUID guid,char* text,int size) { SDL_JoystickGetGUIDString(guid,text,size); }
#endif
