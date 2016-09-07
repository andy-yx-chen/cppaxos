/**
* This project is used to demostrate how classic Paxos works, no for production.
* However, you can leverage the source code or the idea in the project under the
* license of Apache 2
**/

#ifndef _CPPAXOS_RPC_HXX_
#define _CPPAXOS_RPC_HXX_

typedef async_result<std::shared_ptr<rpc_msg>> rpc_result;
typedef rpc_result::handler_type rpc_handler;

class rpc_client {
    __interface_body__(rpc_client)
public:
    virtual void send(const std::shared_ptr<rpc_msg>& msg, rpc_handler& when_done) = 0;
};

class msg_bus_rpc_client : public rpc_client {
public:
    msg_bus_rpc_client(msg_bus<5>& bus, const std::string& port)
        : bus_(bus), port_(port) {}

    __nocopy__(msg_bus_rpc_client)

        virtual void send(const std::shared_ptr<rpc_msg>& msg, rpc_handler& when_done) override {
        std::shared_ptr<async_result<std::shared_ptr<rpc_msg>>> result(std::make_shared<async_result<std::shared_ptr<rpc_msg>>>());
        result->when_ready(when_done);
        msg_bus<5>::message bus_msg(std::make_pair(msg, result));
        bus_.send_msg(port_, bus_msg);
    }
private:
    msg_bus<5>& bus_;
    std::string port_;
};
#endif //_CPPAXOS_RPC_HXX_