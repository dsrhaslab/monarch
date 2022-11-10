//
// Created by dantas on 28/02/21.
//

#ifndef MONARCH_DP_INSTANCE_SESSION_H
#define MONARCH_DP_INSTANCE_SESSION_H

#include <string>

class DPInstanceSession {
    int id;
    std::string server_addr;
    //TODO stats

public:
    DPInstanceSession(int session_id, const std::string& server_addr);
    int get_id() const;
};


#endif //MONARCH_DP_INSTANCE_SESSION_H
