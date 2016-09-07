/**
 * This project is used to demostrate how classic Paxos works, no for production. 
 * However, you can leverage the source code or the idea in the project under the
 * license of Apache 2
 **/
#ifndef _CPPAXOS_ASYNC_RESULT_HXX_
#define _CPPAXOS_ASYNC_RESULT_HXX_
template<typename T, typename TE = std::shared_ptr<std::exception>>
class async_result {
public:
    typedef std::function<void(T&, TE&)> handler_type;
    async_result() : err_(), has_result_(false), lock_(), cv_() {}
    explicit async_result(T& result)
        : result_(result), err_(), has_result_(true), lock_(), cv_() {}
    explicit async_result(handler_type& handler)
        : err_(), has_result_(true), handler_(handler), lock_(), cv_() {}

    ~async_result() {}

    __nocopy__(async_result)

public:
    void when_ready(handler_type& handler) {
        std::lock_guard<std::mutex> guard(lock_);
        if (has_result_) {
            handler(result_, err_);
        }
        else {
            handler_ = handler;
        }
    }

    void set_result(T& result, TE& err) {
        {
            std::lock_guard<std::mutex> guard(lock_);
            result_ = result;
            err_ = err;
            has_result_ = true;
            if (handler_) {
                handler_(result, err);
            }
        }

        cv_.notify_all();
    }

    T& get() {
        std::unique_lock<std::mutex> lock(lock_);
        if (has_result_) {
            if (err_ == nullptr) {
                return result_;
            }

            throw err_;
        }

        cv_.wait(lock);
        if (err_ == nullptr) {
            return result_;
        }

        throw err_;
    }

private:
    T result_;
    TE err_;
    bool has_result_;
    handler_type handler_;
    std::mutex lock_;
    std::condition_variable cv_;
};
 #endif //_CPPAXOS_ASYNC_RESULT_HXX_