/**
 * This project is used to demostrate how classic Paxos works, no for production. 
 * However, you can leverage the source code or the idea in the project under the
 * license of Apache 2
 **/
#ifndef _CPPAXOS_MSG_GLOBAL_HEADER_
#define _CPPAXOS_MSG_GLOBAL_HEADER_

#define __nocopy__(clazz) \
    private:\
    clazz(const clazz&) = delete;\
    clazz& operator=(const clazz&) = delete;\

#define __interface_body__(clazz) \
    public:\
    clazz(){} \
    virtual ~clazz() {} \
    __nocopy__(clazz)


#define auto_lock(lock) std::lock_guard<std::mutex> guard(lock)
#define recur_lock(lock) std::lock_guard<std::recursive_mutex> guard(lock)

#include <cstdlib>
#include <cstdio>
#include <queue>
#include <vector>
#include <unordered_map>
#include <map>
#include <mutex>
#include <thread>
#include <stdexcept>
#include <iostream>
#include <string>
#include "basic_types.hxx"
#include "async_result.hxx"
#include "msg_bus.hxx"
#include "rpc.hxx"
#include "paxos_srv.hxx"

#endif //_CPPAXOS_MSG_GLOBAL_HEADER_