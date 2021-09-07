//
// Created by dantas on 24/10/20.
//

#ifndef THESIS_STATUS_H
#define THESIS_STATUS_H

#include <stdio.h>

enum State {
    NILL = 0x00,
    SUCCESS = 0x01,
    NOT_FOUND = 0x02,
    STORAGE_FULL = 0x03,
    OCCUPATION_THRESHOLD_REACHED = 0X04,
    DELAYED = 0X05,
    MISS = 0x06
};

class Status {
public:
    Status(State s, State as, ssize_t b_size);
    Status(ssize_t b_size);
    Status(State s);
    State state;
    State add_state;
    ssize_t bytes_size;
};

#endif //THESIS_STATUS_H
