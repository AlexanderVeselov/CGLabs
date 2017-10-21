#ifndef UTILS_HPP
#define UTILS_HPP

#include <stdexcept>
#include <sstream>

#define THROW_RUNTIME(message) \
    throw std::runtime_error(((std::ostringstream&) (std::ostringstream("") << message)).str().c_str());

template<class T>
class ScopedObject
{
public:
    explicit ScopedObject(T *p = nullptr) : _pointer(p) {}
    ~ScopedObject()
    {
        if (_pointer)
        {
            _pointer->Release();
            _pointer = nullptr;
        }
    }    

    T* Get() const { return _pointer; }
    bool IsNull() const { return (!_pointer); }
    void Reset(T *p = 0) { if (_pointer) { _pointer->Release(); } _pointer = p; }

    T& operator*() { return *_pointer; }
    T* operator->() { return _pointer; }
    T* const * operator&() const { return &_pointer; }
    T** operator&() { return &_pointer; }

private:
    ScopedObject(const ScopedObject&);
    ScopedObject& operator=(const ScopedObject&);

    T* _pointer;

};

#endif // UTILS_HPP
