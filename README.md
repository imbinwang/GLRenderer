# GLRenderer #

### Introduction ###

This code repository is for rendering OBJ mesh to OpenCV window with background image supporting. 

### Requirements ###

1. OS Platform: Windows, Ubuntu
2. [OpenCV](http://opencv.org/): The version should be below 3.0, because the 2.X APIs are different from 3.X somehow. 
3. [OpenGL](https://www.opengl.org/) or [FreeGLUT](http://freeglut.sourceforge.net/): The best choise is FreeGLUT, cause it supports indepent window event manipulation.

### Installation ###

1. Clone the GLRenderer repository

	```
	git clone https://github.com/imbinwang/GLRenderer.git
	```

2. We'll call the directory that you cloned GLRenderer into GLRenderer_ROOT. You should print the marker file in `GLRenderer_ROOT/data` and calibrate your camera in advance. 

3. Modify the maker size and camera paramters in the file `GLRenderer_ROOT/main.cpp`

4. Run.


