#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <memory>
#pragma comment(lib, "gdiplus.lib")

// Decode an embedded PNG resource without extracting files to disk.
// The original SVG remains the protected source artwork.
namespace primeeight_gui {
class EmbeddedLogo {
  ULONG_PTR token_ = 0;
  std::unique_ptr<Gdiplus::Bitmap> bitmap_;
public:
  EmbeddedLogo() {
    Gdiplus::GdiplusStartupInput input;
    if (Gdiplus::GdiplusStartup(&token_, &input, nullptr) != Gdiplus::Ok) return;
    HRSRC resource = FindResourceW(nullptr, MAKEINTRESOURCEW(101), L"PNG");
    if (!resource) return;
    HGLOBAL loaded = LoadResource(nullptr, resource);
    if (!loaded) return;
    const DWORD size = SizeofResource(nullptr, resource);
    HGLOBAL buffer = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!buffer) return;
    void* dst = GlobalLock(buffer);
    const void* src = LockResource(loaded);
    if (dst && src) CopyMemory(dst, src, size);
    GlobalUnlock(buffer);
    IStream* stream = nullptr;
    if (CreateStreamOnHGlobal(buffer, TRUE, &stream) == S_OK) {
      bitmap_.reset(Gdiplus::Bitmap::FromStream(stream));
      if (bitmap_ && bitmap_->GetLastStatus() != Gdiplus::Ok) bitmap_.reset();
      stream->Release();
    } else GlobalFree(buffer);
  }
  ~EmbeddedLogo() {
    bitmap_.reset();
    if (token_) Gdiplus::GdiplusShutdown(token_);
  }
  void draw(HDC dc, int x, int y, int width, int height) const {
    if (!bitmap_) return;
    Gdiplus::Graphics graphics(dc);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.DrawImage(bitmap_.get(), x, y, width, height);
  }
};
}
