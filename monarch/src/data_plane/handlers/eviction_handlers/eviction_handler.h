//
// Created by dantas on 20/05/21.
//

#ifndef THESIS_EVICTION_HANDLER_H
#define THESIS_EVICTION_HANDLER_H

class HierarchicalDataPlane;
class FileInfo;
class StrictFileInfo;

class EvictionHandler {
protected:
    HierarchicalDataPlane* data_plane_;

    virtual void persist_changes(StrictFileInfo* fi, int from, int target);
    virtual void eviction(StrictFileInfo* sfi, int target);
    virtual void eviction(StrictFileInfo* sfi);

public:
    EvictionHandler(HierarchicalDataPlane* data_plane);
    virtual void evict(StrictFileInfo* sfi);
};

#endif //THESIS_EVICTION_HANDLER_H
