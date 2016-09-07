/**
 * This project is used to demostrate how classic Paxos works, no for production. 
 * However, you can leverage the source code or the idea in the project under the
 * license of Apache 2
 **/
#ifndef _CPPAXOS_PAXOS_SRV_HXX_
#define _CPPAXOS_PAXOS_SRV_HXX_

class paxos_server{
public:
    paxos_server(const size_t srv_id, const std::unordered_map<size_t, std::shared_ptr<rpc_client>>& peers, const std::shared_ptr<logger>& l)
        : server_id_(srv_id), outcomes_(), prev_values_(), last_tried_(), prev_ballots_(), next_ballots_(), status_(), prev_votes_(), quorum_(), voters_(), values_(), client_values_(), locks_(), logger_(l), peers_(peers), new_round_lock_(){}
    __nocopy__(paxos_server)
public:
    std::shared_ptr<rpc_msg> process_msg(const std::shared_ptr<rpc_msg>& msg);
    void handle_response(std::shared_ptr<rpc_msg>& msg, std::shared_ptr<std::exception>& err);
private:
    std::shared_ptr<rpc_msg> try_new_ballot(const std::shared_ptr<rpc_msg>& msg);
    std::shared_ptr<rpc_msg> next_ballot(const std::shared_ptr<rpc_msg>& msg);
    std::shared_ptr<rpc_msg> last_vote(const std::shared_ptr<rpc_msg>& msg);
    std::shared_ptr<rpc_msg> begin_ballot(const std::shared_ptr<rpc_msg>& msg);
    std::shared_ptr<rpc_msg> voted(const std::shared_ptr<rpc_msg>& msg);
    std::shared_ptr<rpc_msg> success(const std::shared_ptr<rpc_msg>& msg);
    size_t ensure_round_ready(size_t round, int client_value = -1);
private:
    size_t server_id_;
    std::vector<int> outcomes_;
    std::vector<int> prev_values_;
    std::vector<ballot_number> last_tried_;
    std::vector<ballot_number> prev_ballots_;
    std::vector<ballot_number> next_ballots_;
    std::vector<status> status_; // having status as a list to allow multiple rounds to be on going at the same time
    std::vector<std::shared_ptr<std::vector<std::shared_ptr<vote>>>> prev_votes_;
    std::vector<std::shared_ptr<std::vector<size_t>>> quorum_;
    std::vector<std::shared_ptr<std::vector<size_t>>> voters_;
    std::vector<int> values_;
    std::vector<int> client_values_;
    std::vector<std::shared_ptr<std::recursive_mutex>> locks_;
    std::shared_ptr<logger> logger_;
    std::unordered_map<size_t, std::shared_ptr<rpc_client>> peers_;
    std::recursive_mutex new_round_lock_;
    
private:
    typedef std::unordered_map<size_t, std::shared_ptr<rpc_client>>::iterator peer_itor;
};

#endif //_CPPAXOS_PAXOS_SRV_HXX_