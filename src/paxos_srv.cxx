/**
* This project is used to demostrate how classic Paxos works, no for production.
* However, you can leverage the source code or the idea in the project under the
* license of Apache 2
**/

#include "../include/cppaxos.hxx"

#define LOCK_AND_LOG(msg, val) \
    recur_lock(*locks_[round]);\
    logger_->log(log_level::debug, lstrfmt("%zd:\tprocess %s message from server %d on server %d with value %d for round %d").fmt(server_id_, msg_type_str[(msg)->type_], (msg)->src_, server_id_, (val), round))
#define LOG_BALLOT_NUMBER(remote, local) \
    logger_->log(log_level::debug, lstrfmt("%zd:\tballot <%d, %d> v.s. local <%d, %d>").fmt(server_id_, (remote).server_id_, (remote).number_, (local).server_id_, (local).number_))

#define LOG_NIL_MSG(msg) logger_->log(log_level::debug, lstrfmt("%zd:\treceive a null msg from server %d").fmt(server_id_, (msg)->src_))

#define LOG_STATUS(s) logger_->log(log_level::debug, sstrfmt("%zd:\tcurrent server status %s").fmt(server_id_, status_str[(s)]))

// for printing
char* msg_type_str[] = {
    "nil",
    "try_new_ballot",
    "next_ballot",
    "last_vote",
    "begin_ballot",
    "voted",
    "success"
};

char* status_str[] = {
    "idle",
    "trying",
    "polling"
};

std::shared_ptr<rpc_msg> paxos_server::process_msg(const std::shared_ptr<rpc_msg>& msg){
    switch(msg->type_){
    case msg_type::try_new_ballot:
        return this->try_new_ballot(msg);
    case msg_type::next_ballot:
        return next_ballot(msg);
    case msg_type::last_vote:
        return last_vote(msg);
    case msg_type::begin_ballot:
        return begin_ballot(msg);
    case msg_type::voted:
        return voted(msg);
    case msg_type::success:
        return success(msg);
    case msg_type::nil:
        LOG_NIL_MSG(msg);
        return std::shared_ptr<rpc_msg>();
    default:
        throw std::runtime_error("undefined rpc message type in paxos_server::process_msg");
    }
}

size_t paxos_server::ensure_round_ready(size_t round, int client_value) {
    recur_lock(new_round_lock_);
    while (round >= locks_.size()) {
        outcomes_.push_back(-1);
        prev_values_.push_back(-1);
        ballot_number zero(server_id_);
        last_tried_.push_back(zero);
        prev_ballots_.push_back(zero);
        next_ballots_.push_back(zero);
        status_.push_back(status::idle);
        prev_votes_.push_back(std::make_shared<std::vector<std::shared_ptr<vote>>>());
        quorum_.push_back(std::make_shared<std::vector<size_t>>());
        voters_.push_back(std::make_shared<std::vector<size_t>>());
        values_.push_back(-1);
        locks_.push_back(std::make_shared<std::recursive_mutex>());
        client_values_.push_back(-1); // this is to save the value that client requests, so when paxos could choose random value, this value will be used
    }

    if (client_value > -1) {
        client_values_[round] = client_value;
    }

    return round;
}

std::shared_ptr<rpc_msg> paxos_server::try_new_ballot(const std::shared_ptr<rpc_msg>& msg) {
    size_t round(ensure_round_ready(msg->round_, msg->value_));
    LOCK_AND_LOG(msg, msg->value_);
    last_tried_[round] = prev_ballots_[round].increase(server_id_);
    status_[round] = status::trying;
    prev_votes_[round]->clear();

    std::shared_ptr<rpc_msg> nxt_ballot_req(std::make_shared<rpc_msg>(msg_type::next_ballot, server_id_, round, last_tried_[round]));
    rpc_handler handler = std::bind(&paxos_server::handle_response, this, std::placeholders::_1, std::placeholders::_2);
    for (peer_itor it = peers_.begin(); it != peers_.end(); ++it) {
        it->second->send(nxt_ballot_req, handler);
    }

    // this is to demostrate how paxos works, so we just strictly follow the spec by lamport's original spec
    // in the spec, the next_ballot should also send to self, and handle the result with last_vote(msg)
    std::shared_ptr<rpc_msg> self_result = next_ballot(nxt_ballot_req);
    process_msg(self_result);

    return std::make_shared<rpc_msg>(msg_type::try_new_ballot, server_id_, round, last_tried_[round]);
}

std::shared_ptr<rpc_msg> paxos_server::next_ballot(const std::shared_ptr<rpc_msg>& msg) {
    size_t round(ensure_round_ready(msg->round_));
    LOCK_AND_LOG(msg, -1);
    LOG_BALLOT_NUMBER(msg->ballot_number_, next_ballots_[round]);
    if (msg->ballot_number_ >= next_ballots_[round]) {
        next_ballots_[round] = msg->ballot_number_;
    }

    if (next_ballots_[round] > prev_ballots_[round]) {
        std::shared_ptr<rpc_msg> ret(std::make_shared<rpc_msg>(msg_type::last_vote, server_id_, round, next_ballots_[round]));
        ret->vote_ = std::make_shared<vote>(server_id_, prev_ballots_[round], prev_values_[round]);
        return ret;
    }

    return std::make_shared<rpc_msg>(msg_type::nil, server_id_, round, msg->ballot_number_);
}

std::shared_ptr<rpc_msg> paxos_server::last_vote(const std::shared_ptr<rpc_msg>& msg) {
    size_t round(ensure_round_ready(msg->round_));
    LOCK_AND_LOG(msg, msg->vote_->value_);
    LOG_BALLOT_NUMBER(msg->ballot_number_, last_tried_[round]);
    LOG_STATUS(status_[round]);
    if (msg->ballot_number_ == last_tried_[round] && status_[round] == status::trying) {
        prev_votes_[round]->push_back(msg->vote_);
    }

    if (status_[round] == status::trying && prev_votes_[round]->size() >= (1 + peers_.size() / 2)) {
        logger_->log(log_level::debug, lstrfmt("%zd:\twe got a quorum(%d) for round %d(%d)").fmt(server_id_, prev_votes_[round]->size(), round, peers_.size() + 1));
        status_[round] = status::polling;
        quorum_[round]->clear();
        int value = -1;
        ballot_number ballot(server_id_);
        for (std::vector<std::shared_ptr<vote>>::const_iterator itor = prev_votes_[round]->begin(); itor != prev_votes_[round]->end(); ++itor) {
            quorum_[round]->push_back((*itor)->server_id_);
            if ((*itor)->ballot_number_ > ballot) {
                ballot = (*itor)->ballot_number_;
                value = (*itor)->value_;
            }
        }

        voters_[round]->clear();
        values_[round] = value >= 0 ? value : client_values_[round]; 

        // begin ballot
        std::shared_ptr<rpc_msg> begin_ballot_req(std::make_shared<rpc_msg>(msg_type::begin_ballot, server_id_, round, last_tried_[round]));
        begin_ballot_req->value_ = values_[round];
        rpc_handler handler = std::bind(&paxos_server::handle_response, this, std::placeholders::_1, std::placeholders::_2);
        for (std::vector<size_t>::const_iterator it = quorum_[round]->begin(); it != quorum_[round]->end(); ++it) {
            peer_itor pit = peers_.find(*it);
            if (pit != peers_.end()) {
                // a remote peer
                pit->second->send(begin_ballot_req, handler);
            }
        }

        // self processing as well
        std::shared_ptr<rpc_msg> self_result = begin_ballot(begin_ballot_req);
        process_msg(self_result);
    }

    return std::make_shared<rpc_msg>(msg_type::nil, server_id_, round, msg->ballot_number_);
}

std::shared_ptr<rpc_msg> paxos_server::begin_ballot(const std::shared_ptr<rpc_msg>& msg) {
    size_t round(ensure_round_ready(msg->round_));
    LOCK_AND_LOG(msg, msg->value_);
    LOG_BALLOT_NUMBER(msg->ballot_number_, next_ballots_[round]);
    LOG_BALLOT_NUMBER(msg->ballot_number_, prev_ballots_[round]);
    if (msg->ballot_number_ == next_ballots_[round] && msg->ballot_number_ > prev_ballots_[round]) {
        prev_ballots_[round] = msg->ballot_number_;
        prev_values_[round] = msg->value_;
        return std::make_shared<rpc_msg>(msg_type::voted, server_id_, round, prev_ballots_[round]);
    }

    return std::make_shared<rpc_msg>(msg_type::nil, server_id_, round, msg->ballot_number_);
}

std::shared_ptr<rpc_msg> paxos_server::voted(const std::shared_ptr<rpc_msg>& msg) {
    size_t round(ensure_round_ready(msg->round_));
    LOCK_AND_LOG(msg, -1);
    LOG_BALLOT_NUMBER(msg->ballot_number_, last_tried_[round]);
    LOG_STATUS(status_[round]);
    if (msg->ballot_number_ == last_tried_[round] && status_[round] == status::polling) {
        voters_[round]->push_back(msg->src_);
    }

    logger_->log(log_level::debug, lstrfmt("%zd\tvoter size %d v.s. quorum size %d, outcome: %d").fmt(server_id_, voters_[round]->size(), quorum_[round]->size(), outcomes_[round]));
    if (voters_[round]->size() == quorum_[round]->size() && outcomes_[round] == -1) {
        logger_->log(log_level::debug, lstrfmt("%zd\tround %d passed the voting").fmt(server_id_, round));
        outcomes_[round] = values_[round];
        std::shared_ptr<rpc_msg> succeed(std::make_shared<rpc_msg>(msg_type::success, server_id_, round, last_tried_[round]));
        succeed->value_ = outcomes_[round];
        rpc_handler handler = std::bind(&paxos_server::handle_response, this, std::placeholders::_1, std::placeholders::_2);
        for (peer_itor it = peers_.begin(); it != peers_.end(); ++it) {
            it->second->send(succeed, handler);
        }

        // also send to self
        success(succeed);
    }

    return std::make_shared<rpc_msg>(msg_type::nil, server_id_, round, msg->ballot_number_);
}

std::shared_ptr<rpc_msg> paxos_server::success(const std::shared_ptr<rpc_msg>& msg) {
    size_t round(ensure_round_ready(msg->round_));
    LOCK_AND_LOG(msg, msg->value_);
    LOG_BALLOT_NUMBER(msg->ballot_number_, last_tried_[round]);
    logger_->log(log_level::info, sstrfmt("%zd\tcurrent outcome for round %d is %d").fmt(server_id_, round, outcomes_[round]));
    if (outcomes_[round] == -1) {
        outcomes_[round] = msg->value_;
        logger_->log(log_level::info, sstrfmt("%zd\toutcome for round %d is updated to %d").fmt(server_id_, round, outcomes_[round]));
    }

    return std::make_shared<rpc_msg>(msg_type::nil, server_id_, round, msg->ballot_number_);
}

void paxos_server::handle_response(std::shared_ptr<rpc_msg>& msg, std::shared_ptr<std::exception>& err) {
    if (err) {
        logger_->log(log_level::warn, lstrfmt("%zd\treceive an error from peer '%s'").fmt(server_id_, err->what()));
    }
    else {
        process_msg(msg);
    }
}