#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

#ifndef INFINITE
#define INFINITE 0xFFFFFFFF
#endif

namespace helper {

template<class T>
class MessageBuffer {
  public:
    explicit MessageBuffer(unsigned long maxBuffer = 1024*1024*1024):MAXBUFFER(maxBuffer) {
    }
    virtual ~MessageBuffer(void) {}

    bool Put(const T& data) {
        return Add(data);
    }
    bool Put(T &&data) {
        return Add(std::forward<T>(data));
    }

    bool Add(const T &data) {
        std::unique_lock<std::mutex> lck(m_mtx);
        if (m_dataBuffer.size() > MAXBUFFER) {
            std::runtime_error ex("MessageBuffer size Exceed max buffer.");
            throw  std::exception(ex);
        }
        this->m_dataBuffer.emplace_back(data);
        this->m_cv.notify_one();
        return true;
    }

    bool Add(T &&data) {
        std::unique_lock<std::mutex> lck(m_mtx);
        if (m_dataBuffer.size() > MAXBUFFER) {
            std::runtime_error ex("MessageBuffer size Exceed max buffer.");
            throw  std::exception(ex);
        }
        this->m_dataBuffer.emplace_back(std::forward<T>(data));
        this->m_cv.notify_one();
        return true;
    }

    bool PutToTop(const T &data) {
        return AddToTop(data);
    }
    bool PutToTop(T &&data) {
        return AddToTop(std::forward<T>(data));
    }

    bool AddToTop(T &&data) {
        std::unique_lock<std::mutex> lck(m_mtx);
        if (m_dataBuffer.size() > MAXBUFFER) {
            std::runtime_error ex("MessageBuffer size Exceed max buffer.");
            throw  std::exception(ex);
        }
        this->m_dataBuffer.emplace_front(std::forward<T>(data));
        this->m_cv.notify_one();
        return true;
    }

    bool AddToTop(const T &data) {
        std::unique_lock<std::mutex> lck(m_mtx);
        if (m_dataBuffer.size() > MAXBUFFER) {
            std::runtime_error ex("MessageBuffer size Exceed max buffer.");
            throw  std::exception(ex);
        }
        this->m_dataBuffer.emplace_front(data);
        this->m_cv.notify_one();
        return true;
    }

    bool Get(T& data, uint64_t dwMilliseconeds = INT32_MAX) {
        std::unique_lock<std::mutex> lck(m_mtx);

        bool result = this->m_cv.wait_for(lck, std::chrono::milliseconds(dwMilliseconeds), [&]()->bool{ return !this->m_dataBuffer.empty(); });

        if (result) {
            data = m_dataBuffer.front();
            m_dataBuffer.pop_front();
        }

        return result;
    }

    bool Get(std::queue<T> & data, uint64_t dwMilliseconeds = INT32_MAX) {
        std::unique_lock<std::mutex> lck(m_mtx);

        bool result = this->m_cv.wait_for(lck, std::chrono::milliseconds(dwMilliseconeds), [&]()->bool { return !this->m_dataBuffer.empty(); });

        if (result) {
            while (!m_dataBuffer.empty()) {
                data.push(m_dataBuffer.front());
                m_dataBuffer.pop_front();
            }
        }

        return result;
    }

    size_t Size() {
        std::unique_lock<std::mutex> lck(m_mtx);
        return m_dataBuffer.size();
    }

    bool IsEmpty() {
        return Size() == 0;
    }

    virtual void Clear() {
        std::unique_lock<std::mutex> lck(m_mtx);
        auto tmp = std::move(m_dataBuffer);
    }

  private:
    std::deque<T> m_dataBuffer;
    std::mutex m_mtx;
    std::condition_variable m_cv;
    const unsigned long MAXBUFFER;
};// end MessageBuffer class

template<class T,
         class Container = std::vector<T>,
         class Compare = std::less<typename Container::value_type> >
class CPriorityMessageBuffer {
  public:
    explicit CPriorityMessageBuffer(unsigned long maxBuffer = 1024 * 1024 * 1024) :MAXBUFFER(maxBuffer) {
    }
    virtual ~CPriorityMessageBuffer(void) {}

    bool Put(const T &data) {
        return Add(data);
    }

    bool Put(T &&data) {
        return Add(std::forward<T>(data));
    }

    bool Add(T &&data) {
        std::unique_lock<std::mutex> lck(m_mtx);
        if (m_dataBuffer.size() > MAXBUFFER) {
            std::runtime_error ex("CPriorityMessageBuffer size Exceed max buffer.");
            throw  std::exception(ex);
        }
        this->m_dataBuffer.emplace(std::forward<T>(data));
        this->m_cv.notify_one();
        return true;
    }
    bool Get(T& data, uint64_t dwMilliseconeds = INT32_MAX) {
        std::unique_lock<std::mutex> lck(m_mtx);

        bool result = this->m_cv.wait_for(lck, std::chrono::milliseconds(dwMilliseconeds), [&]()->bool { return !this->m_dataBuffer.empty(); });

        if (result) {
            data = m_dataBuffer.top();
            m_dataBuffer.pop();
        }

        return result;
    }

    bool Get(std::queue<T> & data, uint64_t dwMilliseconeds = INT32_MAX) {
        std::unique_lock<std::mutex> lck(m_mtx);

        bool result = this->m_cv.wait_for(lck, std::chrono::milliseconds(dwMilliseconeds), [&]()->bool { return !this->m_dataBuffer.empty(); });

        if (result) {
            while (!m_dataBuffer.empty()) {
                data.push(m_dataBuffer.top());
                m_dataBuffer.pop();
            }
        }

        return result;
    }

    size_t Size() {
        std::unique_lock<std::mutex> lck(m_mtx);
        return m_dataBuffer.size();
    }

    bool IsEmpty() {
        return Size() == 0;
    }

    virtual void Clear() {
        std::unique_lock<std::mutex> lck(m_mtx);
        auto tmp = std::move(m_dataBuffer);
    }

  protected:
    std::priority_queue<T, Container, Compare> m_dataBuffer;
    std::mutex m_mtx;
    std::condition_variable m_cv;
    const unsigned long MAXBUFFER;
};// end CPriorityMessageBuffer class
}//end namespace helper
