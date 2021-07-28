# realtime-raytracing-demo

A hardware-accelerated (via OpenGL) realtime raytracer written in C/GLSL.
Move with W/A/S/D and the mouse.
Supports spheres as a shape primitive and three materials: An ideal lambertian diffuse, metal (with adjustable diffuse), and dielectric (glass-like).

## building

GLFW requires X11 development headers to build. Should build on other platforms too, but I haven't tested it.

```
mkdir build
cd build
cmake .. && make && ./realtime-raytracing-demo
```