//=============================================================================
// common.h - common header files and declarations for imagedb
//=============================================================================

#ifndef _IMAGEDB_COMMON_H
#define _IMAGEDB_COMMON_H

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib") 
#undef min
#undef max
inline void ioctl(int s, int cmd, int* argp)
{
	ioctlsocket(s, cmd, (unsigned long*)argp);
}

#define SHUT_RDWR SD_BOTH

#else

#include <sys/socket.h>
#include <sys/ioctl.h>

#endif


extern "C" {
#ifdef DARWIN
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif
}
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>

#ifdef WIN32
//inline stat(_stat((x), (y))
inline float round(float x) { return floor(x+.5); }
#endif

#ifndef GL_TEXTURE_RECTANGLE_EXT
#define GL_TEXTURE_RECTANGLE_EXT           0x84F5
#endif

#ifndef GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT  0x84F8
#endif

#include <assert.h>
#include <string.h>
#include <vector>
using std::vector;
#include "imagedb.h"

//Contains display settings, may be stored in image, frame, or global
struct DisplaySettings
{
	float scale;
	float bias;
	float gamma;
	float zoom;
	int channels;

	DisplaySettings() : scale(1), bias(0), gamma(1), zoom(1), channels(1) /*RGBA*/ {}
};

//Contains data for a single image in the stack
struct Image
{
	int width;
	int height;
	int format;
	DisplaySettings settings;
	char title[128];
	bool flip;
	float (*data)[4];

	Image() : width(0), height(0), format(0), settings(), flip(false), data(0) {
		memset(title, 0, sizeof(title));
	}

	//unsigned char* data;
};

//Contains data for a frame (not its stored image)
struct Frame
{
	int handle;
	int current_index;
	int window_width;
	int window_height;
	int display_pos_x;
	int display_pos_y;

	int click_pos_x;
	int click_pos_y;
	int drag_pos_x;
	int drag_pos_y;
	int diff_x;
	int diff_y;

	DisplaySettings settings;

	char name[128];
	float (*framebuffer)[3];

	Frame() : handle(0), current_index(0), window_width(0), window_height(0), display_pos_x(0), display_pos_y(0), click_pos_x(0), click_pos_y(0),
		drag_pos_x(0), drag_pos_y(0), diff_x(0), diff_y(0), settings(), framebuffer(0) 
	{
		memset(name, 0, sizeof(name));
	}

	int idx(int i, int j)
	{
		return i*window_width + j;
	}
};

typedef vector<Image> ImageVector;
typedef vector<Frame> FrameVector;

extern bool jump_to_front;
extern ImageVector images;
extern FrameVector frames;
extern DisplaySettings global_display_settings;

enum {CHANNELS_RGB, CHANNELS_RGBA, CHANNELS_R, CHANNELS_G, CHANNELS_B, CHANNELS_A};
enum DISPLAY_SETTINGS_MODE { DISPLAY_SETTINGS_PER_IMAGE, DISPLAY_SETTINGS_PER_FRAME, DISPLAY_SETTINGS_GLOBAL };
extern int display_settings_mode;

extern Frame* frame;

//Function declarations for main.cc
Frame* FindFrameByName(char* name);
Frame* FindFrameByWindow(int window);

int FindImageWithTitle(char* title);
void RegisterCallbacks();
int DeleteImage(int idx);
void SetFrame(Frame* theframe);
int SetWindowTitle(Frame* fr,const char*str=NULL);
void PostAllRedisplay();
Frame* CreateFrame();
DisplaySettings* GetSettings(Image* img, Frame* frame);

//Function declarations for server.cc
void ProcessServer();
void CleanupServer();
int InitServer();

//Function declarations for io.cc
void ReadAll(char* filename);
void SaveAll(char* filename);
void SaveImage(Image* image, char* filename);
int GetNextFileNumber(char* filebase, int start);
void ReadPFM(char* filename);
void SavePFM(Image* img, char* filename);
//void WriteBMP(const char *filename, int width, int height, int color, unsigned char* pixels);
#endif

