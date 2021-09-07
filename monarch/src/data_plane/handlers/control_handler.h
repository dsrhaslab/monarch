//
// Created by dantas on 20/05/21.
//

#ifndef THESIS_CONTROL_HANDLER_H
#define THESIS_CONTROL_HANDLER_H

#include "../policies.h"

class PlacementHandler;
class EvictionHandler;
class HierarchicalDataPlane;
class Status;
class PrefetchedFile;
class StrictFileInfo;
class File;
class PlacedFileInfo;

class ControlHandler {
protected:
    HierarchicalDataPlane* data_plane_;
    ControlPolicy control_policy;
    PlacementPolicy placement_policy;
    PlacementHandler* placement_handler;
    EvictionHandler* eviction_handler;
private:
    void base_evict(StrictFileInfo* mfi);
public:
    explicit ControlHandler(HierarchicalDataPlane* data_plane, ControlPolicy cp, PlacementPolicy pp);
    virtual Status place(File* f);
    virtual Status place(PrefetchedFile* pf);
    virtual bool check_placement_validity();
    virtual bool check_placement_validity(PlacedFileInfo* f);
    virtual void evict(StrictFileInfo* mfi);
    virtual void prepare_environment();
    void change_policies(ControlPolicy cp, PlacementPolicy pp);
};

#endif //THESIS_CONTROL_HANDLER_H
