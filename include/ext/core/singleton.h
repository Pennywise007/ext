/*
Default thread safe singleton

Example of using:

class Logger
{
    friend ext::Singleton<Logger>;
public:
    LoggerData() = default;
    ~LoggerData() = default;
};

*/

#pragma once

#include <atomic>

#include <ext/error/dump_writer.h>

#include <ext/types/utils.h>

namespace ext {

// Flag that shows created singletons
template<class T>
std::atomic_bool kSingletonCreated = false;

// Flag that shows destroyed singletons
template<class T>
std::atomic_bool kSingletonDestroyed = false;

template<class T>
struct Singleton final
{
    static T& Instance()
    {
        /*
            Check the situations like this:

            struct MainSingleton
            {
                MainSingleton() { get_singleton<OtherSingleton>(); }
                ~MainSingleton() { get_singleton<OtherSingleton>(); }
            };

            During destroying services OtherSingleton will be destroyed first and when we try to get it in the
            MainSingleton destructor we might create an inconsistency. 

        */
        if (kSingletonDestroyed<T>)
        {
            std::cerr << "Trying to get already destroyed service " << ext::type_name<T>()
                << ". Check service declaration order." << std::endl;
            if (IsDebuggerPresent())
                DebugBreak(); 
        }

        // according to the standard, this code is lazy and thread safe
        static SingletonWatcher watcher;
        return watcher.object;
    }

    Singleton() = delete; // no constructor
    ~Singleton() = delete; // no destructor

    // prohibit copying
    Singleton(Singleton const&) = delete;
    Singleton& operator= (Singleton const&) = delete;

private:
    // Help class which will help to detect objects creation after it destroying
    struct SingletonWatcher
    {
        SingletonWatcher() { kSingletonCreated<T> = true; }
        ~SingletonWatcher() { kSingletonDestroyed<T> = true; }
        SingletonWatcher(SingletonWatcher const&) = delete;
        SingletonWatcher& operator= (SingletonWatcher const&) = delete;

        T object;
    };
};

template<class T>
[[nodiscard]] static T& get_singleton()
{
    return Singleton<T>::Instance();
}

} // namespace ext
