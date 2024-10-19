#pragma once
#include "resource.h"
#include <iostream>
#include <emmintrin.h> // SSE2 instructions library header
#include <cmath>
#include <xmmintrin.h>
#include <windows.h>
#include <tchar.h>
// external JALib.dll library function definition prototype





#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern "C" void  GaussianBlurHorizontalASM(unsigned char* src, float* kernel, int width, int height, int radius);
extern "C" float*  calcuGaussKernel1DASM(int radius);


