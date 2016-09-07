/**
* This project is used to demostrate how classic Paxos works, no for production.
* However, you can leverage the source code or the idea in the project under the
* license of Apache 2
**/

#include "../include/cppaxos.hxx"
#include <chrono>

msg_bus<5> mbus;
std::shared_ptr<logger> l(new console_logger());

void run_paxos_instance(size_t srv_id);

int main() {
    std::cout << "starting 5 instances of paxos server" << std::endl;
    for (size_t id = 1; id <= 5; ++id) {
        std::thread t(std::bind(&run_paxos_instance, id));
        t.detach();
    }

    std::cout << "servers are started, now play with it" << std::endl;
    size_t srv_id;
    size_t round;
    int value;
    while (true) {
        std::cout << "target server to send: ";
        std::cin >> srv_id;
        if (srv_id > 5 || srv_id < 1) {
            std::cout << "invalid server id " << srv_id << ", please retry, server id must be [1,5]" << std::endl;
            continue;
        }

        std::cout << "round number: ";
        std::cin >> round;
        if (round >= 10000) {
            std::cout << "sorry, round number is restricted to [0, 10000)" << std::endl;
            continue;
        }

        std::cout << "value: ";
        std::cin >> value;
        if (value < 0) {
            std::cout << "sorry, value must be something >= 0" << std::endl;
            continue;
        }

        std::string srv_port(sstrfmt("port%d").fmt(srv_id));
        ballot_number zero(0);
        std::shared_ptr<rpc_msg> msg = std::make_shared<rpc_msg>(msg_type::try_new_ballot, 0, round, zero);
        msg->value_ = value;
        std::shared_ptr<async_result<std::shared_ptr<rpc_msg>>> handler = std::make_shared<async_result<std::shared_ptr<rpc_msg>>>();
        mbus.send_msg(srv_port, std::make_pair(msg, handler));
        std::shared_ptr<rpc_msg> resp = handler->get();
        std::this_thread::sleep_for(std::chrono::seconds(1)); 
        std::cout << "message accepted by " << resp->src_ << ", round: " << resp->round_ << ", ballot number <" << resp->ballot_number_.server_id_ << "," << resp->ballot_number_.number_ << ">" << std::endl;
    }
    return 0;
}

void run_paxos_instance(size_t srv_id) {
    if (srv_id < 1 || srv_id > 5) {
        throw std::runtime_error("srv_id out of range");
    }

    std::unordered_map<size_t, std::shared_ptr<rpc_client>> peers;
    for (size_t i = 1; i <= 5; ++i) {
        if (i != srv_id) {
            std::string port(sstrfmt("port%d").fmt(i));
            peers.insert(std::make_pair(i, std::shared_ptr<rpc_client>(new msg_bus_rpc_client(mbus, port))));
        }
    }

    std::shared_ptr<paxos_server> server(std::make_shared<paxos_server>(srv_id, peers, l));
    std::string srv_port(sstrfmt("port%d").fmt(srv_id));
    msg_bus<5>::msg_queue& q = mbus.get_queue(srv_port);
    while (true) {
        msg_bus<5>::message m = q.dequeue();
        std::shared_ptr<std::exception> except;
        std::shared_ptr<rpc_msg> result;
        try {
            result = server->process_msg(m.first);
        }
        catch (std::exception* err) {
            except.reset(err);
        }

        m.second->set_result(result, except);
    }
}