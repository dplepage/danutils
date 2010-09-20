#include "common.h"
#include <math.h>
#include <algorithm>
#include <signal.h>
#include <stdlib.h>
using std::min;

//TODO:
//High priority:
//  Composite operators
//  Per-frame zoom
//  Put more things from keycommands into menu
//
//Moderate priority:
//  Correct support for reading luminance + alpha in server
//  Directly referencing OpenGL textures/buffers as source data in client 
//
//Minor priority:
//  Try to fix the menu mouse click bug
//  Command line usage


// Constants ==============================================================

const int SCREEN_RES_X = 200;
const int SCREEN_RES_Y = 200;
const int SIDE=30;

const char* format_names[] = {0, "Lum", "LumA", "RGB", "RGBA" };
const char* type_names[] = {"Int8", "Int16", "Int32", "Float", "Double" };

GLenum format_table[] = { 0, GL_LUMINANCE, GL_LUMINANCE_ALPHA,
		GL_RGB, GL_RGBA };
GLenum internal_format_table[] = { 0, GL_LUMINANCE8,
		GL_LUMINANCE8_ALPHA8, GL_RGB8, GL_RGBA8 };
GLenum type_table[] = { GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT,
		GL_UNSIGNED_INT, GL_FLOAT, GL_DOUBLE };

enum {SETTINGS_GLOBAL, SETTINGS_PER_FRAME, SETTINGS_PER_IMAGE};

enum {MENU_CHANNEL_RGBA, MENU_CHANNEL_RGB, MENU_CHANNEL_R, MENU_CHANNEL_G, MENU_CHANNEL_B, MENU_CHANNEL_A,
	MENU_SAVE_TGA, MENU_SAVE_BMP, MENU_SAVE_PFM, MENU_SAVE_STACK, MENU_SETTINGS_GLOBAL, MENU_SETTINGS_PER_FRAME, MENU_SETTINGS_PER_IMAGE,
	MENU_DISPLAY_GRIDLINES, MENU_DISPLAY_BACKGROUND};

// Global variables =======================================================

int menuid=0;
bool jump_to_front = true;
ImageVector images;
FrameVector frames;
Frame* frame = NULL;

DisplaySettings global_display_settings;
int display_settings_mode=SETTINGS_PER_IMAGE;

Frame* savedframe=NULL;

bool button_left_down = false;
bool button_middle_down = false;
bool button_right_down = false;


double zoom=1.0;
bool background=true;
bool gridlines=false;

// End Global variables ===================================================

DisplaySettings* GetSettings(Image* img, Frame* frame)
{
	if( display_settings_mode == SETTINGS_PER_IMAGE && img )
		return &img->settings;
	else if ( display_settings_mode == SETTINGS_PER_FRAME && frame )
		return &frame->settings;
	else 
		return &global_display_settings;
}

Image* GetImage(Frame* fr)
{
	if( images.empty() ) return NULL;
	if( fr->current_index == -1 ) return NULL;
	if( fr->current_index >= (int)images.size() ) return NULL;
	return &images[fr->current_index];
}

void PostAllRedisplay()
{
	int oldwindow = glutGetWindow();

	for(unsigned i=0; i<frames.size(); i++)
	{
		glutSetWindow( frames[i].handle );
		glutPostRedisplay();
	}

	glutSetWindow( oldwindow );
}

void SetCurrentFrame()
{
	if( Frame* fr=FindFrameByWindow( glutGetWindow() ) ) 
		SetFrame(fr);
	return;
}

Frame* CreateFrame()
{
	int oldwindow = frame->handle;
	Frame fr;
	frames.push_back(fr);
	frame = FindFrameByWindow(oldwindow);
	return &frames.back();
}

void DeleteFrame(int idx)
{
	int oldwindow = frame->handle;
	glutDestroyWindow( oldwindow );
	frames.erase( frames.begin() + idx );
	frame = FindFrameByWindow(oldwindow);
	if( frame == NULL )
		frame = &frames[0];
	return;
}

Frame* FindFrameByName(char* name)
{
	Frame* tempframe=NULL;
	for(int i=0; i<(int)frames.size(); i++)
		if( !strcmp(frames[i].name, name) )
			tempframe = &frames[i];
	return tempframe;
}

Frame* FindFrameByWindow(int window)
{
	Frame* tempframe=NULL;
	for(int i=0; i<(int)frames.size(); i++)
		if( frames[i].handle == window  )
			tempframe = &frames[i];
	return tempframe;
}

void SetFrame(Frame* theframe)
{
	glutSetWindow(theframe->handle);
	frame = theframe;
}

int FindImageWithTitle(char* title)
{
	for(int i=images.size()-1; i>= 0; i--)
		if( !strcmp(title, images[i].title) )
		{
			return i;
		}

	return -1;
}

void LinearRescale(Image& img, float& scale, float& bias)
{
	double minelement = img.data[0][0];
	double maxelement = img.data[0][0];
	for(int c=0; c< std::min(img.format, 3); c++)
	{
		for(int i=0; i<img.height*img.width; i++)
		{
			double element = img.data[i][c];
			if( element > maxelement ) maxelement = element;
			if( element < minelement ) minelement = element;
		}
	}
	
	bias = -minelement;
	scale = 1.0/(maxelement - minelement);
	return;
}

int SetWindowTitle(Frame* fr, const char* str)
{
	int oldwindow = glutGetWindow();
	glutSetWindow(fr->handle);

	//Image *image = &images[fr->current_index];
	Image *image = GetImage(fr);
	char buffer[1024];
	if( image && images.size() > 0 )
	{
		DisplaySettings *settings = GetSettings(image, fr);
		sprintf(buffer, "(%d/%d) \'%s\' %dx%d %s%s *%.2g +%.2g g%.2g - Image Debugger ", 
				fr->current_index+1,
				(int)images.size(), image->title, image->height, image->width,
				format_names[image->format], (str?str:""),
				settings->scale, settings->bias, settings->gamma );
		glutSetWindowTitle(buffer);
	}
	else
		glutSetWindowTitle("Image Debugger");

	glutSetWindow(oldwindow);
	return 0;
}

int DeleteImage(int idx)
{
	Image *image = &images[idx];
	delete [] image->data;
	images.erase( images.begin() + idx );

	//Now that we have deleted an element, all of the iterators are invalid
	//We need to go through the entire array, and decrement all indices above idx
	for(unsigned i=0; i<frames.size(); i++)
		if( frames[i].current_index >= idx )
		{
			if( idx > 0 )
				frames[i].current_index--;
			else
				frames[i].current_index = 0;
		}

	return 0;
}

int DeleteAll()
{
  for (unsigned i=0; i<images.size(); i++){
    delete [] images[i].data;
  }
  images.clear();
  return 0;
}

void mouse_func(int button, int state, int x, int y)
{
	SetCurrentFrame();

	if( button == GLUT_LEFT_BUTTON )
		button_left_down = (state == 0);
	else if( button == GLUT_MIDDLE_BUTTON )
		button_middle_down = (state == 0);
	else if( button == GLUT_RIGHT_BUTTON )
		button_right_down = (state == 0);

	frame->drag_pos_x = frame->click_pos_x = x;
	frame->drag_pos_y = frame->click_pos_y = y;	

	if( state == 1 )
	{
		frame->display_pos_x += frame->diff_x;
		frame->display_pos_y += frame->diff_y;
	}

	frame->diff_x = frame->drag_pos_x - frame->click_pos_x;
	frame->diff_y = frame->drag_pos_y - frame->click_pos_y;

	//Reverse warp the mouse pointer to the image, get pixel coord
	if( button_middle_down )
	{
		Image *img = GetImage(frame); //( frame->current_index >= 0 ? &images[frame->current_index] : NULL );
		if( img )
		{
			double wratio = (double)frame->window_width / img->width;
			double hratio = (double)frame->window_height / img->height;
			double minratio = ( wratio < hratio ? wratio : hratio );
			float invratio = 1.0/minratio;

			int i=y;
			int j=x;

			float px = floor(  (1/zoom)*(invratio)*(i - frame->display_pos_y + frame->diff_y) );	
			float py = floor(  (1/zoom)*(invratio)*(j - frame->display_pos_x - frame->diff_x) );	

			if( !img->flip )
				px = img->height - px - 1;

			if( px >= 0 && px < img->height && py >= 0 && py < img->width )
			{
				int idx = int(px)*img->width + int(py);
				printf("[%4d,%4d] = (%.4f, %.4f, %.4f, %.4f)\n", (int)px, (int)py, img->data[idx][0],img->data[idx][1],img->data[idx][2],img->data[idx][3]);
			}
		}
	}

	PostAllRedisplay();
}

void mouse_motion_func(int x, int y)
{
	SetCurrentFrame();

	frame->drag_pos_x = x;
	frame->drag_pos_y = y;	

	frame->diff_x = frame->drag_pos_x - frame->click_pos_x;
	frame->diff_y = frame->drag_pos_y - frame->click_pos_y;

	PostAllRedisplay();
}

void mouse_passive_func(int x, int y)
{
	Frame* frame=FindFrameByWindow( glutGetWindow() );
		
	//Reverse warp the mouse pointer to the image, get pixel coord
	Image *img = GetImage(frame); //( frame->current_index > 0 ? &images[frame->current_index] : NULL );
	if( img )
	{
		double wratio = (double)frame->window_width / img->width;
		double hratio = (double)frame->window_height / img->height;
		double minratio = ( wratio < hratio ? wratio : hratio );
		float invratio = 1.0/minratio;

		int i=y;
		int j=x;

		float px = floor(  (1/zoom)*(invratio)*(i - frame->display_pos_y + frame->diff_y) );	
		float py = floor(  (1/zoom)*(invratio)*(j - frame->display_pos_x - frame->diff_x) );	

		if( !img->flip )
			px = img->height - px - 1;

		if( px >= 0 && px < img->height && py >= 0 && py < img->width )
		{
			int idx = int(px)*img->width + int(py);
			char buffer[512];
			sprintf(buffer, " [%d,%d]=(%.3g, %.3g, %.3g, %.3g)", (int)px, (int)py, img->data[idx][0],img->data[idx][1],img->data[idx][2],img->data[idx][3]);
			SetWindowTitle( frame, buffer );
		}
	}
}

char key_array[256] = {0};
void keyboard_handler(unsigned char key, int x, int y)
{
	SetCurrentFrame();
	Image * image = GetImage(frame); //&images[frame->current_index];


	DisplaySettings* settings = GetSettings(image, frame );

	key_array[key] = 1;

	int image_count = images.size();
	if( image_count > 0 )
	{
		if( key == 'l' )
			image->flip = !image->flip;
		else if( key == '=' )
			zoom *= 0.9;
		else if( key == '-' )
			zoom *= 1.0/0.9;
		else if( key == '[' )
			settings->scale *= 0.95;
		else if( key == ']' )
			settings->scale *= 1.0/0.95;
		else if( key == ';' )
			settings->bias = settings->bias - 0.01;
		else if( key == '\'' )
			settings->bias = settings->bias + 0.01;
		else if( key == '.' )
			settings->gamma *= 1.02;
		else if( key == ',' )
			settings->gamma /= 1.02;
		else if( key == 'a' )
			LinearRescale(*image, settings->scale, settings->bias);
		else if( key == 'i' )
		{
			settings->scale = 1.0;
			settings->bias = 0.0;
			settings->gamma = 1.0;
		}
		else if( key == 'd' )
			DeleteImage(frame->current_index);

		else if (key == 'c')
			DeleteAll();

		else if( key == 'f' )
		{
			glutReshapeWindow(image->width, image->height);
			frame->display_pos_x = 0;
			frame->display_pos_y = 0;
			zoom = 1;
		}
		else if( key == 's' )
		{
			jump_to_front = !jump_to_front;
		}
		else if( key == 'w' )
		{
			int nextnum = GetNextFileNumber("image%d.tga", 0);
			char filename[1024];
			sprintf(filename, "image%d.tga", nextnum);
			printf("Saving image as image%d.tga\n", nextnum);
			SaveImage(image, filename);
		}		
		else if( key == 'z' )
		{
			int nextnum = GetNextFileNumber("stack%d.stk", 0);
			char filename[1024];
			sprintf(filename, "stack%d.stk", nextnum);
			printf("Saving stack as stack%d.stk\n", nextnum);
			SaveAll(filename);
		}
		else if( key == 'e' )
		{
			Frame* newframe = CreateFrame();
			newframe->handle = glutCreateWindow("Image Debugger");
			RegisterCallbacks();	
			newframe->current_index = frame->current_index;	
			newframe->window_width = frame->window_width;
			newframe->window_height = frame->window_height;

			frame = &frames.back();
			glutSetWindow(newframe->handle);
			PostAllRedisplay();
		}
		else if( key == 'q' )
		{
			if( frames.size() > 1 )
				DeleteFrame( frame - &frames[0] );
		}

		image = GetImage(frame); //&images[frame->current_index];
	}

	PostAllRedisplay();
}

void keyboard_up_handler(unsigned char key, int x, int y)
{
	key_array[key] = 0;
}

void special_handler(int key, int x, int y)
{
	//printf("test\n");

	SetCurrentFrame();
	Image *image = GetImage(frame); //&images[frame->current_index];
	key_array[key] = 1;
	
	int image_count = images.size();
	if( image_count == 0 )
		return;

	if( key == GLUT_KEY_RIGHT )
		frame->current_index = (frame->current_index+1) % image_count;
	else if( key == GLUT_KEY_LEFT )
		frame->current_index = frame->current_index-1 < 0 ? image_count-1 : frame->current_index-1;
	else if( key == GLUT_KEY_PAGE_DOWN )
	{
		char* current_title = image->title;
		//for(int i=image_index-1; i>= 0; i--)
		for(int i=frame->current_index-1; i>= 0; i--)
			if( !strcmp(current_title, images[i].title) )
			{
				frame->current_index = i;
				//image_index = i;
				break;
			}
	}
	else if( key == GLUT_KEY_PAGE_UP )
	{
		char* current_title = image->title;
		for(int i=frame->current_index+1; i<image_count; i++)
			//for(int i=image_index+1; i<image_count; i++)
			if( !strcmp(current_title, images[i].title) )
			{
				//image_index = i;
				frame->current_index = i;
				break;
			}
	}

	SetWindowTitle(frame);

	PostAllRedisplay();
}

void special_up_handler(int key, int x, int y)
{
	key_array[key] = 0;
}

void resize_func(int w, int h)
{
	//printf("Resize %d %d\n", w, h);
	SetCurrentFrame();
	frame->window_width = w;
	frame->window_height = h;
	//if( frame->framebuffer ) delete [] frame->framebuffer;
	//frame->framebuffer = (new float[w*h][3]);
	//window_width = w;
	//window_height = h;

	PostAllRedisplay();
}

void InitWindow()
{
	glViewport(0, 0, frame->window_width, frame->window_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, frame->window_width, frame->window_height, 0, -1, 1); //LRBTFB
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

//Draws the checkboard pattern background
void DrawBackground()
{
	glColor3f(0.5, 0.5, 0.5);
	glBegin(GL_QUADS);
	for (int i = 0; i*SIDE < frame->window_width; i++) {
		for (int j = 0; j*SIDE < frame->window_height; j++) {
			if ((i + j) & 1) {
				glVertex2i(i*SIDE, j*SIDE);
				glVertex2i(i*SIDE, (j+1)*SIDE);
				glVertex2i((i+1)*SIDE, (j+1)*SIDE);
				glVertex2i((i+1)*SIDE, j*SIDE);
			}
		}
	}
	glEnd();
}

vector<float> scratch;
void DrawImage()
{

	if( images.size() == 0 )
		return;

	Image *image = &images[frame->current_index];
	double wratio = (double)frame->window_width / image->width;
	double hratio = (double)frame->window_height / image->height;
	double minratio = ( wratio < hratio ? wratio : hratio );
	
	DisplaySettings *settings = GetSettings(image, frame);

	glPixelTransferf(GL_RED_SCALE, settings->scale);
	glPixelTransferf(GL_GREEN_SCALE, settings->scale);
	glPixelTransferf(GL_BLUE_SCALE, settings->scale);
	glPixelTransferf(GL_RED_BIAS, settings->bias);
	glPixelTransferf(GL_GREEN_BIAS, settings->bias);
	glPixelTransferf(GL_BLUE_BIAS, settings->bias);

	int channels = settings->channels;
	if( image && (channels == CHANNELS_RGBA || channels == CHANNELS_RGB) )
	{
		if( image->format == 1 || image->format == 2 )
			channels = CHANNELS_R;
		else if( image->format == 3 )
			channels = CHANNELS_RGB;
	}

	if( channels == CHANNELS_R )
	{
		glPixelTransferf(GL_GREEN_SCALE, 0);
		glPixelTransferf(GL_BLUE_SCALE, 0);
	}
	else if( channels == CHANNELS_G )
	{
		glPixelTransferf(GL_RED_SCALE, 0);
		glPixelTransferf(GL_BLUE_SCALE, 0);
	}
	else if( channels == CHANNELS_B )
	{
		glPixelTransferf(GL_RED_SCALE, 0);
		glPixelTransferf(GL_GREEN_SCALE, 0);
	}
	else if( channels == CHANNELS_A )
	{
		float mtx[16] = {
			0.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 0.0,
			1.0, 1.0, 1.0, 0.0 };
		glMatrixMode(GL_COLOR);
		glLoadMatrixf(mtx);
		glMatrixMode(GL_MODELVIEW);
	}

	//glPixelTransferf(GL_RED_SCALE, image->settings.scale);
	//glPixelTransferf(GL_GREEN_SCALE, image->settings.scale);
	//glPixelTransferf(GL_BLUE_SCALE, image->settings.scale);
	//glPixelTransferf(GL_RED_BIAS, image->settings.bias);
	//glPixelTransferf(GL_GREEN_BIAS, image->settings.bias);
	//glPixelTransferf(GL_BLUE_BIAS, image->settings.bias);

	//To modify the gamma, this gets a bit tricky.  I dont think there is functionality
	// in the fixed-function GL, so we will just do it ourselves
	if( settings->gamma != 1 )
	{
		float invgamma = 1.0f/settings->gamma;
		scratch.resize(image->width*image->height*4);
		for(int i=0; i<image->width*image->height; i++)
		{
			scratch[i*4] = pow(image->data[i][0], invgamma);
			scratch[i*4+1] = pow(image->data[i][1], invgamma);
			scratch[i*4+2] = pow(image->data[i][2], invgamma);
			scratch[i*4+3] = image->data[i][3];
		}
	}

	if( channels == CHANNELS_RGBA )
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	GLenum tex = GL_TEXTURE_RECTANGLE_EXT;
	glEnable(tex);
	glTexImage2D(tex, 0, 
		internal_format_table[image->format], 
		image->width, image->height, 0, 
		GL_RGBA, GL_FLOAT,
		( settings->gamma != 1 ) ?
			&scratch[0] :
			&image->data[0][0]
		);
	glTexParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	
	glTranslatef(frame->display_pos_x + frame->diff_x, frame->display_pos_y + frame->diff_y, 0);

	//glTranslatef(+image->width*minratio/2.0, +image->height*minratio/2.0, 0.0);
	//glTranslatef(window_width/2, window_height/2, 0.0);
	glScalef(zoom, zoom, 1.0);
	//glTranslatef(-window_width/2, -window_height/2, 0.0);
	//glTranslatef(-image->width*minratio/2.0, -image->height*minratio/2.0, 0.0);

	//Draw the quad itself
	glBegin(GL_QUADS);

	glColor3f(1,0,0);
	if( image->flip )
		glTexCoord2f(0,0); 				
	else
		glTexCoord2f(0, image->height); 		
	glVertex2f(0,0);
	
	glColor3f(0,1,0);
	if( image->flip )
		glTexCoord2f(image->width, 0); 			
	else 
		glTexCoord2f(image->width, image->height); 	
	glVertex2f(image->width*minratio,0);
	
	glColor3f(0,1,1);
	if( image->flip )
		glTexCoord2f(image->width, image->height); 	
	else
		glTexCoord2f(image->width, 0); 			
	glVertex2f(image->width*minratio,image->height*minratio);

	glColor3f(1,1,0);
	if( image->flip )	
		glTexCoord2f(0, image->height); 		
	else
		glTexCoord2f(0, 0); 			
	glVertex2f(0,image->height*minratio);

	glEnd();
	
	//glDisable(GL_TEXTURE_RECTANGLE_EXT);
	glDisable(tex);
	glDisable(GL_BLEND);

	//Draw the gridlines, if requested
	//float vspace = minratio / image->width;
	//float hspace = minratio / image->height;

	//float vspace = min(frame->window_width, frame->window_height) / (float)image->width;
	//float hspace = min(frame->window_width, frame->window_height) / (float)image->height;

	//float vspace = frame->window_width / (float)image->width;
	//float hspace = frame->window_height / (float)image->height;
	//
	//float vspace = image->width * minratio; 
	//float hspace = image->height * minratio; 

	//printf("minratio = %f vspace = %f hspace = %f\n", minratio, vspace, hspace);
	if( gridlines && minratio*zoom > 10 )
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_DST_COLOR);
		//glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_DST_COLOR);

		glColor3f(1,1,1);
		glBegin(GL_LINES);

		for(int i=0; i<image->width; i++)
		{
			glVertex2f( i*minratio, 0 );
			glVertex2f( i*minratio, image->height*minratio );
		}

		for(int j=0; j<image->height; j++)
		{
			glVertex2f( 0, j*minratio);
			glVertex2f( image->width*minratio, j*minratio );
		}

		glEnd();
		glDisable(GL_BLEND);
	}

	glMatrixMode(GL_COLOR);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
}

inline float apply_settings(float pixel, DisplaySettings& settings)
{
	if( settings.gamma == 1 )
	return settings.scale*(pixel + settings.bias); //TODO: gamma goes here
	else
	return settings.scale*( pow(pixel,settings.gamma) +settings.bias);
	//return pixel;
}

void rasterize_image(Frame* frame) //this function is not actually in use
{
	Image* img = (frame->current_index >= 0 ? &images[frame->current_index] : NULL );

	float invratio = 1;
	if( img )
	{
		double wratio = (double)frame->window_width / img->width;
		double hratio = (double)frame->window_height / img->height;
		double minratio = ( wratio < hratio ? wratio : hratio );
		invratio = 1.0/minratio;
	}

	DisplaySettings* settings = GetSettings(img, frame);
	int channels = settings->channels;
	if( img && (channels == CHANNELS_RGBA || channels == CHANNELS_RGB) )
	{
		if( img->format == 1 || img->format == 2 )
			channels = CHANNELS_R;
		else if( img->format == 3 )
			channels = CHANNELS_RGB;
	}
//
	//
	//For each pixel in the output framebuffer, sample from the original image
	for(int i=0; i<frame->window_height; i++)
		for(int j=0; j<frame->window_width; j++)
		{
			int idx = i*frame->window_width + j;

			//Draw the checkerboard first
			int cx = i / SIDE;
			int cy = j / SIDE;
			if( (cx + cy) & 1 )
				frame->framebuffer[idx][0] = 		
				frame->framebuffer[idx][1] = 		
				frame->framebuffer[idx][2] = 1.0;
			else
				frame->framebuffer[idx][0] = 		
				frame->framebuffer[idx][1] = 		
				frame->framebuffer[idx][2] = 0.5;

			if( !img ) continue;

			float x = zoom*(invratio)*(i + frame->display_pos_y + frame->diff_y);	
			float y = zoom*(invratio)*(j + frame->display_pos_x - frame->diff_x);	

			int ix = (int)round(x);
			int iy = (int)round(y);
			if( ix < 0 || iy < 0 || ix >= img->height || iy >= img->width ) continue;

			int pixel = ix*img->width+iy;

			if( channels == CHANNELS_RGB )
				for(int c=0; c<3; c++)
					frame->framebuffer[idx][c] = apply_settings(img->data[pixel][c],*settings);
			else if( channels == CHANNELS_RGBA )
			{
				float alpha = img->data[pixel][3];
				if( alpha > 1 ) alpha = 1;
				else if( alpha < 0 ) alpha = 0;

				for(int c=0; c<3; c++)
					frame->framebuffer[idx][c] = alpha*apply_settings(img->data[pixel][c],*settings) + (1-alpha)*frame->framebuffer[idx][c];
			}
			else if( channels == CHANNELS_R )
			{
				frame->framebuffer[idx][0] = apply_settings(img->data[pixel][0],*settings);
				frame->framebuffer[idx][1] = frame->framebuffer[idx][2] = 0.0;
			}
			else if( channels == CHANNELS_G )
			{
				frame->framebuffer[idx][1] = apply_settings(img->data[pixel][1],*settings);
				frame->framebuffer[idx][0] = frame->framebuffer[idx][2] = 0.0;
			}
			else if( channels == CHANNELS_B )
			{
				frame->framebuffer[idx][2] = apply_settings(img->data[pixel][2],*settings);
				frame->framebuffer[idx][0] = frame->framebuffer[idx][1] = 0.0;
			}
			else if( channels == CHANNELS_A )
				frame->framebuffer[idx][0] = frame->framebuffer[idx][1] = frame->framebuffer[idx][2] = apply_settings(img->data[pixel][3],*settings);


		}
}

void display_func()
{
	//printf("Display function; window=%d\n", glutGetWindow());

	SetCurrentFrame();

	SetWindowTitle(frame);
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	InitWindow();
	if( background ) DrawBackground();
	DrawImage();
	
	//rasterize_image(frame);
	//glDrawPixels(frame->window_width, frame->window_height, GL_RGB, GL_FLOAT, frame->framebuffer);
	glutSwapBuffers();
}

void exit_func()
{
	CleanupServer();
	for(unsigned i=0; i<images.size(); i++)
		delete [] images[i].data;
	images.clear();
}

void signal_func(int sig)
{
	if( sig != SIGINT )
		printf("Imagedb closing with signal %d.  \n", sig);
	exit(0);
}

void idle_func()
{
	ProcessServer();
}

void menu_func(int value)
{
	Image *img = GetImage(frame); //( frame->current_index >= 0 ? &images[frame->current_index] : NULL );
	DisplaySettings* settings = GetSettings(img, frame );
	if( !frame && (display_settings_mode == SETTINGS_PER_FRAME) ) settings = NULL;
	if( !img && (display_settings_mode == SETTINGS_PER_IMAGE) ) settings = NULL;
	
	switch(value)
	{
		case MENU_CHANNEL_RGBA:
			if( settings ) settings->channels = CHANNELS_RGBA;
			break;
		case MENU_CHANNEL_RGB:
			if( settings ) settings->channels = CHANNELS_RGB;
			break;
		case MENU_CHANNEL_R:
			if( settings ) settings->channels = CHANNELS_R;
			break;
		case MENU_CHANNEL_G:
			if( settings ) settings->channels = CHANNELS_G;
			break;
		case MENU_CHANNEL_B:
			if( settings ) settings->channels = CHANNELS_B;
			break;
		case MENU_CHANNEL_A:
			if( settings ) settings->channels = CHANNELS_A;
			break;

		case MENU_SAVE_TGA:
		{
			if( frame->current_index < 0 ) break;

			Image * image = &images[frame->current_index];
			int nextnum = GetNextFileNumber("image%04d.tga", 0);
			char filename[1024];
			sprintf(filename, "image%04d.tga", nextnum);
			printf("Saving image as image%04d.tga\n", nextnum);
			SaveImage(image, filename);
			break;
		}
		case MENU_SAVE_BMP:
		{
			if( frame->current_index < 0 ) break;

			Image * image = &images[frame->current_index];
			int nextnum = GetNextFileNumber("image%04d.bmp", 0);
			char filename[1024];
			sprintf(filename, "image%04d.bmp", nextnum);
			printf("Saving image as image%04d.bmp\n", nextnum);
			SaveImage(image, filename);
			break;
		}

		case MENU_SAVE_PFM:
		{
			if( frame->current_index < 0 ) break;

			Image * image = &images[frame->current_index];
			int nextnum = GetNextFileNumber("image%04d.pfm", 0);
			char filename[1024];
			sprintf(filename, "image%04d.pfm", nextnum);
			printf("Saving image as image%04d.pfm\n", nextnum);
			SavePFM(image, filename);

			break;
		}

		case MENU_SAVE_STACK:
		{
			int nextnum = GetNextFileNumber("stack%d.stk", 0);
			char filename[1024];
			sprintf(filename, "stack%d.stk", nextnum);
			printf("Saving image as stack%d.stk\n", nextnum);
			SaveAll(filename);
			break;
		}

		case MENU_SETTINGS_GLOBAL:
			display_settings_mode = SETTINGS_GLOBAL;
			break;
		case MENU_SETTINGS_PER_FRAME:
			display_settings_mode = SETTINGS_PER_FRAME;
			break;
		case MENU_SETTINGS_PER_IMAGE:
			display_settings_mode = SETTINGS_PER_IMAGE;
			break;

		case MENU_DISPLAY_GRIDLINES:
			gridlines = !gridlines;
			break;

		case MENU_DISPLAY_BACKGROUND:
			background = !background;
			break;

	}

	PostAllRedisplay();
}

void RegisterCallbacks()
{
	glutKeyboardFunc( keyboard_handler );
	glutKeyboardUpFunc( keyboard_up_handler );
	glutSpecialFunc( special_handler );
	glutSpecialUpFunc( special_up_handler );

	glutMouseFunc(mouse_func);
	glutMotionFunc(mouse_motion_func);
	glutPassiveMotionFunc(mouse_passive_func);
	glutReshapeFunc(resize_func);
	glutDisplayFunc(display_func);

	int channel_menu_id = glutCreateMenu(menu_func);
	glutSetMenu(channel_menu_id);
	glutAddMenuEntry("RGBA", MENU_CHANNEL_RGBA);
	glutAddMenuEntry("RGB", MENU_CHANNEL_RGB);
	glutAddMenuEntry("Red", MENU_CHANNEL_R);
	glutAddMenuEntry("Green", MENU_CHANNEL_G);
	glutAddMenuEntry("Blue", MENU_CHANNEL_B);
	glutAddMenuEntry("Alpha", MENU_CHANNEL_A);

	int save_menu_id = glutCreateMenu(menu_func);
	glutSetMenu(save_menu_id);
	glutAddMenuEntry("As TGA", MENU_SAVE_TGA);
	glutAddMenuEntry("As BMP", MENU_SAVE_BMP);
	glutAddMenuEntry("As PFM", MENU_SAVE_PFM);
	glutAddMenuEntry("Stack", MENU_SAVE_STACK);

	int settings_menu_id = glutCreateMenu(menu_func);
	glutSetMenu(settings_menu_id);
	glutAddMenuEntry("Global", MENU_SETTINGS_GLOBAL);
	glutAddMenuEntry("Per Frame", MENU_SETTINGS_PER_FRAME);
	glutAddMenuEntry("Per Image", MENU_SETTINGS_PER_IMAGE);

	int display_menu_id = glutCreateMenu(menu_func);
	glutSetMenu(display_menu_id);
	glutAddMenuEntry("Gridlines", MENU_DISPLAY_GRIDLINES);
	glutAddMenuEntry("Background", MENU_DISPLAY_BACKGROUND);

	menuid = glutCreateMenu(menu_func);
	glutSetMenu(menuid);
	glutAddSubMenu("Channels", channel_menu_id);
	glutAddSubMenu("Save", save_menu_id);
	glutAddSubMenu("Display", display_menu_id);
	glutAddSubMenu("Settings", settings_menu_id);

	glutAttachMenu(GLUT_RIGHT_BUTTON);


	atexit(exit_func);
	signal(SIGINT, signal_func);
	signal(SIGTERM, signal_func);
}

int main(int argc, char** argv)
{
#ifdef WIN32
	FreeConsole();
#endif

	InitServer();
	//image = &images[0];


	glutInit(&argc, argv);
	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
	glutInitWindowSize(SCREEN_RES_X,SCREEN_RES_Y);		

	int win = glutCreateWindow("Image Debugger");
	RegisterCallbacks();	

	int val;
	glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT, &val);
	//glGetIntegerv(GL_MAX_TEXTURE_SIZE, &val);

	glutIdleFunc(idle_func);

	
	//Setup the initial frame
	Frame frame1;
	frame1.handle = win;
	frame1.current_index = 0;
	frame1.window_width = SCREEN_RES_X;	
	frame1.window_height = SCREEN_RES_Y;	
	frame1.display_pos_x = 0;
	frame1.display_pos_y = 0;
	frames.push_back(frame1);
	frame = &frames[0];

	//std::string lastformat="";
	for(int i=1; i<argc; i++)
	{
		if( strstr(argv[i], ".stk") )
			ReadAll( argv[i] );
		else if( strstr(argv[i], ".pfm") )
			ReadPFM( argv[i] );
	}

	glutMainLoop();
}

