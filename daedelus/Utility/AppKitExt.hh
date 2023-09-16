#pragma once

#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

#define INLINE _MTL_INLINE

namespace NS {
namespace Private {
namespace Selector {
_NS_PRIVATE_DEF_SEL(stringWithContentsOfFile_encoding_error_, "stringWithContentsOfFile:encoding:error:");
} /* namespace NS */
} /* namespace Private */
} /* namespace Selector */

namespace NSExt {

struct SendMessage : public NS::Object {
    template <typename _Ret, typename... _Args>
    static _Ret sendMessage(const void* pObj, SEL selector, _Args... args) {
        return NS::Object::sendMessage<_Ret, _Args...>(pObj, selector, args...);
    }
};

INLINE
NS::String* UTF8String(const char * charStr) {
    return NS::String::string(charStr, NS::StringEncoding::UTF8StringEncoding);
}

INLINE
NS::String* StringWithContentsOfFile(NS::String* fileName, NS::StringEncoding encoding, NS::Error** error) {
    using namespace NS;
    return SendMessage::sendMessage<String*>(_NS_PRIVATE_CLS(NSString), _NS_PRIVATE_SEL(stringWithContentsOfFile_encoding_error_), fileName, encoding, error);
}

} /* namespace NSExt */



