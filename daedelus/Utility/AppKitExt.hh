#pragma once

#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

#define INLINE _MTL_INLINE

namespace NSExt {

INLINE
NS::String* UTF8String(const char * charStr) {
    return NS::String::string(charStr, NS::StringEncoding::UTF8StringEncoding);
}

} /* namespace NSExt */
