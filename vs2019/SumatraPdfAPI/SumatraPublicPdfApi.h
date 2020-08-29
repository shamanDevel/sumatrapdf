#pragma once

#ifdef SUMATRA_PUBLIC_PDF_API_EXPORTS
#define SUMATRA_PDF_API __declspec(dllexport)
#else
#define SUMATRA_PDF_API __declspec(dllimport)
#endif

#include <ostream>
#include <memory>
#include <string>
#include <math.h>
#include <optional>
#include <cassert>

namespace SumatraPdfApi {

struct SUMATRA_PDF_API NonCopyable {
    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable() = default;
};

template <typename T>
class Vec2 {
    T x, y;

    friend std::ostream& operator<<(std::ostream& o, const Vec2<T>& v) {
        o << "(" << v.x << ", " << v.y << ")";
        return o;
    }
    friend std::wostream& operator<<(std::wostream& o, const Vec2<T>& v) {
        o << "(" << v.x << ", " << v.y << ")";
        return o;
    }
};


template <typename T>
class RectT {
  public:
    T x, y;
    T dx, dy;

    RectT() : x(0), y(0), dx(0), dy(0) {
    }
    RectT(T x, T y, T dx, T dy) : x(x), y(y), dx(dx), dy(dy) {
    }

    static RectT FromXY(T xs, T ys, T xe, T ye) {
        if (xs > xe)
            std::swap(xs, xe);
        if (ys > ye)
            std::swap(ys, ye);
        return RectT(xs, ys, xe - xs, ye - ys);
    }

    template <typename S>
    RectT<S> Convert() const {
        return RectT<S>((S)x, (S)y, (S)dx, (S)dy);
    }

    RectT<int> ToInt() const {
        return RectT<int>((int)floor(x + 0.5), (int)floor(y + 0.5), (int)floor(dx + 0.5), (int)floor(dy + 0.5));
    }
    // cf. fz_roundrect in mupdf/fitz/base_geometry.c
#ifndef FLT_EPSILON
#define FLT_EPSILON 1.192092896e-07f
#endif
    RectT<int> Round() const {
        return RectT<int>::FromXY((int)floor(x + FLT_EPSILON), (int)floor(y + FLT_EPSILON),
                                  (int)ceil(x + dx - FLT_EPSILON), (int)ceil(y + dy - FLT_EPSILON));
    }

    bool IsEmpty() const {
        return dx == 0 || dy == 0;
    }
    bool empty() const {
        return dx == 0 || dy == 0;
    }

    bool Contains(Vec2<T> pt) const {
        if (pt.x < this->x)
            return false;
        if (pt.x > this->x + this->dx)
            return false;
        if (pt.y < this->y)
            return false;
        if (pt.y > this->y + this->dy)
            return false;
        return true;
    }

    /* Returns an empty rectangle if there's no intersection (see IsEmpty). */
    RectT Intersect(RectT other) const {
        /* The intersection starts with the larger of the start coordinates
           and ends with the smaller of the end coordinates */
        T _x = std::max(this->x, other.x);
        T _y = std::max(this->y, other.y);
        T _dx = std::min(this->x + this->dx, other.x + other.dx) - _x;
        T _dy = std::min(this->y + this->dy, other.y + other.dy) - _y;

        /* return an empty rectangle if the dimensions aren't positive */
        if (_dx <= 0 || _dy <= 0)
            return RectT();
        return RectT(_x, _y, _dx, _dy);
    }

    RectT Union(RectT other) const {
        if (this->dx <= 0 && this->dy <= 0)
            return other;
        if (other.dx <= 0 && other.dy <= 0)
            return *this;

        /* The union starts with the smaller of the start coordinates
           and ends with the larger of the end coordinates */
        T _x = std::min(this->x, other.x);
        T _y = std::min(this->y, other.y);
        T _dx = std::max(this->x + this->dx, other.x + other.dx) - _x;
        T _dy = std::max(this->y + this->dy, other.y + other.dy) - _y;

        return RectT(_x, _y, _dx, _dy);
    }

    RectT offset(T _x, T _y) const {
        return RectT(x + _x, y + _y, dx, dy);
    }

    RectT inflate(T _x, T _y) const {
        return RectT(x - _x, y - _y, dx + 2 * _x, dy + 2 * _y);
    }

    RectT<T> scale(T _sx, T _sy) const {
        return RectT<T>(x * _sx, y * _sy, dx * _sx, dy * _sy);
    }
    RectT<T> scale(T _s) const {
        return scale(_s, _s);
    }

    Vec2<T> TL() const {
        return Vec2<T>{x, y};
    }
    Vec2<T> BR() const {
        return Vec2<T>{x + dx, y + dy};
    }
    Vec2<T> Size() const {
        return Vec2<T>{dx, dy};
    }

    bool operator==(const RectT<T>& other) const {
        return this->x == other.x && this->y == other.y && this->dx == other.dx && this->dy == other.dy;
    }
    bool operator!=(const RectT<T>& other) const {
        return !this->operator==(other);
    }

    friend std::ostream& operator<<(std::ostream& o, const RectT<T>& v) {
        o << "[" << v.x << "," << v.y << "; " << v.dx << "," << v.dy << "]";
        return o;
    }
    friend std::wostream& operator<<(std::wostream& o, const RectT<T>& v) {
        o << "[" << v.x << "," << v.y << "; " << v.dx << "," << v.dy << "]";
        return o;
    }
};
typedef RectT<double> RectD;

/**
 * Information on which page should be rendered and how.
 */
struct SUMATRA_PDF_API RenderPageArgs {
    int pageNo = 0; //the page number from 0 to engine->pageCount()-1
    /**
     * The zoom value, output size in pixels = page size in dots * zoom
     */
    float zoom = 0;
    int rotation = 0; //rotation
    /* if null: defaults to the page's mediabox */
    RectD* pageRect = nullptr;
};

class SUMATRA_PDF_API PdfEngine;

/**
 * The rendered page as 32bit image (RGBA)
 */
class SUMATRA_PDF_API Bitmap : private NonCopyable {
  public:
    const int width;
    const int height;
    const int channels; // will always be 4
    const unsigned char* const data;
    /**
     * \brief Maps the position in the bitmap
     * to the linearized index of the data vector.
     * \param x the x position, from 0 to width-1
     * \param y the y position, from 0 to height-1
     * \param channel the channel, from 0 to channels (red, green, blue, alpha)
     */
    inline int idx(int x, int y, int channel) {
        assert(x >= 0);
        assert(x < width);
        assert(y >= 0);
        assert(y < height);
        assert(channel >= 0);
        assert(channel < channels);
        return channel + channels * (x + width * y);
    }

    ~Bitmap() {
        delete[] data;
    }

  private:
    Bitmap(int width, int height, int channels, const unsigned char* data)
        : width(width), height(height), channels(channels), data(data)
    {}
    friend class PdfEngine;
};
typedef std::shared_ptr<Bitmap> Bitmap_ptr;

/**
 * \brief The main PDF engine.
 * It loads and holds the PDF file, allows querying of attributes like number of pages,
 * DPI and page dimensions.
 * Single pages can be rendered via renderPage(const RenderPageArgs&).
 */
class SUMATRA_PDF_API PdfEngine : private NonCopyable {
  private:
    void* const impl_;
    PdfEngine(void* impl);

  public:
    ~PdfEngine();

    static bool IsPdfFile(const std::wstring& filename);
    static std::shared_ptr<PdfEngine> CreateFromFile(const std::wstring& filename);

    /**
     * \brief Returns the number of pages in the PDF file.
     */
    int pageCount() const;
    /**
     * \brief Returns the DPI of the file.
     * The DPI for a file is needed when converting internal measures to physical ones
     */
    float fileDPI() const;

    /**
     * \brief Returns the dimension (media box) of the specified page in dots.
     * Divide it by the fileDPI() to get the dimension in inches.
     * \param pageNo the page index from 0 to pageCount()-1
     * \return the page dimension in dots.
     */
    RectD pageMediabox(int pageNo);

    /**
     * \brief The box inside PageMediabox that actually contains any relevant content,
     * used for auto-cropping in Fit Content mode, can be pageMediabox().
     * Divide it by the fileDPI() to get the dimension in inches.
     * \param pageNo the page index from 0 to pageCount()-1
     * \return the content dimension in dots.
     */
    RectD pageContentBox(int pageNo);

    /**
     * \brief Renders the page.
     * This method is thread-safe. It can be (and is expected to be) called
     * from a background thread.
     */
    std::shared_ptr<Bitmap> renderPage(const RenderPageArgs& args);
};
typedef std::shared_ptr<PdfEngine> PdfEngine_ptr;

}
