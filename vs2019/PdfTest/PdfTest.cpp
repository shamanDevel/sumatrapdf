// PdfTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <chrono>
#include <stdio.h>

#include "..\SumatraPdfAPI/SumatraPublicPdfApi.h"

constexpr float INCH_TO_CM = 2.54;

const int BYTES_PER_PIXEL = 3; /// red, green, & blue
const int FILE_HEADER_SIZE = 14;
const int INFO_HEADER_SIZE = 40;

void generateBitmapImage(unsigned char* image, int height, int width, const char* imageFileName);
unsigned char* createBitmapFileHeader(int height, int stride);
unsigned char* createBitmapInfoHeader(int height, int width);

int main()
{
    const std::wstring filename = L"../Sheety/Sadness and Sorrow.pdf";
    const std::string outputFilename = "../Sheety/Sadness and Sorrow.bmp";
    std::wcout << "Open file " << filename << std::endl;

    bool isPdf = SumatraPdfApi::PdfEngine::IsPdfFile(filename);
    std::wcout << "File is a PDF file: " << isPdf << std::endl;

    std::shared_ptr<SumatraPdfApi::PdfEngine> engine =
        SumatraPdfApi::PdfEngine::CreateFromFile(filename);
    std::wcout << "Engine created!" << std::endl;
    std::wcout << "Number of pages: " << engine->pageCount() << std::endl;
    std::wcout << "File DPI: " << engine->fileDPI() << std::endl;
    const auto mediaBoxDots = engine->pageMediabox(0);
    const auto mediaBoxCentimeters =
        mediaBoxDots.scale(INCH_TO_CM / engine->fileDPI());
    std::wcout << "Page 0 media box: " << mediaBoxDots << " dots, " << mediaBoxCentimeters << " cm" << std::endl;
    std::wcout << "Page 0 content box: " << engine->pageContentBox(0) << " dots" << std::endl;

    //render example page
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    SumatraPdfApi::RenderPageArgs renderArgs{0, 2.0, 0, nullptr};
    SumatraPdfApi::Bitmap_ptr bitmap = engine->renderPage(renderArgs);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::wcout << "Page 0 rendered, width=" << bitmap->width << ", height=" << bitmap->height << " in "
               << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms" << std::endl;

    //save to bmp
    //but first convert to 3-channel image
    unsigned char* image = new unsigned char[bitmap->width * bitmap->height * 3];
    for (int y = 0; y < bitmap->height; ++y)
        for (int x = 0; x < bitmap->width; ++x)
            for (int c = 0; c < 3; ++c)
                image[c + 3 * (x + bitmap->width * y)] = bitmap->data[bitmap->idx(x, y, c)];
    //now save
    generateBitmapImage(image, bitmap->height, bitmap->width, outputFilename.c_str());
    delete[] image;
    std::wcout << "Page 0 saved to " << outputFilename.c_str() << std::endl;
}

// Source: https://stackoverflow.com/a/47785639

void generateBitmapImage(unsigned char* image, int height, int width, const char* imageFileName) {
    int widthInBytes = width * BYTES_PER_PIXEL;

    unsigned char padding[3] = {0, 0, 0};
    int paddingSize = (4 - (widthInBytes) % 4) % 4;

    int stride = (widthInBytes) + paddingSize;

    FILE* imageFile = fopen(imageFileName, "wb");

    unsigned char* fileHeader = createBitmapFileHeader(height, stride);
    fwrite(fileHeader, 1, FILE_HEADER_SIZE, imageFile);

    unsigned char* infoHeader = createBitmapInfoHeader(height, width);
    fwrite(infoHeader, 1, INFO_HEADER_SIZE, imageFile);

    int i;
    for (i = 0; i < height; i++) {
        fwrite(image + (i * widthInBytes), BYTES_PER_PIXEL, width, imageFile);
        fwrite(padding, 1, paddingSize, imageFile);
    }

    fclose(imageFile);
}

unsigned char* createBitmapFileHeader(int height, int stride) {
    int fileSize = FILE_HEADER_SIZE + INFO_HEADER_SIZE + (stride * height);

    static unsigned char fileHeader[] = {
        0, 0,       /// signature
        0, 0, 0, 0, /// image file size in bytes
        0, 0, 0, 0, /// reserved
        0, 0, 0, 0, /// start of pixel array
    };

    fileHeader[0] = (unsigned char)('B');
    fileHeader[1] = (unsigned char)('M');
    fileHeader[2] = (unsigned char)(fileSize);
    fileHeader[3] = (unsigned char)(fileSize >> 8);
    fileHeader[4] = (unsigned char)(fileSize >> 16);
    fileHeader[5] = (unsigned char)(fileSize >> 24);
    fileHeader[10] = (unsigned char)(FILE_HEADER_SIZE + INFO_HEADER_SIZE);

    return fileHeader;
}

unsigned char* createBitmapInfoHeader(int height, int width) {
    static unsigned char infoHeader[] = {
        0, 0, 0, 0, /// header size
        0, 0, 0, 0, /// image width
        0, 0, 0, 0, /// image height
        0, 0,       /// number of color planes
        0, 0,       /// bits per pixel
        0, 0, 0, 0, /// compression
        0, 0, 0, 0, /// image size
        0, 0, 0, 0, /// horizontal resolution
        0, 0, 0, 0, /// vertical resolution
        0, 0, 0, 0, /// colors in color table
        0, 0, 0, 0, /// important color count
    };

    infoHeader[0] = (unsigned char)(INFO_HEADER_SIZE);
    infoHeader[4] = (unsigned char)(width);
    infoHeader[5] = (unsigned char)(width >> 8);
    infoHeader[6] = (unsigned char)(width >> 16);
    infoHeader[7] = (unsigned char)(width >> 24);
    infoHeader[8] = (unsigned char)(height);
    infoHeader[9] = (unsigned char)(height >> 8);
    infoHeader[10] = (unsigned char)(height >> 16);
    infoHeader[11] = (unsigned char)(height >> 24);
    infoHeader[12] = (unsigned char)(1);
    infoHeader[14] = (unsigned char)(BYTES_PER_PIXEL * 8);

    return infoHeader;
}
