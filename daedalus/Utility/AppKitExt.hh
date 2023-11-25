#pragma once

#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

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
    _NS_INLINE
    static _Ret sendMessage(const void* pObj, SEL selector, _Args... args) {
        return NS::Object::sendMessage<_Ret, _Args...>(pObj, selector, args...);
    }
};

_NS_INLINE
static NS::String* UTF8String(const char * charStr) {
    return NS::String::string(charStr, NS::StringEncoding::UTF8StringEncoding);
}

_NS_INLINE
static NS::String* StringWithContentsOfFile(NS::String* fileName, NS::StringEncoding encoding, NS::Error** error) {
    using namespace NS;
    return SendMessage::sendMessage<String*>(_NS_PRIVATE_CLS(NSString), _NS_PRIVATE_SEL(stringWithContentsOfFile_encoding_error_), fileName, encoding, error);
}

template<class T>
struct ScopedRef {
    ScopedRef(const ScopedRef<T>& t): value(t.value) {
        value->retain();
    }
    
    ScopedRef(ScopedRef<T>& t): value(t.value) {
        value->retain();
    }
    
    constexpr T* operator->() {
        return value;
    }
    
    constexpr T* operator*() {
        return value;
    }
    
    ~ScopedRef() {
        value->release();
    }
    
    constexpr bool operator!() {
        return !value;
    }
    
    T* value;
    
    _NS_INLINE
    static ScopedRef<T> make(T* p) {
        return ScopedRef<T>(p);
    }
    
    _NS_INLINE
    static ScopedRef<T> retain(T* p) {
        p->retain();
        return ScopedRef<T>(p);
    }
    
private:
    ScopedRef(T* value) : value(value) {}
};

template<class T>
_NS_INLINE
static ScopedRef<T> RetainScoped(T* p) {
    return ScopedRef<T>::retain(p);
}

template<class T>
_NS_INLINE
static ScopedRef<T> MakeScoped(T* p) {
    return ScopedRef<T>::make(p);
}





} /* namespace NSExt */



