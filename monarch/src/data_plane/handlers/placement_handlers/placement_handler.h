//
// Created by dantas on 20/05/21.
//

#ifndef MONARCH_PLACEMENT_HANDLER_H
#define MONARCH_PLACEMENT_HANDLER_H

#include "unistd.h"
#include "../../stages/monarch.h"

class HierarchicalStage;
class File;
class PlacedFileInfo;
class PrefetchedFile;

class PlacementHandler {
protected:
    Monarch* monarch_;
    HierarchicalStage* hierarchical_stage;
    PlacementPolicy placement_policy_;
    bool async_placement_;
    bool reached_stability_;

    ProfilerProxy* profiler;
    Logger* debug_logger;

    virtual Status<ssize_t> target_placement_logic(File* f, int target_level);
    virtual Status<ssize_t> targeted_placement(File* f, int target_level, bool with_alloc);
    virtual Status<ssize_t> push_down_placement(File* f);
    virtual Status<ssize_t> base_placement(File* f);

    void debug_write(const std::string& msg);

public:
    PlacementHandler(Monarch* monarch, PlacementPolicy placement_policy, bool async_placement);
    virtual Status<ssize_t> place(File* f);
    virtual bool check_placement_validity();
    virtual bool check_placement_validity(Info* i);
};

#endif //MONARCH_PLACEMENT_HANDLER_H
