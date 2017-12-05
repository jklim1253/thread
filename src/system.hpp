#pragma once

#include <mutex>
#include <thread>

namespace module
{

template<typename IMPL>
class basic_thread
{
    typedef basic_thread<IMPL> Me;
protected :
    virtual int entry(Me* p) = 0;
    virtual void enter() {}
    virtual void leave() {}
    bool need_stop() const
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        return m_need_stop;
    }
public :
    void start()
    {
        {
            std::lock_guard<std::mutex> locker(m_mutex);
            if (m_running) return;
        }
        m_thread = std::thread(&Me::execute, this, this);
    }
    void stop()
    {
        {
            std::lock_guard<std::mutex> locker(m_mutex);
            m_need_stop = true;
        }
        if (m_thread.joinable())
            m_thread.join();
    }

    bool is_running() const
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        return m_running;
    }

private :
    struct entry_notifier
    {
        std::mutex& m_;
        bool& r_;
        std::function<void()> enter_;
        std::function<void()> leave_;
        entry_notifier(std::mutex& m, bool& r, std::function<void()> enter, std::function<void()> leave)
            : m_(m), r_(r), enter_(enter), leave_(leave)
        {
            std::lock_guard<std::mutex> locker(m_);
            r_ = true;
            enter_();
        }
        ~entry_notifier()
        {
            std::lock_guard<std::mutex> locker(m_);
            r_ = false;
            leave_();
        }
    };
    int execute(Me* p)
    {
        entry_notifier notifier(m_mutex, m_running, std::bind(&basic_thread<IMPL>::enter, this), std::bind(&basic_thread<IMPL>::leave, this));
        return entry(p);
    }

private :
    mutable std::mutex m_mutex;
    bool m_need_stop;
    bool m_running;
    std::thread m_thread;
};



} // namespace system
