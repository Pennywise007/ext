#pragma once

namespace ext {

/*
Default thread safe singleton

Example of using:

/*class Logger
{
    friend ext::Singleton<Logger>;
public:
    LoggerData() = default;
    ~LoggerData() = default;
};

*/
template<class T>
struct Singleton final
{
    static T& Instance()
    {
        // according to the standard, this code is lazy and thread safe
        static T s;
        return s;
    }

    Singleton() = delete;  // no constructor
    ~Singleton() = delete; // no destructor

    // prohibit copying
    Singleton(Singleton const&) = delete;
    Singleton& operator= (Singleton const&) = delete;
};

// get singleton function
template<class T>
static T& get_service()
{
    return Singleton<T>::Instance();
}

} // namespace ext
