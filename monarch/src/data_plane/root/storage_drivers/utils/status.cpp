//
// Created by dantas on 24/10/20.
//

#include "status.h"

Status::Status(State s) {
    state = s;
    add_state = NILL;
    bytes_size = 0;
}

Status::Status(State s, State as, ssize_t b_size) {
    state = s;
    add_state = as;
    bytes_size = b_size;
}

Status::Status(ssize_t b_size) {
    state = SUCCESS;
    add_state = NILL;
    bytes_size = b_size;
}


