g++ -O3 -std=c++11 -c png.cc `libpng-config --cflags` && g++ -o png png.o `libpng-config --ldflags` && ./png
