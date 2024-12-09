#include "pch.h"

#include "framework.h"

#include "framework.h"
#include <memory>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


extern "C" {
    __declspec(dllexport) float* calcuGaussKernel1D(int radius)
    {
        int size = 2 * radius + 1;
        float* kernel = new float[size];

        float stddiv = radius / 2.0f; // Standard deviation
        float sum = 0.0f;

        // Calculate the Gaussian kernel
        for (int i = 0; i < size; i++)
        {
            int x = i - radius; // Calculate the distance from the center
            kernel[i] = exp(-(x * x) / (2 * stddiv * stddiv)) / sqrt(2 * M_PI * stddiv * stddiv); // 1d gaussian formula
            sum += kernel[i];
        }
        //kernel normalization
        for (int i = 0; i < size; i++)
        {
            kernel[i] /= sum;
        }
        return kernel;
    }

    __declspec(dllexport) void GaussianBlurHorizontal(unsigned char* src, float* kernel, int width, int height, int radius) {
        int kernel_size = 2 * radius + 1;
        unsigned char* blr = new unsigned char[width * height * 3]; // For BGR images

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float sum[3] = { 0.0f, 0.0f, 0.0f }; // To hold sums for B, G, R channels

                // Apply kernel to the pixel at (x, y)
                for (int k = -radius; k <= radius; ++k) {
                    int neighbor_x = x + k;

                    // Handle boundaries: clamp to valid range
                    if (neighbor_x < 0) neighbor_x = 0;  // Clamp to 0
                    if (neighbor_x >= width) neighbor_x = width - 1;  // Clamp to width-1

                    // Apply the kernel for each channel (B, G, R)
                    int srcIndex = (y * width + neighbor_x) * 3; // 3 bytes per pixel (BGR)

                    sum[0] += src[srcIndex] * kernel[radius + k];     // Blue channel
                    sum[1] += src[srcIndex + 1] * kernel[radius + k]; // Green channel
                    sum[2] += src[srcIndex + 2] * kernel[radius + k]; // Red channel
                }

                // Store the result in the blurred image
                int destIndex = (y * width + x) * 3; // 3 bytes per pixel (BGR)
                blr[destIndex] = static_cast<unsigned char>(sum[0]);         // Blue
                blr[destIndex + 1] = static_cast<unsigned char>(sum[1]);     // Green
                blr[destIndex + 2] = static_cast<unsigned char>(sum[2]);     // Red
            }
        }

        std::memcpy(src, blr, width * height * 3); // Copy blurred data back to src
        delete[] blr; // Clean up memory
    }

    __declspec(dllexport) void GaussianBlurVertical(unsigned char* src, float* kernel, int width, int height, int radius) {
        int kernel_size = 2 * radius + 1;
        unsigned char* blr = new unsigned char[width * height * 3]; // For BGR images

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float sum[3] = { 0.0f, 0.0f, 0.0f }; // To hold sums for B, G, R channels

                // Apply kernel to the pixel at (x, y)
                for (int k = -radius; k <= radius; ++k) {
                    int neighbor_y = y + k;

                    // Handle boundaries: clamp to valid range
                    if (neighbor_y < 0) neighbor_y = 0;  // Clamp to 0
                    if (neighbor_y >= height) neighbor_y = height - 1;  // Clamp to height-1

                    // Apply the kernel for each channel (B, G, R)
                    int srcIndex = (neighbor_y * width + x) * 3; // 3 bytes per pixel (BGR)
                    sum[0] += src[srcIndex] * kernel[radius + k];     // Blue channel
                    sum[1] += src[srcIndex + 1] * kernel[radius + k]; // Green channel
                    sum[2] += src[srcIndex + 2] * kernel[radius + k]; // Red channel
                }

                // Store the result in the blurred image
                int destIndex = (y * width + x) * 3; // 3 bytes per pixel (BGR)
                blr[destIndex] = static_cast<unsigned char>(sum[0]);         // Blue
                blr[destIndex + 1] = static_cast<unsigned char>(sum[1]);     // Green
                blr[destIndex + 2] = static_cast<unsigned char>(sum[2]);     // Red
            }
        }

        std::memcpy(src, blr, width * height * 3); // Copy blurred data back to src
        delete[] blr; // Clean up memory
    }
}