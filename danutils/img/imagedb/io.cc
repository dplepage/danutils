//=============================================================================
// io.cc - Input/Output routines for imagedb
//=============================================================================

#include "common.h"
#include <math.h>

int ComputeImageSize(Image* image)
{
	int size = image->width*image->height*image->format*imagedb_bytes_per_pixel[IMAGEDB_FLOAT];
	//printf("size: %d\n", size);
	return size;
}

int GetNextFileNumber(char* filebase, int start)
{
	char buffer[1024];

	while(true)
	{
		struct stat  buf;
		sprintf(buffer, filebase, start);
		if( stat(buffer, &buf) == -1 )
			return start;
		start++;
	}
}

static inline unsigned char topixel(float x, DisplaySettings* settings)
{
	float rescaled = powf(x,1.0/settings->gamma)*settings->scale + settings->bias;
	if( rescaled > 1 ) rescaled = 1;
	else if( rescaled < 0 ) rescaled = 0;
	return (unsigned char)(255*rescaled);
}

static inline unsigned char toalpha(float x)
{
	if( x > 1 ) x = 1;
	else if( x < 0 ) x = 0;
	return (unsigned char)(255*x);
}

bool WriteTGA(char* filename, int width, int height, int colors, unsigned char* pixels)
{
	FILE *fptr = fopen(filename, "wb+");
	if( !fptr )
	{
		fprintf(stderr, "Error: Unable to open %s\n", filename);
		return false;
	}

	putc(0,fptr);
	putc(0,fptr);
	putc(2,fptr);                         /* uncompressed RGB */
	putc(0,fptr); putc(0,fptr);
	putc(0,fptr); putc(0,fptr);
	putc(0,fptr);
	putc(0,fptr); putc(0,fptr);           /* X origin */
	putc(0,fptr); putc(0,fptr);           /* y origin */
	putc((width & 0x00FF),fptr);
	putc((width & 0xFF00) / 256,fptr);
	putc((height & 0x00FF),fptr);
	putc((height & 0xFF00) / 256,fptr);
	putc(colors*8,fptr);                        /* 24 bit bitmap */
	putc(0,fptr);

	int size = width*height*colors;
	int written = fwrite(pixels, 1, size, fptr);
	fclose(fptr);
	if( size != written )
	{
		fprintf(stderr, "Error: Attempted to write %d bytes to %s, only wrote %d\n", size, filename, written);
		return false;
	}
	else return true;
}

bool WriteBMP(const char *filename, int width, int height, int colors, unsigned char* pixels)
{
	// Open file
	FILE *fp = fopen(filename, "wb");
	if (!fp) {
		fprintf(stderr, "Unable to open image file: %s", filename);
		return false;
	}

	// Compute number of bytes in row
	int rowsize = colors * width;
	if ((rowsize % 4) != 0) rowsize = (rowsize / 4 + 1) * 4;
	int size = 54 + rowsize * height;

	// Write file header 
	fputc(0x42, fp);
	fputc(0x4D, fp); //magic number
	fputc( (size & 0x000000FF), fp );
	fputc( (size & 0x0000FF00) >> 8, fp );
	fputc( (size & 0x00FF0000) >> 16, fp );
	fputc( (size & 0xFF000000) >> 24, fp ); //32-bit total file size
	fputc(0, fp);
	fputc(0, fp);
	fputc(0, fp);
	fputc(0, fp); //4 reserved bytes
	fputc(54, fp);
	fputc(0, fp);
	fputc(0, fp);
	fputc(0, fp); //32-bit offset (54) to start of data

	//Write info header
	fputc(40, fp);
	fputc(0, fp);
	fputc(0, fp);
	fputc(0, fp); //32-bit header size (40 bytes)
	fputc( (width & 0x000000FF), fp );
	fputc( (width & 0x0000FF00) >> 8, fp );
	fputc( (width & 0x00FF0000) >> 16, fp );
	fputc( (width & 0xFF000000) >> 24, fp ); //32-bit width
	fputc( (height & 0x000000FF), fp );
	fputc( (height & 0x0000FF00) >> 8, fp );
	fputc( (height & 0x00FF0000) >> 16, fp );
	fputc( (height & 0xFF000000) >> 24, fp ); //32-bit height
	fputc(1, fp);
	fputc(0, fp);                        //16-bit Colors planes =1 
	fputc((unsigned char)colors*8, fp);
	fputc(0, fp);                        //16-bit bits-per-pixel count

	//Write 24 zero bytes to indicate (32-bits for each)
	//  No compression, reader computes size, xpels, ypels, colors used, important colors
	for(int i=0; i<24; i++)
		fputc(0, fp);

	/*
	fputc(0, fp);
	fputc(0, fp);
	fputc(0, fp);
	fputc(0, fp); //no compression
	fputc(0, fp);
	fputc(0, fp);
	fputc(0, fp);
	fputc(0, fp); //size, let the reader figure it out
	fputc(0, fp);
	fputc(0, fp);
	fputc(0, fp);
	fputc(0, fp); //x pels per meter (dont care)
	fputc(0, fp);
	fputc(0, fp);
	fputc(0, fp);
	fputc(0, fp); //y pels per meter (dont care)
	fputc(0, fp);
	fputc(0, fp);
	fputc(0, fp);
	fputc(0, fp); //colors used (dont care)
	fputc(0, fp);
	fputc(0, fp);
	fputc(0, fp);
	fputc(0, fp); //important colors (dont care)
	*/

	int pad = rowsize - width * colors;

	for(int i=0; i<height; i++)
	{
		for(int j=0; j<width; j++)
		{
			for(int c=0; c<colors; c++)
			{
				fputc(pixels[(i*width+j)*colors+c], fp);
			}
		}

		for (int j = 0; j < pad; j++) fputc(0, fp);
	}

	// Close file
	fclose(fp);

	// Return success
	return true;  
}



void SaveImage(Image* image, char* filename)
{
	DisplaySettings* settings = GetSettings(image, frame);
	bool limitRGB = false;
	int c= (!limitRGB && settings->channels == CHANNELS_RGBA) ? 4 : 3;

	unsigned char* pixels = new unsigned char[image->width*image->height*c];
	for(int i=0; i<image->width*image->height; i++)
	{
		if( settings->channels == CHANNELS_RGBA && c == 4 )
		{
			pixels[i*c+0] = topixel(image->data[i][2],settings);
			pixels[i*c+1] = topixel(image->data[i][1],settings);
			pixels[i*c+2] = topixel(image->data[i][0],settings);
			pixels[i*c+3] = toalpha(image->data[i][3]);
		}
		else if( c == 3 )
		{
			pixels[i*c+0] = topixel(image->data[i][2],settings);
			pixels[i*c+1] = topixel(image->data[i][1],settings);
			pixels[i*c+2] = topixel(image->data[i][0],settings);
		}
		else if( settings->channels == CHANNELS_R )
		{
			pixels[i*c+2] = topixel(image->data[i][0],settings);
			pixels[i*c+1] = 0;
			pixels[i*c+0] = 0;
		}
		else if( settings->channels == CHANNELS_G )
		{
			pixels[i*c+2] = 0;
			pixels[i*c+1] = topixel(image->data[i][1],settings);
			pixels[i*c+0] = 0;
		}
		else if( settings->channels == CHANNELS_B )
		{
			pixels[i*c+2] = 0;
			pixels[i*c+1] = 0;
			pixels[i*c+0] = topixel(image->data[i][2],settings);
		}
		else
		{
			pixels[i*c+0] = toalpha(image->data[i][3]);
			pixels[i*c+1] = toalpha(image->data[i][3]);
			pixels[i*c+2] = toalpha(image->data[i][3]);
		}
	}

	/*
	for(int c=0; c<image->format; c++)
	{
			int pixc=c;
			if( image->format >= 3 )
			{
				if( c <=2 ) pixc = 2 - c;
			}

			//Apply the current pixel transformations
			DisplaySettings* settings = GetSettings(image, frame);
			float pixvalue=0.0, scale=0, bias=0, gamma=1;
			if( c == 0 && (settings->channels == CHANNELS_RGBA || settings->channels == CHANNELS_RGB || settings->channels == CHANNELS_R ))
				scale = settings->scale;
			else if( c == 1 && (settings->channels == CHANNELS_RGBA || settings->channels == CHANNELS_RGB || settings->channels == CHANNELS_G ))
				scale = settings->scale;
			else if( c == 2 && (settings->channels == CHANNELS_RGBA || settings->channels == CHANNELS_RGB || settings->channels == CHANNELS_G ))
				scale = settings->scale;

			pixels[i*image->format+c] = (unsigned char)( image->data[i][pixc] * 255 );

			//pixels[i*image->format+c] = (unsigned char)( image->data[i][pixc] * 255 );
	}
	*/

	if( strstr(filename,".bmp") )
		WriteBMP(filename, image->width, image->height, c, pixels);
	else
		WriteTGA(filename, image->width, image->height, c, pixels);
	
	delete [] pixels;
	return;
}

void SavePFM(Image* img, char* filename)
{
	FILE* file = fopen(filename, "wb");
	if( !file )
	{
		fprintf(stderr, "Error: Unable to open %s\n", filename);
		return ;
	}
	fprintf(file, "PF\n%d %d\n-1.000000\n", img->width, img->height);

	float (*data)[3] = new float[img->width*img->height][3];
	for(int i=0; i<img->height*img->width; i++)
	{
		for(int c=0; c<3; c++)
			data[i][c] = img->data[i][c];

		data[i][3] = 1.0f;
	}
	fwrite(&data[0], sizeof(float), img->width*img->height*3, file);
	fclose(file);
	delete [] data;
	return ;
}

void SaveAll(char* filename)
{
	FILE *file = fopen(filename, "wb+");
	int length=images.size();
	fwrite(&length, sizeof(int), 1, file);
	for(int i=0; i<(int)images.size(); i++)
	{
		fwrite(&images[i], sizeof(Image), 1, file);
		//fwrite(images[i].data, ComputeImageSize(&images[i]), 1, file);
		fwrite(images[i].data, images[i].height*images[i].width*sizeof(float[4]), 1, file);
	}
	fclose(file);
}

void ReadAll(char* filename)
{
	FILE *file = fopen(filename, "rb");
	int length;
	images.clear();
	fread(&length, sizeof(int), 1, file);
	for(int i=0; i<length; i++)
	{
		int read=0;
		Image * image = new Image();
		read = fread(image, sizeof(Image), 1, file);
		//image->data = new unsigned char[ComputeImageSize(image)];
		//read = fread(image->data, ComputeImageSize(image), 1, file);
		image->data = (new float[image->height*image->width][4]);
		read = fread(image->data, image->height*image->width*sizeof(float[4]), 1, file);
		images.push_back(*image);
		delete image;
	}
	//image = &images[0];
	frame->current_index = 0;
	fclose(file);
	SetWindowTitle( frame );
	glutPostRedisplay();
}

void ReadPFM(char* filename)
{
	int rows, cols;

	FILE* file = fopen(filename, "rb");
	if( !file )
	{
		fprintf(stderr, "Error: Unable to open %s\n", filename);
		return ;
	}
	
	fscanf(file, "PF\n");
	fscanf(file, "%d %d\n", &cols, &rows);
	fscanf(file, "-1.000000\n");

	Image img;
	img.width = cols;
	img.height = rows;
	img.format = IMAGEDB_RGB;
	strcpy(img.title, filename);
	
	float* data = new float[rows*cols*3];
	fread(data, rows*cols*3, sizeof(float), file);
	fclose(file);

	img.data = (new float[img.height*img.width][4]);
	for(int i=0; i<rows; i++)
		for(int j=0; j<cols; j++)
		{
			for(int c=0; c<3; c++)
			{
				img.data[i*img.width+j][c] = data[3*cols*i+3*j+c];
			}
			img.data[i*img.width+j][3] = 1.0f;
		}

	delete[] data;
	images.push_back(img);
	//frame->current_index = images.size()-1;
	return ;
}

