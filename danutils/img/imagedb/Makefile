#PLATFORM=Linux
#PLATFORM=Cygwin
#PLATFORM=MacOsX
default:
	make -fMakefile.$(PLATFORM)

clean:
	make -fMakefile.$(PLATFORM) clean

zip:
	zip imagedb.zip *.h *.cc Makefile* metalrect.tga \
		windows/glut32.lib windows/imagedb.sln \
		windows/imagedb.vcproj windows/OpenGL32.Lib \
		windows/GL/glut.h windows/imagedb.exe

