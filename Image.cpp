//  This file is part of TinyBeMeViewer by Or Avnat: https://github.com/oravnat/TinyBeMeViewer

#include "Image.hpp"
#include <strstream>
#include <assert.h>
#include <fstream>
#include <cstring>

struct ImagePropVer3
{
	int32_t left, top;
    int32_t w, h; // width, height
    int32_t widthBytes;
    int32_t bitsPerPixel;
    int32_t paletteCount;
    int32_t transparentIndex; // -1 if no transparency
    uint32_t flags;
};

// Only for 8 bits images (called from Octree() method):
Image::Image(uint8_t *data, int w, int h, int widthBytes, BGRA* quad)
{
	this->data = data;
	this->w = w;
	this->h = h;
	this->widthBytes = widthBytes;
	bitsPerPixel = 8;
	memcpy(q, quad, 256 * sizeof(BGRA));
	left = top = 0;
	full = true;
	paletteCount = 256;
	transparent = false;
	dataSize = widthBytes * h;
}

// Only for 32 bits images:
Image::Image(uint8_t * data, int w, int h, int widthBytes)
{
	this->data = data;
	this->w = w;
	this->h = h;
	this->widthBytes = widthBytes;
	bitsPerPixel = 32;
	left = top = 0;
	full = true;
	paletteCount = 256;
	transparent = false;
	dataSize = widthBytes * h;
}

Image::Image(int w, int h, int widthBytes, int bitsPerPixel)
{
	this->w = w;
	this->h = h;
	this->widthBytes = widthBytes;
	this->bitsPerPixel = bitsPerPixel;
	left = top = 0;
	full = false;
	paletteCount = 256;
	transparent = false;
}

// The copy operator takes the ownership of the data from the other Image (the other Image
// is empty after the operator call).
Image & Image::operator = (Image&& other)
{
	memcpy(this, &other, sizeof(Image));
	other.data = NULL;
	other.dataSize = 0;
	return *this;
}

void Image::GetPixel(int x, int y, uint8_t& r, uint8_t& g, uint8_t& b)
{
	int index;

	if (bitsPerPixel == 24)
	{
		r = data[y*widthBytes + x*3];
		g = data[y*widthBytes + x*3 + 1];
		b = data[y*widthBytes + x*3 + 2];
	}
	else if (bitsPerPixel == 8)
	{
		index = data[y*widthBytes + x];
		b = q[index].blue;
		g = q[index].green;
		r = q[index].red;
	}
	else if (bitsPerPixel == 32)
	{
		r = data[y*widthBytes + x*4];
		g = data[y*widthBytes + x*4 + 1];
		b = data[y*widthBytes + x*4 + 2];
	}
}

RGBA Image::GetPixel(int x, int y)
{
    uint8_t r, g, b;

    GetPixel(x, y, r, g, b);
    return {r, g, b, 255};
}


void Image::SetPixel(int x, int y, RGBA color)
{
	assert(bitsPerPixel == 32);
	*(RGBA *)&data[y*widthBytes + x*4] = color;
}

void Image::Release()
{
	delete [] data; 
	data = NULL;
	dataSize = 0;
}

void Image::FilterUp()
{
	if (h < 2)
		return;
	// p starting from the last pixel (one pixel after actually):
	uint8_t* p = data + h * widthBytes;
	// up starting from the first row:
    uint8_t* up = data + (h - 1) * widthBytes;
	int n = widthBytes * (h - 1);
	while (n--)
	{
		p--;
		up--;
		*p = *p - *up;
	}
}

void Image::UnfilterUp()
{
	if (h < 2)
		return;
	// p starting from the second row:
    uint8_t* p = data + widthBytes;
	// up starting from the first row:
    uint8_t* up = data;
	int n = widthBytes * (h - 1);
	while (n--)
	{
		*p = *p + *up;
		p++;
		up++;
	}
}

void Image::FilterUp(const ImageRect& rect)
{
	if (rect.width < 2)
		return;
	int bytesPerPixel = bitsPerPixel / 8;
	for (int i = rect.top + rect.height - 1; i > rect.top; i--)
	{
        uint8_t* p = data + i * widthBytes + (rect.left + rect.width) * bytesPerPixel;
        uint8_t* up = p - widthBytes;
		for (int j = rect.width * bytesPerPixel; j > 0; j--)
		{
			p--;
			up--;
			*p = *p - *up;
		}
	}
}

void Image::Copy32To32(uint8_t* bits, int destWidthBytes, int destWidth, int destHeight)
{
    int top = this->top;
    int left = this->left;
    int endx = std::min(left + w, destWidth);
    int endy = std::min(top + h, destHeight);
    // Ensure this is a bottom up image:
    assert(destHeight > 0);
    assert(bitsPerPixel == 32);

    int bytesToCopyPerLine = std::min(widthBytes, (int)((destWidth - left) * sizeof(RGBA)));
    for (int y = top; y < endy; y++)
    {
        void* src = data + (y - top) * widthBytes;
        RGBA* dest = (RGBA*)(bits + y * destWidthBytes) + left;
        memcpy(dest, src, bytesToCopyPerLine);
    }
}

void Image::CopyToTransparentIndex(uint8_t* bits, int destWidthBytes, int destWidth, int destHeight, int transpapentIndex)
{
    int top = this->top;
    int left = this->left;
    int endx = std::min(left + w, destWidth);
    int endy = std::min(top + h, destHeight);
    // Ensure this is a bottom up image:
    assert(destHeight > 0);

    assert(bitsPerPixel == 8);
    for (int y = top; y < endy; y++)
    {
        uint8_t* src = data + (y - top) * widthBytes;
        RGBA* dest = (RGBA*)(bits + y * destWidthBytes) + left;
        for (int x = left; x < endx; x++, src++, dest++)
        {
            if (*src != transpapentIndex)
            {
                //*dest = q[*src];
                BGRA color = q[*src];
                dest->red = color.red;
                dest->green = color.green;
                dest->blue = color.blue;
                dest->alpha = color.alpha;
           }
        }
    }
}
