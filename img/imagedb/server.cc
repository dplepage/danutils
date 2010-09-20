//=============================================================================
// server.cc - network/socket server code for imagedb
//=============================================================================

#include "common.h"

int server_socket;

#ifdef WIN32
#undef MSG_WAITALL
bool CriticalRecvError()
{
	//int err = WSAGetLastError();
	//printf("%d\n", err);
	return false;
}
#else 
bool CriticalRecvError()
{
	if( errno == 11 )
		return false;
	else
	printf("Imagedb Server Recv Error: %d %s\n",
					errno, strerror(errno) );
	return true;
}
#endif

int recvall(int s, char* buf, int len, int flags)	
{
#ifndef MSG_WAITALL
	int bytesread=0; int attempts=200;
	while(bytesread < len && attempts-- >=0 )
	{
		errno = 0;
		int ret = recv(s, buf+bytesread, len-bytesread, flags);
		if( ret == -1  )
		{
			if( CriticalRecvError() ) return -1;
			usleep(1000);
		}
		else
			bytesread += ret;
	}
	return bytesread;
#else
	return recv(s, buf, len, flags | MSG_WAITALL);
#endif
}

int InitServer()
{
	imagedb_initialize_sockets();

	int ret;
	int off=1;

	sockaddr_in addr;	
	addr.sin_family = AF_INET;
	addr.sin_port = ntohs(IMAGEDB_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	
	server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&off, sizeof(int));
	ret = bind(server_socket, (sockaddr*)&addr, sizeof(sockaddr));
	if( ret != 0 )
	{
		fprintf(stderr, "Imagedb Server Error: Unable to bind to requested socket\n"
				"\tThis is a common error that seems to happen when the OS is slow cleaning up the old socket\n"
				"\tAlso make sure no other instance of imagedb is running on this machine\n"
				"\tIf you retry in a few seconds, it will probably work.\n"
				);
		exit(0);
	}
	
	ret = listen(server_socket, 10);	
	if( ret != 0 )
	{
		fprintf(stderr, "Imagedb Server Error: Unable to listen to requested socket\n");
		exit(0);
	}

	ioctl(server_socket, FIONBIO, &off);
	return 0;
}

void CleanupServer()
{
#ifdef WIN32
	closesocket(server_socket);
#else
	shutdown(server_socket, SHUT_RDWR);
	close(server_socket);
#endif
}

void ProcessServer()
{
	//if( shift_down ) printf("Shift down\n");

	//Perform the network tasks
	sockaddr_in addr;
	//unsigned int addrlen;	
	socklen_t addrlen = sizeof(addr);	
	memset(&addr, 0, sizeof(addr));
	int ret = 0;

	Image newimg;
	int datasize;
	unsigned char* data=NULL;

	Frame* targetframe = frame;

	usleep(1000);
	int client_socket = accept(server_socket, (sockaddr*)&addr, &addrlen);
	if( client_socket  == -1 )
	{
		return;
	}

    int zero=0;
    ioctl(client_socket, FIONBIO, &zero);
    //printf("Received connection\n");

	imagedb_hdr hdr;
	ret = recvall(client_socket, (char*)&hdr, sizeof(hdr), 0);

	if( ret != sizeof(hdr) )
	{
		fprintf(stderr, "Imagedb Server Error: Unable to read image header.  Read %d, expected %d\n",
			ret, (int)sizeof(hdr));
		goto END;
	}

	//Now that we know how long the request is supposed to be, allocated a buffer
	datasize = hdr.cols*hdr.rows*hdr.format*imagedb_bytes_per_pixel[hdr.type];
	data = new unsigned char[datasize];
	memset(data, 0, datasize);
	ret = recvall(client_socket, (char*)data, datasize, 0);
	if( ret != datasize )
	{
		fprintf(stderr, "Imagedb Server Error: Unable to read image data.  Read %d, expected %d\n",
			ret, datasize);
		goto END;
	}

	//printf("New image: rows=%d cols=%d \n", hdr.rows, hdr.cols);
	//memset(&newimg, 0, sizeof(newimg));
	newimg.height = hdr.rows;
	newimg.width = hdr.cols;
	newimg.format = hdr.format;
	//printf("Newimg format: %d\n", newimg.format);

	//newimg.type = hdr.type;
	//newimg.scale = 1.0;
	//newimg.bias = 0.0;
	newimg.flip = hdr.flip;
	//if( hdr.flip )
		//printf("Flip\n");
	strncpy(newimg.title, hdr.title, sizeof(hdr.title)-1);

	//No matter what datatype it was, we convert it all to float
	newimg.data = (new float[hdr.cols*hdr.rows][4]);
	for(int i=0; i<newimg.height*newimg.width; i++)
	{
		newimg.data[i][0] = newimg.data[i][1] = newimg.data[i][2] = 0.0f;
		newimg.data[i][3] = 1.0f;
		for(int c=0; c<hdr.format; c++)
		{
			if( hdr.type == IMAGEDB_DOUBLE )
				newimg.data[i][c] = (float)(((double*)data)[i*hdr.format+c]);
			else if( hdr.type == IMAGEDB_FLOAT )
				newimg.data[i][c] = (float)(((float*)data)[i*hdr.format+c]);
			else if( hdr.type == IMAGEDB_32BIT )
				newimg.data[i][c] = float(((double)(((unsigned int*)data)[i*hdr.format+c])) / 0xFFFFFFFF);
			else if( hdr.type == IMAGEDB_16BIT )
				newimg.data[i][c] = float(((double)(((unsigned short*)data)[i*hdr.format+c])) / 0xFFFF);
			else if( hdr.type == IMAGEDB_8BIT )
				newimg.data[i][c] = float(((double)(((unsigned char*)data)[i*hdr.format+c])) / 0xFF);
			else
				printf("Error: invalid type\n");
		}
	}
	//newimg.type = IMAGEDB_FLOAT;
	delete [] data;
	data = NULL;

	/*
	//We have to have a special case because apparently it is
	//illegal to pass GL_DOUBLE to the type arg of TexImage
	if( newimg.type == IMAGEDB_DOUBLE )
	{
		float * newdata = new float[hdr.cols*hdr.rows*hdr.format];
		for(int i=0; i<newimg.height*newimg.width*hdr.format; i++)
		{
			newdata[i] = (float)(((double*)data)[i]);
			//printf("%f\n", newdata[i]);
		}
		delete [] data;
		newimg.data = (unsigned char*)newdata;
		newimg.type = IMAGEDB_FLOAT;
	}
	else
	{
		newimg.data = data;
	}
	*/

	//LinearRescale(newimg, newimg.scale, newimg.bias);
	
	if( hdr.duplicate == IMAGEDB_DUP_REPLACE )
	{
		int idx = FindImageWithTitle(newimg.title);
		//newimg.settings.scale = images[idx].settings.scale;
		//newimg.settings.bias = images[idx].settings.bias;
		//newimg.flip = images[idx].flip;
		if( idx != -1 )
		{
			newimg.settings = images[idx].settings;
			DeleteImage(idx);
		}
	}
	//else if( hdr.duplicate & IMAGEDB_DUP_ADD )
	//{
		//int idx = FindImageWithTitle(newimg.title);
		//if( idx != -1 )
		//{
			//if( !(hdr.duplicate & IMAGEDB_DUP_APPEND) )
					//DeleteImage(idx);
		//}
	//}

	images.push_back(newimg);

	//If they have designated a certain frame, find its handle
	if( hdr.frame[0] )
	{
		targetframe = FindFrameByName(hdr.frame);

		//If it could not be found, create a new frame
		if( !targetframe )
		{
			targetframe = CreateFrame();
			targetframe->handle = glutCreateWindow("Image Debugger");
			targetframe->current_index = images.size()-1;	
			targetframe->display_pos_x = 0;	
			targetframe->display_pos_y = 0;	
			targetframe->window_width = frame->window_width;
			targetframe->window_height = frame->window_height;
			strncpy(targetframe->name, hdr.frame, 128);

			int oldwindow = glutGetWindow();
			glutSetWindow( frames.back().handle );
			RegisterCallbacks();	
			glutPostRedisplay();
			glutSetWindow( oldwindow );

		}
		//else
			//printf("Found frame, name=%s\n", targetframe->name);

	}

	//Loop through all the frames and tell them to redisplay
	
	if( jump_to_front )
	{
		//image_index = images.size()-1;
		targetframe->current_index = images.size()-1;
		//image = &images[image_index];
		//image = &images[targetframe->current_index];
	}

END:
	shutdown(client_socket, SHUT_RDWR);
	close(client_socket);
	
	if( data ) delete [] data;

	SetFrame(targetframe);
	//SetWindowTitle(targetframe);
	PostAllRedisplay();

	return ;
}


