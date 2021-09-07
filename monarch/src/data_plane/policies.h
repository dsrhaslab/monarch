//
// Created by dantas on 03/06/21.
//

#ifndef THESIS_POLICIES_H
#define THESIS_POLICIES_H


enum ControlPolicy : unsigned int{
    LOCK_ORDERED = 1,
    QUEUE_ORDERED = 2,
    SINGLE_THREAD_ORDERED = 3,
    SOLO_PLACEMENT = 4
};

enum PlacementPolicy : unsigned int{
    FIRST_LEVEL_ONLY = 1,
    PUSH_DOWN = 2
};

enum ReadPolicy : unsigned int{
    WAIT_ENFORCED = 1,
    RELAXED = 2
};

enum EvictCallType : unsigned int {
    CLIENT = 1,
    HOUSEKEEPER = 2
};

#endif //THESIS_POLICIES_H
