//
// Created by dantas on 28/02/21.
//

#ifndef THESIS_DP_INSTANCE_SESSION_H
#define THESIS_DP_INSTANCE_SESSION_H

#include <string>

class DPInstanceSession {
    int id;
    std::string server_addr;
    //TODO stats

public:
    DPInstanceSession(int session_id, const std::string& server_addr);
    int get_id() const;
};


#endif //THESIS_DP_INSTANCE_SESSION_H
