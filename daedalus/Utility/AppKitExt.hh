#pragma once

#include <utility>
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
struct ns_ptr {
    constexpr ns_ptr(): value(nullptr) {}
    constexpr ns_ptr(const ns_ptr<T>& t): value(t.value) { value->retain(); }
    constexpr ns_ptr(ns_ptr<T>& t): value(t.value) { value->retain(); }
    constexpr ns_ptr(ns_ptr<T>&& t): value(t.release()) {}
    
    // Assumes t->retain() has already been called. You can use retain(t) that calls retain before wrapping.
    constexpr ns_ptr(T* t) : value(t) {}
    
    constexpr ns_ptr<T>& operator=(std::nullptr_t) {
        reset(nullptr);
        return *this;
    }
    
    constexpr ns_ptr<T>& operator=(const ns_ptr<T>& t) {
        t.value->retain();
        reset(t.value);
        return *this;
    }
    
    constexpr ns_ptr<T>& operator=(ns_ptr<T>& t) {
        t.value->retain();
        reset(t.value);
        return *this;
    }
    
    constexpr ns_ptr<T>& operator=(ns_ptr<T>&& t) {
        reset(t.release());
        return *this;
    }
    
    constexpr void reset(T* t) {
        auto _ret = value;
        value = t;
        if (_ret) { _ret->release(); }
    }
    
    // Follows C++ unique_ptr.release() semantics by forfeiting ownership.
    // Not the same as t.get()->release(), in fact it does not call it!
    constexpr T* release() {
        auto _ret = value;
        value = nullptr;
        return _ret;
    }
    
    void swap(ns_ptr& t) {
        std::swap(&t.value, &value);
    }
    
    constexpr T* get() {
        return value;
    }
    
    constexpr T* operator->() {
        return value;
    }
    
    constexpr std::add_lvalue_reference<T>::type operator*() {
        return *value;
    }
    
    constexpr bool operator!() {
        return !value;
    }
    
    ~ns_ptr() {
        value->release();
    }
    
private:
    T* value;
};

template<class T>
_NS_INLINE
static ns_ptr<T> retain(T* t) {
    t->retain();
    return ns_ptr<T>(t);
}

} /* namespace NSExt */



