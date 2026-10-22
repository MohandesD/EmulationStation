//the Makefile defines one of these:
//#define USE_OPENGL_ES
//#define USE_OPENGL_DESKTOP

#ifdef __XSDK__
	#define GLHEADER "es_gl.h"
#elif defined(USE_OPENGL_ES)
	#define GLHEADER <GLES/gl.h>
#endif

#if defined(USE_OPENGL_DESKTOP) && !defined(__XSDK__)
	//why the hell this naming inconsistency exists is well beyond me
	#ifdef WIN32
		#define sleep Sleep
	#endif

	#define GLHEADER <SDL_opengl.h>
#endif

#include <string>

std::string getHomePath();

int runShutdownCommand(); // shut down the system (returns 0 if successful)
int runRestartCommand(); // restart the system (returns 0 if successful)
int runSystemCommand(const std::string& cmd_utf8); // run a utf-8 encoded in the shell (requires wstring conversion on Windows)
