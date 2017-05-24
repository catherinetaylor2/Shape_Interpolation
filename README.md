# Shape Interpolation 

Coursework for Computer Animation and Games 2 module. Carries out linear and as-rigid-as-possible shape interpolation in 2D and 3D.

![m_d](https://cloud.githubusercontent.com/assets/25514442/26422887/77ef4b06-40c3-11e7-8bd6-2b5f3a90b375.PNG)

### Dependencies:

To run successfully, this code must be linked to the following 5 libraries.

* OpenGl
* GLEW 
* GLFW3
* GLM
* Eigen

### To Compile CL:
LERP:

     cl  /EHsc  linear_interp.cpp read_obj.cpp shader.cpp  /link /NODEFAULTLIB:"LIBCMT"  "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\lib\glfw3.lib" "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\lib\glew32.lib" "C:\Program Files (x86)\Windows Kits\8.1\Lib\winv6.3\um\x86\OpenGL32.Lib"  "user32.lib" "gdi32.lib"  "shell32.lib" "msvcrt.lib" 

Rigid-as-poss 2D:

     cl /O2 /EHsc  rigid_interp.cpp read_obj.cpp shader.cpp  /link /NODEFAULTLIB:"LIBCMT"  "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\lib\glfw3.lib" "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\lib\glew32.lib" "C:\Program Files (x86)\Windows Kits\8.1\Lib\winv6.3\um\x86\OpenGL32.Lib"  "user32.lib" "gdi32.lib"  "shell32.lib" "msvcrt.lib" 
     
Rigid-as-poss 3D:

    cl /O2  /EHsc  rigid_interp_3D.cpp read_obj.cpp shader.cpp /openmp /link /NODEFAULTLIB:"LIBCMT"  "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\lib\glfw3.lib" "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\lib\glew32.lib" "C:\Program Files (x86)\Windows Kits\8.1\Lib\winv6.3\um\x86\OpenGL32.Lib"  "user32.lib" "gdi32.lib"  "shell32.lib" "msvcrt.lib" 

### To Run:
* Enter key to start or reset.
* S - slow down.
* F - speed up.




### References:
* 2D Rigid-as-possible: As-rigid-as-possible shape interpolation - Alexa et al (2000).
* 3D Rigid-as-possible: As-rigid-as-possible surface morphing - Liu et al (2011)
