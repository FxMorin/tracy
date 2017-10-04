#ifndef __TRACYLOCK_HPP__
#define __TRACYLOCK_HPP__

#include "../common/TracySystem.hpp"
#include "TracyProfiler.hpp"

namespace tracy
{

template<class T>
class Lockable
{
public:
    Lockable( const SourceLocation* srcloc )
    {
        Magic magic;
        auto& token = s_token;
        auto item = s_queue.enqueue_begin( token, magic );
        item->hdr.type = QueueType::LockAnnounce;
        item->lockAnnounce.id = (uint64_t)&m_lockable;
        item->lockAnnounce.srcloc = (uint64_t)srcloc;
        s_queue.enqueue_finish( token, magic );
    }

    Lockable( const Lockable& ) = delete;
    Lockable& operator=( const Lockable& ) = delete;

    void lock()
    {
        int8_t cpu;
        const auto thread = GetThreadHandle();
        {
            Magic magic;
            auto& token = s_token;
            auto item = s_queue.enqueue_begin( token, magic );
            item->hdr.type = QueueType::LockWait;
            item->lockWait.id = (uint64_t)&m_lockable;
            item->lockWait.thread = thread;
            item->lockWait.time = Profiler::GetTime( item->zoneBegin.cpu );
            s_queue.enqueue_finish( token, magic );
        }

        m_lockable.lock();

        {
            Magic magic;
            auto& token = s_token;
            auto item = s_queue.enqueue_begin( token, magic );
            item->hdr.type = QueueType::LockObtain;
            item->lockObtain.id = (uint64_t)&m_lockable;
            item->lockObtain.thread = thread;
            item->lockObtain.time = Profiler::GetTime( item->zoneBegin.cpu );
            s_queue.enqueue_finish( token, magic );
        }
    }

    void unlock()
    {
        m_lockable.unlock();

        int8_t cpu;
        Magic magic;
        auto& token = s_token;
        auto item = s_queue.enqueue_begin( token, magic );
        item->hdr.type = QueueType::LockRelease;
        item->lockRelease.id = (uint64_t)&m_lockable;
        item->lockRelease.thread = GetThreadHandle();
        item->lockRelease.time = Profiler::GetTime( item->zoneBegin.cpu );
        s_queue.enqueue_finish( token, magic );
    }

    bool try_lock()
    {
        const auto ret = m_lockable.try_lock();
        if( ret )
        {
            int8_t cpu;
            Magic magic;
            auto& token = s_token;
            auto item = s_queue.enqueue_begin( token, magic );
            item->hdr.type = QueueType::LockObtain;
            item->lockObtain.id = (uint64_t)&m_lockable;
            item->lockObtain.thread = GetThreadHandle();
            item->lockObtain.time = Profiler::GetTime( item->zoneBegin.cpu );
            s_queue.enqueue_finish( token, magic );
        }
        return ret;
    }

private:
    T m_lockable;
};

};

#endif
