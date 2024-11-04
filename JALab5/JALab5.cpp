// JALab1.cpp : Defines the entry point for the application.
#include "framework.h"
#include "JALab5.h"
#include <string>
#include <sstream>

#include "framework.h"
#include "JALab5.h"
#include "Resource.h"

#include <math.h>
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <iostream>
#include <cstring>
#include <chrono>
#include <fstream>
#include <vector>
#include <filesystem>
#include <commdlg.h> 
#include <map>

#pragma comment(lib, "gdiplus.lib")

/*
--------------------------------------------------

  Assembler Programming Languages Gaussian Blur;

  Author:	Daniel Pasierb;
  INF SSI GLI;
  SEK 10;

--------------------------------------------------
*/
using namespace Gdiplus;



// Structure to hold image data and related properties
struct ImageData {
    Gdiplus::Bitmap* image = nullptr;
    unsigned char* pixelData = nullptr;
    unsigned char* blurPixelData = nullptr;
    int imageWidth = 0;
    int imageHeight = 0;
    bool useASM = false; // Boolean variable to track the button state
    unsigned int rad = 1;
};

void reloadImgData(HWND hwnd, ImageData* imgData);
void RenderImage(HWND hwnd, ImageData* imgData);
double ApplyBlur(HWND hwnd, ImageData* imgData, int rad);
void RedrawImage(HWND hwnd);
void TestBlurOnImages(HWND hwnd, ImageData* imgData);
void SaveElapsedTimeToCSV(const std::string& imageName, int radius, double elapsedTime, bool useASM);
void SetMenuText(HWND hwnd, const wchar_t* newText);
std::string wstring_to_string(const std::wstring& wstr);
void GaussBlur(unsigned char* pixel_data, int pixel_data_size, int width, int height, int radius);
void GaussBlurASM(unsigned char* pixel_data, int pixel_data_size, int width, int height, int radius);
std::vector<std::wstring> LoadImagesFromFolder(const std::wstring& folderPath);
void TestBlurOnImage(HWND hwnd, const std::wstring& imagePath, const int* radii, size_t numRadii, ImageData* imgData);
std::string wstring_to_string(const std::wstring& wstr);
void LoadNewImage(HWND hwnd, ImageData* imgData, const std::wstring& imagePath);


// Function to apply Gaussian blur
float* calcuGaussKernel1D(int radius)
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

void GaussianBlurHorizontal(unsigned char* src, float* kernel, int width, int height, int radius) {
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

// Vertical Gaussian Blur Function
void GaussianBlurVertical(unsigned char* src, float* kernel, int width, int height, int radius) {
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
void SavePixelDataToTxt(const unsigned char* pixel_data, int width, int height, const char* filename)
{
	//Function meant purely for debugging purposes
    std::ofstream file(filename);

    if (!file.is_open())
    {
        std::cerr << "Error: Could not open file for writing\n";
        return;
    }

    // Save pixel data into the text file
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            // Assuming BGR format (Blue, Green, Red), 3 channels per pixel
            int idx = (y * width + x) * 3;
            file << (int)pixel_data[idx] << " "      // Blue channel
                << (int)pixel_data[idx + 1] << " "  // Green channel
                << (int)pixel_data[idx + 2] << " "; // Red channel
        }
        file << "\n";
    }

    file.close();
    std::cout << "Pixel data saved to " << filename << std::endl;
}
void GaussBlur(unsigned char* pixel_data, int pixel_data_size, int width, int height, int radius) 
{
    //SavePixelDataToTxt(pixel_data, width, height, "pixel_data_before_blur.txt");
    float* kernel = calcuGaussKernel1D(radius);
    GaussianBlurHorizontal(pixel_data, kernel, width, height, radius);
    GaussianBlurVertical(pixel_data, kernel, width, height, radius);
    //SavePixelDataToTxt(pixel_data, width, height, "pixel_data_after_blur.txt");
    delete[] kernel;
}
void GaussBlurASM(unsigned char* pixel_data, int pixel_data_size, int width, int height, int radius)
{
    //SavePixelDataToTxt(pixel_data, width, height, "pixel_data_before_blurASM.txt");
    float* kernel = calcuGaussKernel1DASM(radius);
    GaussianBlurHorizontalASM(pixel_data, kernel, width, height, radius);
    GaussianBlurVerticalASM(pixel_data, kernel, width, height, radius);

    //SavePixelDataToTxt(pixel_data, width, height, "pixel_data_after_blurASM.txt");
    // delete[] kernel;
}

bool IsFileEmpty(const std::string& fileName) {
    std::ifstream file(fileName, std::ios::ate); // Open file at the end
    return file.tellg() == 0; // If at start, file is empty
}

void SaveElapsedTimeToCSV(const std::string& imageName, int radius, double elapsedTime, bool useASM) {
    const std::string fileName = "blur_elapsed_time_test.csv";
    std::ifstream inputFile(fileName);
    std::stringstream buffer;

    // Maps to hold data, with each entry as {imageName -> [kernel1_time, kernel5_time, kernel10_time]}
    std::map<std::string, std::vector<std::string>> asmData;
    std::map<std::string, std::vector<std::string>> cppData;

    // If file exists, load data into the maps
    if (inputFile.is_open()) {
        buffer << inputFile.rdbuf();
        inputFile.close();
    }

    std::string line;
    bool asmSection = false, cppSection = false;

    std::istringstream fileContent(buffer.str());

    while (std::getline(fileContent, line)) {
        std::stringstream lineStream(line);
        std::string cell;
        std::vector<std::string> row;

        while (std::getline(lineStream, cell, '\t')) {
            row.push_back(cell);
        }

        // Switch sections based on headers
        if (row.size() > 0) {
            if (row[0] == "ASM") {
                asmSection = true;
                cppSection = false;
                continue;
            }
            else if (row[0] == "C++") {
                cppSection = true;
                asmSection = false;
                continue;
            }
        }

        // Skip headers inside sections
        if ((asmSection || cppSection) && row.size() > 1 && row[0] == "file_name") continue;

        // Extract rows within each section
        if (row.size() == 4 && !row[0].empty()) {
            std::string name = row[0];
            if (asmSection) {
                asmData[name] = { row[1], row[2], row[3] };
            }
            else if (cppSection) {
                cppData[name] = { row[1], row[2], row[3] };
            }
        }
    }

    // Update the relevant section with new data
    std::vector<std::string>& times = useASM ? asmData[imageName] : cppData[imageName];
    if (times.empty()) times = { "0", "0", "0" };  // Initialize if not present

    // Update the correct kernel time slot (assuming radii of 1, 5, and 10)
    if (radius == 1) times[0] = std::to_string(elapsedTime);
    else if (radius == 5) times[1] = std::to_string(elapsedTime);
    else if (radius == 10) times[2] = std::to_string(elapsedTime);

    // Write back to the file, starting with headers
    std::ofstream outputFile(fileName);
    outputFile << "ASM\nfile_name\t1\t5\t10\n";

    for (const auto& [name, timeValues] : asmData) {
        outputFile << name;
        for (const auto& time : timeValues) {
            outputFile << "\t" << (time.empty() ? "0" : time);
        }
        outputFile << "\n";
    }

    outputFile << "C++\nfile_name\t1\t5\t10\n";
    for (const auto& [name, timeValues] : cppData) {
        outputFile << name;
        for (const auto& time : timeValues) {
            outputFile << "\t" << (time.empty() ? "0" : time);
        }
        outputFile << "\n";
    }

    outputFile.close();
}

std::string wstring_to_string(const std::wstring& wstr) {
    // Calculate the size needed for the narrow string
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), &str[0], size_needed, NULL, NULL);
    return str;
}

// Function to test multiple images with different blur radii
void TestBlurOnImages(HWND hwnd, ImageData* imgData) {
    // Specify the directory containing test images
    std::wstring folderPath = L"testImgs\\";
    std::vector<std::wstring> imageFiles = LoadImagesFromFolder(folderPath);

    // Define the blur radii to test
    int rad4test[] = { 1, 5, 10 };

    // Test each image with the defined radii
    for (const auto& imagePath : imageFiles)
    {
        TestBlurOnImage(hwnd, imagePath, rad4test, sizeof(rad4test) / sizeof(rad4test[0]), imgData);
    }

    // Notify completion
    MessageBox(hwnd, L"Testing complete. Results saved to blur_elapsed_time_test.csv.", L"Test Complete", MB_OK);
}

// Function to load all image files from the specified folder
std::vector<std::wstring> LoadImagesFromFolder(const std::wstring& folderPath) {
    std::vector<std::wstring> imageFiles;

    for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            imageFiles.push_back(entry.path().wstring());
        }
    }

    return imageFiles;
}

// Function to test blur on a single image
void TestBlurOnImage(HWND hwnd, const std::wstring& imagePath, const int* radii, size_t numRadii, ImageData* imgData)
{
    LoadNewImage(hwnd, imgData, imagePath);

    // Test the blur with each radius
    for (size_t i = 0; i < numRadii; ++i) {
        int radius = radii[i];

        // Start time measurement

        double elapsedTime = ApplyBlur(hwnd, imgData, radius);

        // Extract the filename from imagePath
        size_t lastSlashPos = imagePath.find_last_of(L"\\");
        std::wstring filenameW = imagePath.substr(lastSlashPos + 1); // Get the filename as wstring
        std::string filename = wstring_to_string(filenameW); // Convert to std::string

        // Save the elapsed time to the CSV file
        SaveElapsedTimeToCSV(filename, radius, elapsedTime, imgData->useASM);
        RedrawImage(hwnd);
        UpdateWindow(hwnd);
        Sleep(1000);
    }
}

// Function to apply blur effect
void RedrawImage(HWND hwnd)
{
    // Invalidate the window to trigger a repaint
    InvalidateRect(hwnd, NULL, TRUE);
}

// Function to apply blur effect
double ApplyBlur(HWND hwnd, ImageData* imgData, int rad) {
    // Start time measurement
    RedrawImage(hwnd);
    LARGE_INTEGER frequency, start, end;

    if (imgData->useASM == false)
    {
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);
        GaussBlur(imgData->blurPixelData, imgData->imageWidth * imgData->imageHeight * 3, imgData->imageWidth, imgData->imageHeight, rad);
        QueryPerformanceCounter(&end);
    }
    else
    {
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);
        GaussBlurASM(imgData->blurPixelData, imgData->imageWidth * imgData->imageHeight * 3, imgData->imageWidth, imgData->imageHeight, rad);
        QueryPerformanceCounter(&end);

    }

    // End time measurement

    // Calculate elapsed time in milliseconds
    double elapsedTime = static_cast<double>(end.QuadPart - start.QuadPart) * 1000.0 / frequency.QuadPart;

    // Create a message string to display the elapsed time
    RedrawImage(hwnd);

    return elapsedTime;
}

// Function to initialize ImageData structure
void LoadNewImage(HWND hwnd, ImageData* imgData, const std::wstring& imagePath = L"testImgs\\¿acy_144.jpg")
{
    if (!imgData)
        return; // Exit initialization

    if (imgData->image) {
        delete imgData->image;
        imgData->image = nullptr;
    }
    imgData->image = new Gdiplus::Bitmap(imagePath.c_str());
    if (!imgData->image || imgData->image->GetLastStatus() != Ok)
    {
        MessageBox(hwnd, L"Failed to load image!", L"Error", MB_OK | MB_ICONERROR);
        delete imgData; // Clean up if failed
        SetWindowLongPtr(hwnd, GWLP_USERDATA, NULL); // Clear the user data
        return; // Exit initialization
    }
    else {
        // Get image dimensions
        reloadImgData(hwnd, imgData);

    }
}

void reloadImgData(HWND hwnd, ImageData* imgData) {
    imgData->imageWidth = imgData->image->GetWidth();
    imgData->imageHeight = imgData->image->GetHeight();

    if (imgData->pixelData) delete[] imgData->pixelData;
    if (imgData->blurPixelData) delete[] imgData->blurPixelData;

    // Allocate pixel data buffer
    imgData->pixelData = new unsigned char[imgData->imageWidth * imgData->imageHeight * 3];
    imgData->blurPixelData = new unsigned char[imgData->imageWidth * imgData->imageHeight * 3];

    // Create BitmapData object to access pixel data
    BitmapData bitmapData;
    Rect rect(0, 0, imgData->imageWidth, imgData->imageHeight);
    imgData->image->LockBits(&rect, ImageLockModeRead, PixelFormat24bppRGB, &bitmapData);

    // Get a pointer to the pixel data
    BYTE* originalPixels = (BYTE*)bitmapData.Scan0;

    // Copy the original pixel data to the pixelData array
    for (int y = 0; y < imgData->imageHeight; ++y) {
        for (int x = 0; x < imgData->imageWidth; ++x) {
            int srcIndex = (y * bitmapData.Stride) + (x * 3); // 3 bytes per pixel (BGR)
            int destIndex = (y * imgData->imageWidth + x) * 3; // 3 bytes per pixel (RGB)

            // Keep the channel order intact (BGR -> BGR)
            imgData->pixelData[destIndex] = originalPixels[srcIndex];         // Blue
            imgData->pixelData[destIndex + 1] = originalPixels[srcIndex + 1]; // Green
            imgData->pixelData[destIndex + 2] = originalPixels[srcIndex + 2]; // Red
        }
    }
    std::memcpy(imgData->blurPixelData, imgData->pixelData, imgData->imageWidth * imgData->imageHeight * 3);

    // Unlock the original image data
    imgData->image->UnlockBits(&bitmapData);
    RenderImage(hwnd, imgData);
}

void RenderImage(HWND hwnd, ImageData* imgData) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    // Get the size of the window's client area
    RECT rect;
    GetClientRect(hwnd, &rect);
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;

    // Create a GDI+ Graphics object from the HDC
    Graphics graphics(hdc);

    // Create a GDI+ Bitmap from the pixel data (after possible blur)
    Bitmap bitmapNorm(imgData->imageWidth, imgData->imageHeight, imgData->imageWidth * 3, PixelFormat24bppRGB, imgData->pixelData);
    Bitmap bitmapblr(imgData->imageWidth, imgData->imageHeight, imgData->imageWidth * 3, PixelFormat24bppRGB, imgData->blurPixelData);

    // Define the size of the smaller version of the image
    int smallerWidth = windowWidth / 2;  // Make the image half the window width
    int smallerHeight = (smallerWidth * imgData->imageHeight) / imgData->imageWidth; // Maintain aspect ratio

    // Draw the images
    graphics.DrawImage(&bitmapNorm, 0, 0, smallerWidth, smallerHeight);
    graphics.DrawImage(&bitmapblr, smallerWidth, 0, smallerWidth, smallerHeight);

    EndPaint(hwnd, &ps);
    RedrawImage(hwnd);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Retrieve ImageData pointer from window's user data
    ImageData* imgData = reinterpret_cast<ImageData*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    // Declare the imageFiles variable at the beginning of the function
    static std::vector<std::wstring> imageFiles;

    switch (uMsg) {
    case WM_PAINT: {
        RenderImage(hwnd, imgData); // Render the image
    } break;

    case WM_DESTROY:
        if (imgData) {
            if (imgData->image) {
                delete imgData->image;  // Clean up the image object
            }
            delete[] imgData->pixelData; // Clean up the pixel data
            delete imgData; // Clean up the ImageData structure
        }
        PostQuitMessage(0);
        return 0;

    case WM_CREATE: {
        // Initialize ImageData
        imgData = new ImageData();
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(imgData));
        LoadNewImage(hwnd, imgData); // Call the initialization function

        // Load images from folder and create menu items
        imageFiles = LoadImagesFromFolder(L"testImgs\\"); // Load images here

        // Create a menu
        HMENU hMenu = CreateMenu();
        HMENU hSubMenuActions = CreatePopupMenu();
        HMENU hSubMenuFuncType = CreatePopupMenu(); // Submenu for Function Type
        HMENU hSubMenuImages = CreatePopupMenu();
        HMENU hSubMenuBlurRad = CreatePopupMenu(); // Submenu for Blur Radius

        // Populate the "Actions" menu
        AppendMenu(hSubMenuActions, MF_STRING, 1, L"Apply");
        AppendMenu(hSubMenuActions, MF_STRING, 2, L"Run Tests");

        // Populate the "Function Type" menu
        AppendMenu(hSubMenuFuncType, MF_STRING, 3, L"Blur (CPP)");
        AppendMenu(hSubMenuFuncType, MF_STRING, 4, L"Blur (ASM)");

        // Populate the "Change Image" menu
        for (const auto& filePath : imageFiles) {
            std::wstring fileName = std::filesystem::path(filePath).filename().wstring();
            AppendMenu(hSubMenuImages, MF_STRING, 100 + &filePath - &imageFiles[0], fileName.c_str());
        }

        // Populate the "Blur Radius" menu with options 1, 2, 3, 4, 5, and 10
        AppendMenu(hSubMenuBlurRad, MF_STRING, 200, L"1");
        AppendMenu(hSubMenuBlurRad, MF_STRING, 201, L"2");
        AppendMenu(hSubMenuBlurRad, MF_STRING, 202, L"3");
        AppendMenu(hSubMenuBlurRad, MF_STRING, 203, L"4");
        AppendMenu(hSubMenuBlurRad, MF_STRING, 204, L"5");
        AppendMenu(hSubMenuBlurRad, MF_STRING, 205, L"10");

        // Add submenus to the main menu
        AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenuActions, L"Actions");
        AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenuFuncType, L"Blur (CPP)");
        AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenuImages, L"Change Image");
        AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenuBlurRad, L"Blur Radius");
        SetMenu(hwnd, hMenu);

    } break;

    case WM_COMMAND: {
        if (LOWORD(wParam) == 1) {  // Apply Action
            double elapsedTime = ApplyBlur(hwnd, imgData, imgData->rad);  // Use the rad value for blurring
            std::wstring message = L"Blurring took " + std::to_wstring(elapsedTime) + L" milliseconds.";
            MessageBox(hwnd, message.c_str(), L"Blurring Time", MB_OK | MB_ICONINFORMATION);
        }
        else if (LOWORD(wParam) == 2) { // Run Tests Action
            TestBlurOnImages(hwnd, imgData);
        }
        else if (LOWORD(wParam) == 3) { // Use CPP
            imgData->useASM = false; // Set to use C++
            SetMenuText(hwnd, L"Blur (CPP)"); // Optional: Change menu text if needed
        }
        else if (LOWORD(wParam) == 4) { // Use ASM
            imgData->useASM = true; // Set to use ASM
            SetMenuText(hwnd, L"Blur (ASM)"); // Optional: Change menu text if needed
        }
        else if (LOWORD(wParam) >= 100 && LOWORD(wParam) < 100 + imageFiles.size()) { // Change Image Menu Item
            size_t index = LOWORD(wParam) - 100; // Get index of selected image
            if (index < imageFiles.size()) {
                imgData->image = new Gdiplus::Bitmap(imageFiles[index].c_str());
                if (imgData->image->GetLastStatus() != Ok) {
                    MessageBox(hwnd, L"Failed to load selected image!", L"Error", MB_OK | MB_ICONERROR);
                    delete imgData->image;
                    imgData->image = nullptr;
                }
                else {
                    reloadImgData(hwnd, imgData);
                }
            }
        }
        else if (LOWORD(wParam) >= 200 && LOWORD(wParam) <= 205) { // Blur Radius Menu Item
            // Get blur radius from menu selection
            switch (LOWORD(wParam)) {
            case 200: imgData->rad = 1; break;  // Set rad to 1
            case 201: imgData->rad = 2; break;  // Set rad to 2
            case 202: imgData->rad = 3; break;  // Set rad to 3
            case 203: imgData->rad = 4; break;  // Set rad to 4
            case 204: imgData->rad = 5; break;  // Set rad to 5
            case 205: imgData->rad = 10; break; // Set rad to 10
            }
        }
    } break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void SetMenuText(HWND hwnd, const wchar_t* newText) {
    // Optional function to update the menu text if needed
    HMENU hMenu = GetMenu(hwnd);
    ModifyMenu(hMenu, 1, MF_BYPOSITION | MF_STRING, 1, newText);
    DrawMenuBar(hwnd);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    // Register the window class
    const wchar_t CLASS_NAME[] = L"ImageWindowClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    // Create the window
    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Assembler Programming Languages Gaussian Blur ", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1920, 540,
        nullptr, nullptr, hInstance, nullptr
    );

    if (hwnd == nullptr) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Run the message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Shutdown GDI+
    GdiplusShutdown(gdiplusToken);
    return 0;
}
