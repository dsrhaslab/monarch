//
// Created by dantas on 07/03/21.
//

#ifndef THESIS_PREFETCHED_FILE_H
#define THESIS_PREFETCHED_FILE_H

#include "file.h"
#include "strict_file_info.h"

class PrefetchedFile : public File {
    int request_id;
    bool placeholder;

public:
    PrefetchedFile(StrictFileInfo* file_info, int id);
    PrefetchedFile(File* f, int id) : File(f), request_id(id) {};
    int get_request_id() const;
    void set_as_placeholder();
    bool is_placeholder();
    StrictFileInfo* get_info() override;

};


#endif //THESIS_PREFETCHED_FILE_H
