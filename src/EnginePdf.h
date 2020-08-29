/* Copyright 2020 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

bool IsEnginePdfSupportedFile(const WCHAR* fileName, bool sniff = false);
EngineBase* CreateEnginePdfFromFile(const WCHAR* fileName, PasswordUI* pwdUI = nullptr);
EngineBase* CreateEnginePdfFromStream(IStream* stream, PasswordUI* pwdUI = nullptr);

//Shaman: extension to render directly a fz_pixmap
//Result must be cleared with fz_drop_pixmap(ctx, pix);
class SimpleBitmap {
  public:
    const int width;
    const int height;
    const int channels; // will always be 4
    const unsigned char* const data; //has to be manually deleted
};
SimpleBitmap* EnginePdfRenderPageToBitmap(EngineBase* engine, RenderPageArgs& args);
