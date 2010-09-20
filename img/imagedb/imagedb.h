#ifndef IMAGEDB_H
#define IMAGEDB_H



#ifdef WIN32

#pragma warning(disable: 4996)

static inline void usleep(int microseconds)
{
	int milliseconds = microseconds/1000;
	if( milliseconds < 1 ) milliseconds=1;
	Sleep(milliseconds);
}

static inline void close(int sock)
{
	closesocket(sock);
}


#else

#include <unistd.h>
//#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>


inline void imagedb_set_server(const char* svrname)
{
	setenv("IMAGEDB_SERVER", svrname, 1);
}

inline void imagedb_set_program(const char* svrname)
{
	setenv("IMAGEDB_PROGRAM", svrname, 1);
}
#endif

inline void imagedb_initialize_sockets()
{
#ifdef WIN32
    static int init = false;
    if( init ) return;

    WSADATA wsaData;
    int err;
    err = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
    if ( err != 0 ) {
        printf("Error: Unable to initialize Winsock\n");
        exit(0);
    }
    init = true;
#endif
}

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

struct imagedb_hdr
{
	imagedb_hdr() { memset(this, 0, sizeof(*this)); }
	int rows;
	int cols;
	int type;
	int format;
	int colstride;
	int rowstride;
	int duplicate;
	int flip;
	char title[128];
	char frame[128];
	char server[128];
	char program[128];
};

enum {IMAGEDB_LUM=1, IMAGEDB_LUMA, IMAGEDB_RGB, IMAGEDB_RGBA};
enum {IMAGEDB_8BIT=0, IMAGEDB_16BIT, IMAGEDB_32BIT, IMAGEDB_FLOAT, IMAGEDB_DOUBLE };
enum {IMAGEDB_DUP_IGNORE=0, IMAGEDB_DUP_REPLACE};
static const int imagedb_bytes_per_pixel[] = { 1, 2, 4, 4, 8 }; //use the above enum to lookup
//static int components[] = {0, 1, 2, 3, 4};


static const int IMAGEDB_PORT = 50123;

static void imagedb(imagedb_hdr& hdr, void* data); 
static void imagedb(int rows, int cols, int type, int format, char* title, void* data, char* frame=NULL);

//=============================================================================
// void imagedb(void* data, const char* format, ...)
// This version is a convenience to the second, which it calls
//=============================================================================
static void imagedb(void* data, const char* format, ...)
{
	imagedb_initialize_sockets();

	char buffer[4096]; 

	va_list arglist;
	va_start(arglist, format);
	vsprintf (buffer, format, arglist);
	va_end(arglist);

	bool inquote=false;
	for(char* start=&buffer[0]; start-&buffer[0]<4096 && *start; start++)
	{
		if( *start == '\'' ) inquote = !inquote;
		else if( inquote && *start == ' ' ) *start = 0x1;
		else if( inquote && *start == '=' ) *start = 0x2;
	}

	char* tok=NULL;
	tok = strtok(buffer, " ");

	imagedb_hdr hdr;
	hdr.type = IMAGEDB_8BIT;
	hdr.format = IMAGEDB_RGB;

	int skip=0;
	while( tok )
	{
		int theint;
		char thestring[4096];

		if( sscanf(tok, "b=%s", thestring) == 1 ||
			sscanf(tok, "bits=%s", thestring) == 1 )
		{
			if( !strcmp("8", thestring) )
				hdr.type = IMAGEDB_8BIT;	
			else if( !strcmp("16", thestring) )
				hdr.type = IMAGEDB_16BIT;	
			else if( !strcmp("32", thestring) )
				hdr.type = IMAGEDB_32BIT;	
			else if( !strcmp("32f", thestring) )
				hdr.type = IMAGEDB_FLOAT;	
			else if( !strcmp("64f", thestring) )
				hdr.type = IMAGEDB_DOUBLE;	
		}
		else if( sscanf(tok, "f=%s", thestring) == 1 ||
                 sscanf(tok, "fmt=%s", thestring) == 1 ||
                 sscanf(tok, "format=%s", thestring) == 1   )
		{
			if( !strcmp("l", thestring) || !strcmp("lum", thestring) )
				hdr.format = IMAGEDB_LUM;
			else if( !strcmp("la", thestring) || !strcmp("luma", thestring) )
				hdr.format = IMAGEDB_LUMA;
			else if( !strcmp("rgb", thestring) )
				hdr.format = IMAGEDB_RGB;
			else if( !strcmp("rgba", thestring) )
				hdr.format = IMAGEDB_RGBA;
		}
		else if( sscanf(tok, "w=%d", &theint) == 1 ||
		         sscanf(tok, "width=%d", &theint) == 1   )
		{
			if( theint > 0 )
				hdr.cols = theint;
		}
		else if( sscanf(tok, "h=%d", &theint) == 1 ||
		         sscanf(tok, "height=%d", &theint) == 1 )
		{
			if( theint > 0 )
				hdr.rows = theint;
		}
		else if( sscanf(tok, "t=%s", thestring) == 1 ||
		         sscanf(tok, "title=%s", thestring) == 1 )
		{
			int idx=0;
			for(char* start=thestring; start-thestring<128 && *start; start++)
			{
				if( *start == 0x1 )
					hdr.title[idx++] = ' ';
				else if( *start == 0x2 )
					hdr.title[idx++] = '=';
				else 
					hdr.title[idx++] = *start;
			}
		}
		else if( sscanf(tok, "fr=%s", thestring) == 1 ||
		         sscanf(tok, "frame=%s", thestring) == 1 )
		{
			int idx=0;
			for(char* start=thestring; start-thestring<128 && *start; start++)
			{
				if( *start == 0x1 )
					hdr.frame[idx++] = ' ';
				else if( *start == 0x2 )
					hdr.frame[idx++] = '=';
				else 
					hdr.frame[idx++] = *start;
			}
		}
		else if( sscanf(tok, "d=%s", thestring) == 1 ||
                 sscanf(tok, "dup=%s", thestring) == 1 ||
                 sscanf(tok, "duplicate=%s", thestring) == 1   )
		{
			if( !strcmp("i", thestring) || !strcmp("ignore", thestring) )
				hdr.duplicate = IMAGEDB_DUP_IGNORE;
			else if( !strcmp("r", thestring) || !strcmp("replace", thestring) )
				hdr.duplicate = IMAGEDB_DUP_REPLACE;
		}
		else if( sscanf(tok, "svr=%s", thestring) == 1 ||
		         sscanf(tok, "server=%s", thestring) == 1 )
		{
			int idx=0;
			for(char* start=thestring; start-thestring<128 && *start; start++)
			{
				if( *start == 0x1 )
					hdr.server[idx++] = ' ';
				else if( *start == 0x2 )
					hdr.server[idx++] = '=';
				else 
					hdr.server[idx++] = *start;
			}
		}
		else if( sscanf(tok, "pg=%s", thestring) == 1 ||
		         sscanf(tok, "program=%s", thestring) == 1 )
		{
			int idx=0;
			for(char* start=thestring; start-thestring<128 && *start; start++)
			{
				if( *start == 0x1 )
					hdr.program[idx++] = ' ';
				else if( *start == 0x2 )
					hdr.program[idx++] = '=';
				else 
					hdr.program[idx++] = *start;
			}
		}
		else if( sscanf(tok, "sk=%d", &theint) == 1 )
		{
			skip = theint;
		}
		else if( sscanf(tok, "rs=%d", &theint) == 1 )
		{
			//printf("Row skip not yet implemented\n");
			hdr.rowstride = theint;
		}
		else if( sscanf(tok, "cs=%d", &theint) == 1 )
		{
			//printf("Column skip not yet implemented\n");
			hdr.colstride = theint;
		}
		else if( !strcmp(tok, "flip") )
			hdr.flip = 1;

		tok = strtok(NULL, " ");
	}
	
	imagedb(hdr, ((char*)data) + imagedb_bytes_per_pixel[hdr.format]*skip);

	//imagedb(hdr.rows, hdr.cols, hdr.type, hdr.format, hdr.title, ((char*)data) + bytes_per_pixel[hdr.format]*skip, hdr.frame);
	return;
}

inline void imagedb(int rows, int cols, int type, int format, char* title, void* data, char* frame)
{
	imagedb_hdr hdr;
	hdr.rows = rows;
	hdr.cols = cols;
	hdr.type = type;
	hdr.format = format;
	if( title ) strcpy(hdr.title, title);
	if( frame ) strcpy(hdr.frame, frame);
	imagedb(hdr, data);
}

static void imagedb(imagedb_hdr& hdr, void* data)
{
	char* servername = NULL;
	
	if( strlen(hdr.server) > 0 )
	{
		servername = &hdr.server[0];
	}
	else
	{
		servername = getenv("IMAGEDB_SERVER");
		if( servername == NULL )
			servername = "127.0.0.1";
		else if( !strcmp(servername,"0") )
		{
			return;
		}
	}

	int type = hdr.type;
	if( type != IMAGEDB_8BIT && type != IMAGEDB_16BIT && 
            type != IMAGEDB_32BIT && type != IMAGEDB_FLOAT &&
            type != IMAGEDB_DOUBLE )
	{
		fprintf(stderr, "Error: ImageDB requires a valid data type\n");
		return;
	}

	int format = hdr.format;
	if( format != IMAGEDB_LUM && format != IMAGEDB_LUMA && 
            format != IMAGEDB_RGB && format != IMAGEDB_RGBA )
	{
		fprintf(stderr, "Error: ImageDB requires a valid data format\n");
		return;
	}

	int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in addr;	
	addr.sin_family = AF_INET;
	addr.sin_port = ntohs(IMAGEDB_PORT);

	hostent* host = gethostbyname(servername);
	addr.sin_addr.s_addr = *(unsigned long*)host->h_addr;

	int ret = connect(sock, (sockaddr*)&addr, sizeof(sockaddr));
	if( ret == -1 )
	{
		char* clientname = NULL;
		if( strlen(hdr.program) > 0 )
			clientname = &hdr.program[0];
		else
			clientname = getenv("IMAGEDB_PROGRAM");

		if( !clientname )
			clientname = "imagedb";	

#ifdef WIN32
		int ret = WinExec(clientname, SW_SHOWNORMAL);
		if( ret < 31 ) //this is silly, but the documentation page says check >31 for success
		{
			printf("Error: Unable to spawn imagedb server\n");
		}
#else
		int ppid;
		if( (ppid=fork()) == 0 )
		{
			int ret = execlp(clientname, "imagedb", (char*)NULL);
			if( ret == - 1)
			{
				ret = execlp("./imagedb", "imagedb", (char*)NULL);
				if( ret == -1 )
					printf("Error: Unable to spawn imagedb server\n");
			}
			exit(0);
		}
#endif

		for(int tries=0; tries<5; tries++)
		{
			ret = connect(sock, (sockaddr*)&addr, sizeof(sockaddr));
			usleep(1000*1000);
			if( ret != -1 )
				break;
		}

		if( ret == -1 )
		{
			printf("Error: Unable to connect to imagedb server\n");
			return;
		}
	}

	//Send the header over to the server
	ret = send(sock, (char*)&hdr, sizeof(hdr), 0);
	if( ret == -1 )
		printf("Error: %s\n", strerror(errno)); 

	//Process the data, if necessary
	if( hdr.rowstride > 1 || hdr.colstride > 1 )
	{
		if( hdr.rowstride == 0 ) hdr.rowstride = 1;
		if( hdr.colstride == 0 ) hdr.colstride = 1;

		char*  dataptr = (char*)data;
		int bpp = hdr.format*imagedb_bytes_per_pixel[hdr.type];
		char* datacopy = new char[hdr.rows*hdr.cols*bpp];	

		for(int i=0; i<hdr.rows; i++)
			for(int j=0; j<hdr.cols; j++)
				for(int k=0; k<bpp; k++)
					datacopy[i*hdr.cols*bpp + j*bpp + k] = 
						dataptr[i*hdr.rowstride*hdr.cols*hdr.colstride*bpp + j*hdr.colstride*bpp + k];
		data = (void*)datacopy;
	}

	errno=0;
	int datasize = hdr.rows*hdr.cols*hdr.format*imagedb_bytes_per_pixel[hdr.type];
	int sent = ret = send(sock, (char*)data, datasize, 0);
	if( sent != datasize && ret != -1 )
	{
		for(int i=0; i<3 && sent < datasize; i++)
		{
			//printf("Sending more...\n");
			errno = 0;
			ret = send(sock, ((char*)(data))+sent, datasize-sent, 0);
			if( ret == -1 ) break;
			sent += ret;
			usleep(1000);
		}
	}
	if( ret == -1 )
		printf("Imagedb Send Error: %d - %s \n", errno, strerror(errno));
	else if(sent != datasize )
		printf("Imagedb Send Error: %d - %s Tried to send datasize=%d, actually sent %d\n", errno, strerror(errno), datasize, ret);

	close(sock);	

	if( hdr.rowstride > 1 || hdr.colstride > 1 )
		delete [] (char*)data;

	return;
}

#ifdef _CDLIB_IMAGE_H
static void imagedb(Image& img, char* format, ...)
{
	char buffer[256];

	va_list arglist;
	va_start(arglist, format);
	vsprintf (buffer, format, arglist);
	va_end(arglist);

	char finalbuffer[256];
	sprintf(finalbuffer, "%s b=32f f=rgba w=%d h=%d\n", buffer, img.Cols(), img.Rows());

	imagedb(img.Data(), finalbuffer);
}

#endif


inline void _imagedb_noop() {
	fprintf(stderr, "Error: This function exists solely to quiet warnings\n");
	abort();
	imagedb(NULL, "");
}

#endif
