/**
 * This project is used to demostrate how classic Paxos works, no for production. 
 * However, you can leverage the source code or the idea in the project under the
 * license of Apache 2
 **/

#ifndef _CPPAXOS_BASIC_TYPES_HXX_
#define _CPPAXOS_BASIC_TYPES_HXX_

struct ballot_number{
    size_t server_id_;
    size_t number_;

    ballot_number(size_t srv_id, size_t number = 0) : server_id_(srv_id), number_(number) {}

    ballot_number inline increase(const size_t server_id){
        return ballot_number(server_id, server_id <= server_id_ ? number_ + 1 : number_);
    }
};

bool inline operator > (const ballot_number& number1, const ballot_number& number2){
    return number1.number_ > number2.number_ || 
        (number1.number_ == number2.number_ && number1.server_id_ > number2.server_id_);
}

bool inline operator == (const ballot_number& number1, const ballot_number& number2){
    return number1.number_ == number2.number_ && number1.server_id_ == number2.server_id_;
}

bool inline operator <= (const ballot_number& number1, const ballot_number& number2){
    return !(number1 > number2);
}

bool inline operator >= (const ballot_number& number1, const ballot_number& number2){
    return (number1 > number2) || (number1 == number2);
}

bool inline operator < (const ballot_number& number1, const ballot_number& number2){
    return !(number1 >= number2);
}

enum msg_type{
    nil,
    try_new_ballot,
    next_ballot,
    last_vote,
    begin_ballot,
    voted,
    success
};

enum status{
    idle,
    trying,
    polling
};

struct vote{
    size_t server_id_;
    ballot_number ballot_number_;
    int value_;

    vote(size_t pst, const ballot_number& ballot, int value) : server_id_(pst), ballot_number_(ballot), value_(value) {}
};

struct rpc_msg {
    msg_type type_;
    size_t src_;
    size_t round_;
    ballot_number ballot_number_;
    int value_;
    std::shared_ptr<vote> vote_;
    rpc_msg(msg_type type, size_t src, size_t round, const ballot_number& ballot) : type_(type), src_(src), round_(round), ballot_number_(ballot) {}
};

enum log_level{
    debug,
    info,
    warn,
    error,
    fatal
};

class logger{
    __interface_body__(logger);

public:
    virtual void log(log_level level, const std::string& msg) = 0;
};

class console_logger: public logger{
public:
    console_logger() : lock_(){}
    __nocopy__(console_logger)

public:
    virtual void log(log_level level, const std::string& msg) override{
        auto_lock(lock_);
        std::cout << msg << std::endl;
    }
private:
    std::mutex lock_;
};

template<int N>
class strfmt {
public:
    strfmt(const char* fmt)
        : fmt_(fmt) {
    }

    template<typename ... TArgs>
    const char* fmt(TArgs... args) {
        ::snprintf(buf_, N, fmt_, args...);
        return buf_;
    }

__nocopy__(strfmt)
private:
    char buf_[N];
    const char* fmt_;
};

typedef strfmt<100> sstrfmt;
typedef strfmt<200> lstrfmt;

#endif //_CPPAXOS_BASIC_TYPES_HXX_