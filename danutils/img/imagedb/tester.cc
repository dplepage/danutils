#include <stdlib.h>
#include "imagedb.h"

typedef unsigned short	UINT16;
typedef unsigned char 	UINT8;
typedef unsigned int    UINT32;

typedef struct _DevDir
{
	UINT16	tagValue;
	UINT32	tagOffset;
	UINT32	tagSize;
} DevDir;

struct TGAFile
{
	UINT8	idLength;		/* length of ID string */
	UINT8	mapType;		/* color map type */
	UINT8	imageType;		/* image type code */
	UINT16	mapOrigin;		/* starting index of map */
	UINT16	mapLength;		/* length of map */
	UINT8	mapWidth;		/* width of map in bits */
	UINT16	xOrigin;		/* x-origin of image */
	UINT16	yOrigin;		/* y-origin of image */
	UINT16	imageWidth;		/* width of image */
	UINT16	imageHeight;	/* height of image */
	UINT8	pixelDepth;		/* bits per pixel */
	UINT8	imageDesc;		/* image descriptor */
	char	idString[256];	/* image ID string */
	UINT16	devTags;		/* number of developer tags in directory */
	DevDir	*devDirs;		/* pointer to developer directory entries */
	UINT16	extSize;		/* extension area size */
	char	author[41];		/* name of the author of the image */
	char	authorCom[4][81];	/* author's comments */
	UINT16	month;			/* date-time stamp */
	UINT16	day;
	UINT16	year;
	UINT16	hour;
	UINT16	minute;
	UINT16	second;
	char	jobID[41];		/* job identifier */
	UINT16	jobHours;		/* job elapsed time */
	UINT16	jobMinutes;
	UINT16	jobSeconds;
	char	softID[41];		/* software identifier/program name */
	UINT16	versionNum;		/* software version designation */
	UINT8	versionLet;
	UINT32	keyColor;		/* key color value as A:R:G:B */
	UINT16	pixNumerator;	/* pixel aspect ratio */
	UINT16	pixDenominator;
	UINT16	gammaNumerator;	/* gamma correction factor */
	UINT16	gammaDenominator;
	UINT32	colorCorrectOffset;	/* offset to color correction table */
	UINT32	stampOffset;	/* offset to postage stamp data */
	UINT32	scanLineOffset;	/* offset to scan line table */
	UINT8	alphaAttribute;	/* alpha attribute description */
	UINT32	*scanLineTable;	/* address of scan line offset table */
	UINT8	stampWidth;		/* width of postage stamp */
	UINT8	stampHeight;	/* height of postage stamp */
	void	*postStamp;		/* address of postage stamp data */
	UINT16	*colorCorrectTable;
	UINT32	extAreaOffset;	/* extension area offset */
	UINT32	devDirOffset;	/* developer directory offset */
	char	signature[18];	/* signature string	*/
};

//Swaps the endianness of the input short
short swapEndian(short in)
{
#ifndef SOLARIS
	return in;
#else
	short retval = 0;
	retval = ((in & 0x00FF) << 8) | ((in & 0xFF00) >> 8);
	return retval;
#endif

}

//ReadByte - reads a single byte from the specified file
UINT8 ReadByte( FILE* fp )
{
	UINT8	value;
	fread( &value, 1, 1, fp );
	return( value );
}

//Read a short from the file and translates it into the 
//appropriate byte ordering
UINT16 ReadShort( FILE* fp )
{
	UINT16	value;
	int size = fread( &value, 1, 2, fp );
	size = size;
	return( swapEndian(value) );
}


//LoadTGAFile
//Given a filename, loads and returns the pixels from a targa file 
//The file header is loaded and copied into the structure pointed to by f
//This function is entirely standard C, and is not OpenGL, Windows or Unix specific
void * LoadTGAFile(const char* filename, struct TGAFile *f)
{
	FILE* fp;
	char szJunk[65535];
	int size;
	unsigned char *pixels;
	
	//Attempt to open the file
	//Return null on fail
	fp = fopen( filename, "rb" );
	if( !fp )
		return NULL;

	//Load all of the fields from the header
	//Most are unimportant (at least for this function)
	//we need idLength to figure out how much junk is at the end of the header
	//  (so we can skip by it)
	//we especially need imageWidth, imageHeight, and pixelDepth to
	//  compute the number of pixels in the image
	f->idLength = ReadByte( fp );		//Important
	f->mapType = ReadByte( fp );
	f->imageType = ReadByte( fp );
	f->mapOrigin = ReadShort( fp );
	f->mapLength = ReadShort( fp );
	f->mapWidth = ReadByte( fp );
	f->xOrigin = ReadShort( fp );
	f->yOrigin = ReadShort( fp );
	f->imageWidth = ReadShort( fp );    //Important
	f->imageHeight = ReadShort( fp );   //Important
	f->pixelDepth = ReadByte( fp );		//Important
	f->imageDesc = ReadByte( fp );

	//Pull out all of the id data - this is basically junk, and is ignored
	//  but we have to skip over it to get the pixels
	fread(szJunk, 1, f->idLength, fp);

	//Compute the size from the header fields
	//Note that pixelDepth is in BITS, and is divided by 8 to get bytes (>> 3)
	//Then we use this to allocate the pixels buffer
	size = f->imageWidth * f->imageHeight * (f->pixelDepth >> 3);
	pixels = (unsigned char*)malloc( size );

	//(New) If image type is >= 9, then the data is compressed
	if( f->imageType >= 9 )
	{
		//int pixelCount = f->imageWidth * f->imageHeight;
		int pixelCount = size;
		int pixelsRead=0;

		while(pixelsRead < pixelCount)
		{
			//Read the packet-type / pixel-count byte from the next packet
			unsigned char packetType=0;
			packetType = ReadByte(fp);
			
			//Test to see if the highest bit is a 1 or 0 (sign test)
			//If the type is a 1 (packetType is < 0), the packet is a RLE packet
			if( packetType & 0x80 )
			{
				//Compute the number of RL encoded pixels to be created
				int count = (packetType & 0x7F) + 1;
				unsigned char *pixelData = new unsigned char[f->pixelDepth >> 3];
				//char pixelData[f->pixelDepth >> 3];

				//Read the appropriate number of pixel data bytes
				for(int k=0; k< f->pixelDepth >> 3; k++)
				{
					pixelData[k] = ReadByte( fp );
				}
				
				//For each pixel to be created, copy all of the members into the array
				for(int i=0; i<count; i++)
				{
					memcpy(&pixels[pixelsRead], pixelData, (f->pixelDepth>>3));
					pixelsRead += (f->pixelDepth>>3);
				}

				delete [] pixelData;
			}
			else
			{
				//Since we have a raw packet, get the count and read it into the buffer
				//  at the appopriate location.  Then increment the counter
				int count = (f->pixelDepth>>3) * (packetType + 1);
				fread(&pixels[pixelsRead], 1, count, fp);
				pixelsRead += count;
			}
		}
	}
	else
	{
		//Actually read the pixels from the buffer, close the file, and return
		fread(pixels, 1, size, fp);
		fclose(fp);
	}

	return pixels;
}


struct Image
{
	int width;
	int height;
	unsigned char* data;
};

float *images2data;
Image images[3];

int main(int argc, char** argv)
{
	TGAFile file;
	unsigned char* data = (unsigned char*)LoadTGAFile("metalrect.tga", &file);

	unsigned short * shortdata = new unsigned short[file.imageHeight*file.imageWidth*3];
	for(int i=0; i<file.imageHeight*file.imageWidth*3; i++)
		shortdata[i] = data[i] << 8;

	double * doubledata = new double[file.imageHeight*file.imageWidth*3];
	for(int i=0; i<file.imageHeight*file.imageWidth*3; i++)
		doubledata[i] = data[i] / 255.0;

	float * floatdata = new float[file.imageHeight*file.imageWidth*3];
	for(int i=0; i<file.imageHeight*file.imageWidth*3; i++)
		floatdata[i] = data[i] / 255.0;



	imagedb(file.imageHeight, file.imageWidth, IMAGEDB_8BIT, IMAGEDB_RGB, "metal", data);
	imagedb(file.imageHeight, file.imageWidth, IMAGEDB_16BIT, IMAGEDB_RGB, "metal short", shortdata);
	imagedb(file.imageHeight, file.imageWidth, IMAGEDB_FLOAT, IMAGEDB_RGB, "metal float", floatdata);
	imagedb(file.imageHeight, file.imageWidth, IMAGEDB_DOUBLE, IMAGEDB_RGB, "metal double", doubledata);

	imagedb(floatdata, "b=32f f=rgb h=%d w=%d t='Row skip' rs=4 cs=2 fr=new ", file.imageHeight/4, file.imageWidth/2);
	imagedb(floatdata-2, "b=32f f=rgb h=%d w=%d t='This is a test' sk=2 fr=new2", file.imageHeight, file.imageWidth);

	for(int i=0; i<10; i++)
	imagedb(floatdata-2, "b=32f f=rgb h=%d w=%d t='This is a test' sk=2 fr=new2 d=r", file.imageHeight, file.imageWidth);
	//imagedb(floatdata-2, "b=32f f=rgb h=%d w=%d t='This is a test' sk=2 fr=newframe", file.imageHeight, file.imageWidth);

	//BITMAPINFO *info;
	//data = (char*)LoadBMPFile("myfile.bmp", &info);
	//imagedb(info->biHeight, info->biWidth, IMAGEDB_8BIT, IMAGEDB_RGB, "bmp", data);


	images[0].width = 128;
	images[0].height = 256;
	images[0].data = new unsigned char[images[0].width*images[0].height*3];

	images[1].width = 100;
	images[1].height = 100;
	images[1].data = new unsigned char[images[1].width*images[1].height*4];

	images[2].width = 300;
	images[2].height = 100;
	images2data = new float[images[2].width*images[2].height*4];

	for(int i=0; i<images[0].width*images[0].height*3; i+= 3)
	{
		double percent = ((double)i)/(images[0].width*images[0].height*4);
		images[0].data[i] = (unsigned char)(255.0*percent);		
		images[0].data[i+1] = 0;		
		images[0].data[i+2] = 0;		
	}
	for(int i=0; i<images[1].width*images[1].height*4; i+= 4)
	{
		images[1].data[i] = 0;		
		double percent = ((double)i)/(images[1].width*images[1].height*4);
		images[1].data[i+1] = (unsigned char)(255.0*percent);		
		images[1].data[i+2] = 0;		
		images[1].data[i+3] = (unsigned char)(255*percent);		
	}
	for(int i=0; i<images[2].width*images[2].height*4; i+= 4)
	{
		images2data[i] = 0;		
		images2data[i+1] = 0;		
		double percent = ((double)i)/(images[2].width*images[2].height*4);
		images2data[i+2] = percent;		
		images2data[i+3] = 0.4;		
	}
	imagedb(images[0].height, images[0].width, 
			IMAGEDB_8BIT, IMAGEDB_RGB, "Red Gradient", images[0].data);
	sleep(1);
		imagedb(images[1].height, images[1].width, 
			IMAGEDB_8BIT, IMAGEDB_RGBA, "Square Green and Alpha Gradient", images[1].data);
	sleep(1);
		imagedb(images[2].height, images[2].width, 
			IMAGEDB_FLOAT, IMAGEDB_RGBA, "Wide Blue", images2data);
}

