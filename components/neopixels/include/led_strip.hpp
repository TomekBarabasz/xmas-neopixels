#pragma once
#include <cstdint>

namespace Neopixel
{
struct RGB;
struct HSV;
struct LedStripConfig;
struct LedStrip
{
    static LedStrip* create(const LedStripConfig&);
    virtual int getLength() const = 0;
    virtual RGB* getBuffer() = 0;
    virtual void setPixelsRGB(int first, int num, const RGB*) = 0;
    virtual void fillPixelsRGB(int first, int num, const RGB&) = 0;
    virtual void setPixelsHSV(int first, int num, const HSV*) = 0;
    virtual void refresh(bool wait=false) = 0;
    virtual void copyFrontToBack() = 0;
    virtual bool waitReady(uint32_t timeout_ms) = 0;
    virtual void release() = 0;
protected:
    virtual ~LedStrip(){}
};
}