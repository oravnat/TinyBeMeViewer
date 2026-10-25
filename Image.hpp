//  This file is part of TinyBeMeViewer by Or Avnat: https://github.com/oravnat/TinyBeMeViewer

#ifndef IMAGE_H
#define IMAGE_H

#include <strstream>
#include <cstdint>

struct RGBA
{
    uint8_t    red;
    uint8_t    green;
    uint8_t    blue;
    uint8_t    alpha;
};

struct BGRA
{
    uint8_t    blue;
    uint8_t    green;
    uint8_t    red;
    uint8_t    alpha;
};

struct ImageRect
{
	int left, top;
	int width, height;
};

// The image data order is RGB for 24 bits and RGBA for 32 bits, but the palette for 8 bits is in BGRA order
// Important note: the original Image class from the older Windows version of BeMe uses the BGR and BGRA orders for 24 and 32 bits images respectively
class Image
{
public:
	Image() {}
	// Only for 8 bits images:
	Image(uint8_t* data, int w, int h, int widhtBytes, BGRA* quad);
	// Only for 32 bits images:
	Image(uint8_t* data, int w, int h, int widthBytes);
	~Image() {Release();}
protected:
	Image(int w, int h, int widthBytes, int bitsPerPixel);
public:
	// The copy operator takes the ownership of the data from the other Image (the other
    // Image is empty after the operator call).
	Image & operator = (Image&& other);
public:
	void GetPixel(int x, int y, uint8_t& r, uint8_t& g, uint8_t& b);
	RGBA GetPixel(int x, int y);
	// Only for 32 bit currently:
	void SetPixel(int x, int y, RGBA color);
	void Release();
	// (0, 0) is the top left corner for this function:
	Image* Rect(int left, int top, int width, int height) const;
	// For top-down images:
	void FilterUp();
	void FilterUp(const ImageRect& rect);
	void UnfilterUp();
    void SetSize(int width, int height, int widthBytes) {w = width; h = height; this->widthBytes = widthBytes;}
    void Copy32To32(uint8_t* bits, int destWidthBytes, int destWidth, int destHeight);
    void CopyToTransparentIndex(uint8_t* bits, int destWidthBytes, int destWidth, int destHeight, int transpapentIndex);
public:
	bool IsEmpty() const {return data == NULL;}
public:
    uint8_t* data = nullptr;
    int dataSize = 0;
    int w, h; // width, height
    int widthBytes;
    int bitsPerPixel;
    BGRA q[256];
    int paletteCount;
    int left, top;
    bool transparent;
    int transparentIndex;
    bool full; // true if the image is for the full screen
};


#endif //end of #ifndef IMAGE_H
