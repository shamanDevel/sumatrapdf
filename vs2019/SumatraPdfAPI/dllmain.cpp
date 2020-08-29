// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#define SUMATRA_PUBLIC_PDF_API_EXPORTS
#include "SumatraPublicPdfApi.h"

/*
 * AdditionalIncludeDirectories: ..\..\src;..\..\mupdf\include
 */

//copied from EnginePdf.cpp and then removed all unneeded includes

#include <utils/BaseUtil.h>
//#include <utils/Archive.h>
#include <utils/ScopedWin.h>
//#include <utils/FileUtil.h>
//#include <utils/HtmlParserLookup.h>
//#include <utils/HtmlPullParser.h>
//#include <utils/TrivialHtmlParser.h>
#include <utils/WinUtil.h>
//#include <utils/ZipUtil.h>
//#include <utils/Log.h>
//#include <utils/LogDbg.h>
//#include <AppColors.h>
#include <wingui/TreeModel.h>
#include <EngineBase.h>
#include <EnginePdf.h>

extern "C" {
#include <mupdf/fitz/geometry.h>
}

// copied from mupdf/source/fitz/geometry.c

const fz_matrix fz_identity = {1, 0, 0, 1, 0, 0};

const fz_rect fz_infinite_rect = {1, 1, -1, -1};
const fz_rect fz_empty_rect = {0, 0, 0, 0};
const fz_rect fz_unit_rect = {0, 0, 1, 1};

const fz_irect fz_infinite_irect = {1, 1, -1, -1};
const fz_irect fz_empty_irect = {0, 0, 0, 0};
const fz_irect fz_unit_bbox = {0, 0, 1, 1};

extern "C" {
#include <mupdf/fitz/color.h>
}
// copied from colorspace.c
const fz_color_params fz_default_color_params = {FZ_RI_RELATIVE_COLORIMETRIC, 1, 0, 0};



BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

// now implement the public API
namespace SumatraPdfApi {

PdfEngine::PdfEngine(void* impl) : impl_(impl) {}

PdfEngine::~PdfEngine() {
    EngineBase* engine = reinterpret_cast<EngineBase*>(impl_);
    delete engine;
}

bool PdfEngine::IsPdfFile(const std::wstring& filename) {
    const WCHAR* str = filename.c_str();
    return IsEnginePdfSupportedFile(str);
}

std::shared_ptr<PdfEngine> PdfEngine::CreateFromFile(const std::wstring& filename) {
    const WCHAR* str = filename.c_str();
    EngineBase* engine = CreateEnginePdfFromFile(str);
    return std::shared_ptr<PdfEngine>(new PdfEngine(engine));
}

int PdfEngine::pageCount() const {
    const EngineBase* engine = reinterpret_cast<EngineBase*>(impl_);
    return engine->PageCount();
}

float PdfEngine::fileDPI() const {
    const EngineBase* engine = reinterpret_cast<EngineBase*>(impl_);
    return engine->GetFileDPI();
}

RectD PdfEngine::pageMediabox(int pageNo) {
    EngineBase* engine = reinterpret_cast<EngineBase*>(impl_);
    const auto r = engine->PageMediabox(pageNo+1); //Sumatra is 1-based
    return {r.x, r.y, r.dx, r.dy};
}

RectD PdfEngine::pageContentBox(int pageNo) {
    EngineBase* engine = reinterpret_cast<EngineBase*>(impl_);
    const auto r = engine->PageContentBox(pageNo + 1); // Sumatra is 1-based
    return {r.x, r.y, r.dx, r.dy};
}

std::shared_ptr<Bitmap> PdfEngine::renderPage(const RenderPageArgs& args) {
    EngineBase* engine = reinterpret_cast<EngineBase*>(impl_);

    //convert my render page args to the one from sumatra
    ::RectD pageRect;
    if (args.pageRect)
        pageRect = ::RectD{args.pageRect->x, args.pageRect->y, args.pageRect->dx, args.pageRect->dy};
    ::RenderPageArgs sArgs(args.pageNo + 1, args.zoom, args.rotation, args.pageRect ? &pageRect : nullptr);

#if 0
    //render bitmap
    ::RenderedBitmap* rb = engine->RenderPage(sArgs);
    if (!rb)
        throw std::exception("Unable to render page");

    //convert HBITMAP to my bitmap format
    //TODO: directly use fz_pixmap without the conversion in EnginePdf.cpp, line 1193
    HBITMAP source = rb->GetBitmap();
    //get info about the source
    BITMAPINFO bmInfo = {0};
    bmInfo.bmiHeader.biSize = sizeof(bmInfo.bmiHeader);
    if (!GetDIBits(nullptr, source, 0, 0, nullptr, &bmInfo, DIB_RGB_COLORS)) {
        throw std::exception("Unable to query BITMAPINFO structure");
    }
    //create the pixel buffer
    unsigned char* data = new unsigned char[bmInfo.bmiHeader.biSizeImage];
    //now set the correct settings and grab the pixels
    bmInfo.bmiHeader.biBitCount = 32;
    bmInfo.bmiHeader.biCompression = BI_RGB;
    bmInfo.bmiHeader.biHeight = abs(bmInfo.bmiHeader.biHeight);
    if (!GetDIBits(nullptr, source, 0, bmInfo.bmiHeader.biHeight, data, &bmInfo, DIB_RGB_COLORS)) {
        throw std::exception("Unable to grab the pixels");
    }

    //free the rendered bitmap
    delete rb;

    //create and return the bitmap
    return std::shared_ptr<Bitmap>(new Bitmap(
        bmInfo.bmiHeader.biWidth, bmInfo.bmiHeader.biHeight, 4, data));
#else
    //render directly into a bitmap
    const auto* sb = EnginePdfRenderPageToBitmap(engine, sArgs);
    if (!sb) {
        throw std::exception("Unable to render page");
    }
    if (sb->channels != 4) {
        throw std::exception("Expected a four channel image");
    }
    //directly use the data from the library
    std::shared_ptr<Bitmap> ret(new Bitmap(sb->width, sb->height, 4, sb->data));
    delete sb;
    return ret;
#endif
}


} // namespace SumatraPdfApi
