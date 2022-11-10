//
// Created by dantas on 19/10/20.
//

#ifndef READ_CONTROLLER_H
#define READ_CONTROLLER_H

#include <unistd.h>
#include <string>
#include <tuple>

#include "absl/strings/string_view.h"

class CollectedStats;

class Stage {
public:
    virtual void print() = 0;
};

#endif // READ_CONTROLLER_H
