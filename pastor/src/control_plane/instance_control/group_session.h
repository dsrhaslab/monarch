//
// Created by dantas on 27/03/21.
//

#ifndef THESIS_GROUP_SESSION_H
#define THESIS_GROUP_SESSION_H

#include <vector>
#include <tuple>
#include <grpcpp/grpcpp.h>

#include "dp_instance_session.h"

class SynchronizeCall;

//NOT thread safe. It's expected to be used by a single thread.
class GroupSession {
    int world_size;
    int number_of_workers;
    int expected_instances;
    int started_instances_count;
    int instances_in_sync;
    std::string name;
    std::vector<DPInstanceSession*> instances;

public:
    std::vector<SynchronizeCall*> delayed_responses;

    explicit GroupSession(int world_size, int number_of_workers, const std::string& name);
    //(instance_id, world_size, number_of_workers)
    std::tuple<int, int, int> register_instance(const std::string& server_addr);
    bool sync(SynchronizeCall* callback);

};


#endif //THESIS_GROUP_SESSION_H
