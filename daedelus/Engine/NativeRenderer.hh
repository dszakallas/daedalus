#pragma once

#include <CoreGraphics/CoreGraphics.h>
#include <MetalKit/MetalKit.hpp>

#define INLINE _MTL_INLINE

struct NativeRenderer : public MTK::ViewDelegate {};
