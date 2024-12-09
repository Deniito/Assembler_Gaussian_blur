#pragma once
#include "resource.h"
#include <iostream>
#include <emmintrin.h> // SSE2 instructions library header
#include <cmath>
#include <xmmintrin.h>
#include <windows.h>
#include <tchar.h>
// external JALib.dll library function definition prototype






extern "C" void  GaussianBlurHorizontalASM(unsigned char* src, float* kernel, int width, int height, int radius);
extern "C" void  GaussianBlurVerticalASM(unsigned char* src, float* kernel, int width, int height, int radius);
extern "C" float*  calcuGaussKernel1DASM(int radius);

extern "C" {
    __declspec(dllexport) void GaussianBlurHorizontal(unsigned char* src, float* kernel, int width, int height, int radius);
    __declspec(dllexport) void GaussianBlurVertical(unsigned char* src, float* kernel, int width, int height, int radius);
    __declspec(dllexport) float* calcuGaussKernel1D(int radius);

}

