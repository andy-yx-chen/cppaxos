/**
 * This project is used to demostrate how classic Paxos works, no for production. 
 * However, you can leverage the source code or the idea in the project under the
 * license of Apache 2
 **/
#ifndef _CPPAXOS_MSG_BUS_HXX_
#define _CPPAXOS_MSG_BUS_HXX_
template<size_t N>
class msg_bus;

template<>
class msg_bus<0>;

template<size_t N>
class msg_bus {
public:
    typedef std::pair<std::shared_ptr<rpc_msg>, std::shared_ptr<async_result<std::shared_ptr<rpc_msg>>>> message;
    
    class msg_queue {
    public:
        msg_queue()
            : queue_(), cv_(), lock_() {}
        __nocopy__(msg_queue)
    public:
        void enqueue(const message& msg) {
            {
                auto_lock(lock_);
                queue_.push(msg);
            }

            cv_.notify_one();
        }

        message dequeue() {
            std::unique_lock<std::mutex> u_lock(lock_);
            while (queue_.size() == 0) {
                cv_.wait(u_lock);
            }

            message& front = queue_.front();
            message m = std::make_pair(front.first, front.second);
            queue_.pop();
            return m;
        }
    private:
        std::queue<message> queue_;
        std::condition_variable cv_;
        std::mutex lock_;
    };

    typedef std::unordered_map<std::string, std::shared_ptr<msg_queue>> msg_q_map;

public:
    msg_bus()
        : q_map_() {
        for(size_t i = 1; i <= N; ++i){
            char str[10];
            snprintf(str, 10, "port%zd", i);
            q_map_.insert(std::make_pair(str, std::make_shared<msg_queue>()));
        }
    }

    __nocopy__(msg_bus)

public:
    void send_msg(const std::string& port, message& msg) {
        msg_q_map::const_iterator itor = q_map_.find(port);
        if (itor != q_map_.end()) {
            itor->second->enqueue(msg);
            return;
        }

        // this is for test usage, if port is not found, faulted
        throw std::runtime_error(sstrfmt("bad port %s for msg_bus").fmt(port.c_str()));
    }

    msg_queue& get_queue(const std::string& port) {
        msg_q_map::iterator itor = q_map_.find(port);
        if (itor == q_map_.end()) {
            throw std::runtime_error("bad port for msg_bus, no queue found.");
        }

        return *itor->second;
    }

private:
    msg_q_map q_map_;
};

#endif //_CPPAXOS_MSG_BUS_HXX_
