#ifndef SHM_MANAGER_H
#define SHM_MANAGER_H

#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>

namespace shm {

class BaseSHMConditionVariable {
public:
    virtual ~BaseSHMConditionVariable() {}
    virtual void wait(pthread_mutex_t* mutex) = 0;
    virtual void notify_one() = 0;
    virtual void notify_all() = 0;
};

class BaseSHMMutex {
public:
    BaseSHMMutex() {};
    ~BaseSHMMutex() {};

    virtual void Lock() = 0;
    virtual void Unlock() = 0;
};

template<typename T>
class BaseSHMArea {
public:
    BaseSHMArea(std::string name, size_t size_of_shm_area)
        : name_(name)
        , size_of_shm_area_(size_of_shm_area) {}

    virtual std::remove_pointer_t<T>* AttachSHM() = 0;
    virtual std::remove_pointer_t<T>* GetSHMAddr() = 0;
    virtual void DeattachSHM() = 0;
    virtual std::shared_ptr<BaseSHMMutex> GetLock() = 0;

protected:
    std::string name_;
    size_t size_of_shm_area_;
};
} // namespace shm

#endif
