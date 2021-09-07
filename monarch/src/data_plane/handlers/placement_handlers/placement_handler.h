//
// Created by dantas on 20/05/21.
//

#ifndef THESIS_PLACEMENT_HANDLER_H
#define THESIS_PLACEMENT_HANDLER_H

#include "../../policies.h"

class HierarchicalDataPlane;
class File;
class PlacedFileInfo;
class PrefetchedFile;
class Status;

class PlacementHandler {
protected:
    HierarchicalDataPlane* data_plane_;
    PlacementPolicy placement_policy_;

    virtual Status targeted_placement(File* f, int target_level, bool with_alloc);
    virtual Status push_down_placement(File* f);
    //virtual void placement_callback() = 0;

public:
    PlacementHandler(HierarchicalDataPlane* data_plane, PlacementPolicy placement_policy);
    virtual Status place(File* f);
    virtual Status place(PrefetchedFile* pf);
    virtual bool check_placement_validity();
    virtual bool check_placement_validity(PlacedFileInfo* f);
};

#endif //THESIS_PLACEMENT_HANDLER_H
