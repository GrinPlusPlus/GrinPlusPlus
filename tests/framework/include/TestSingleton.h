#pragma once

#include <mutex>

namespace test
{
    template<class Type>
    struct SingleTon
    {
    private:
        static Type& instance()
        {
            // Use static function scope variable to 
            // correctly define lifespan of object.
            static Type instance;
            return instance;
        }
    public:
        static Type& get()
        {
            // Note the constructor of std::once_flag
            // is a constexpr and is thus done at compile time
            // thus it is immune to multithread construction
            // issues as nothing is done at runtime.
            static std::once_flag flag;

            // Make sure all threads apart from one wait
            // until the first call to instance has completed.
            // This guarantees that the object is fully constructed
            // by a single thread.
            std::call_once(flag, [] { instance(); });

            // Now all threads can go get the instance.
            // as it has been constructed.
            return instance();
        }
    };
}