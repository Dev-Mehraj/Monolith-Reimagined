// Singleton.h - Generic singleton template
#ifndef MONOLITH_UTILS_SINGLETON_H
#define MONOLITH_UTILS_SINGLETON_H

#include <mutex>
#include <memory>

namespace monolith {

/**
 * @brief Thread-safe singleton template using Meyer's Singleton pattern
 */
template<typename T>
class Singleton
{
public:
    static T& instance()
    {
        static T instance;
        return instance;
    }

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;

protected:
    Singleton() = default;
    virtual ~Singleton() = default;
};

/**
 * @brief Singleton with lazy initialization and explicit control
 */
template<typename T>
class LazySingleton
{
public:
    static T* instance()
    {
        std::call_once(m_onceFlag, []() {
            m_instance.reset(new T());
        });
        return m_instance.get();
    }

    static void destroy()
    {
        m_instance.reset();
    }

    LazySingleton(const LazySingleton&) = delete;
    LazySingleton& operator=(const LazySingleton&) = delete;

protected:
    LazySingleton() = default;
    virtual ~LazySingleton() = default;

private:
    static std::once_flag m_onceFlag;
    static std::unique_ptr<T> m_instance;
};

template<typename T>
std::once_flag LazySingleton<T>::m_onceFlag;

template<typename T>
std::unique_ptr<T> LazySingleton<T>::m_instance;

} // namespace monolith

#endif // MONOLITH_UTILS_SINGLETON_H
