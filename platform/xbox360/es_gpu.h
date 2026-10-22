#pragma once
#ifdef __cplusplus
extern "C" {
#endif
int esGpuOpen(void);
int esGpuPresent(void);
// Optional diagnostic: writes the last resolved 1280x720 tiled ARGB target.
// Call only between present and the next draw; waits for GPU completion.
int esGpuCapture(const char *path);
void esGpuClose(void);
#ifdef __cplusplus
}
#endif
