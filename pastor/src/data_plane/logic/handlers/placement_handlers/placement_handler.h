//
// Created by dantas on 20/05/21.
//

#ifndef THESIS_PLACEMENT_HANDLER_H
#define THESIS_PLACEMENT_HANDLER_H

#include "../../policies.h"
#include "unistd.h"
#include "../../root/storage_drivers/utils/status.h"

class HierarchicalDataPlane;
class File;
class PlacedFileInfo;
class PrefetchedFile;

class PlacementHandler {
protected:
    HierarchicalDataPlane* data_plane_;
    PlacementPolicy placement_policy_;
    bool async_placement_;

    virtual Status<ssize_t> target_placement_logic(File* f, int target_level);
    virtual Status<ssize_t> targeted_placement(File* f, int target_level, bool with_alloc);
    virtual Status<ssize_t> push_down_placement(File* f);
    virtual Status<ssize_t> base_placement(File* f);
    //virtual void placement_callback() = 0;

public:
    PlacementHandler(HierarchicalDataPlane* data_plane, PlacementPolicy placement_policy, bool async_placement);
    virtual Status<ssize_t> place(File* f);
    virtual Status<ssize_t> place(PrefetchedFile* pf);
    virtual bool check_placement_validity();
    virtual bool check_placement_validity(PlacedFileInfo* f);
};

#endif //THESIS_PLACEMENT_HANDLER_H
