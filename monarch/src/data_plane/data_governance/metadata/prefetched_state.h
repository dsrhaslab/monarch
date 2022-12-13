//
// Created by dantas on 05/09/22.
//

#ifndef MONARCH_PREFETCHED_STATE_H
#define MONARCH_PREFETCHED_STATE_H

class PrefetchedState {
    int request_id;
    bool placeholder;

public:
    PrefetchedState(int id) {
        request_id = id;
        placeholder = false;
    }   

    int get_request_id() const {
        return request_id;
    }

    void set_as_placeholder() {
        placeholder = true;
    }

    bool is_placeholder() {
        return placeholder;
    }
};

#endif //MONARCH_PREFETCHED_STATE_H
