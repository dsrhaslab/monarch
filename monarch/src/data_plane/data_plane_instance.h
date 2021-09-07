//
// Created by dantas on 27/11/20.
//

#ifndef THESIS_DATA_PLANE_INSTANCE_H
#define THESIS_DATA_PLANE_INSTANCE_H

#include <mutex>
#include "data_plane.h"
#include "remote/remote_stage_builder.h"
#include "absl/base/call_once.h"

//https://stackoverflow.com/questions/19133552/crop-an-image-from-a-byte-array
//https://stackoverflow.com/questions/63880081/how-to-convert-a-torch-tensor-into-a-byte-string
//https://stackoverflow.com/questions/54137790/convert-from-io-bytesio-to-a-bytes-like-object-in-python3-6

//Not trully singleton
class DataPlaneInstance {
private:
    int rank_;
    int worker_id_;
    DataPlane* data_plane;
    RemoteStageBuilder* remote_handler;

    static absl::once_flag once_;
    static DataPlaneInstance* instance;

public:
    DataPlaneInstance(DataPlaneInstance &dpi) = delete;
    void operator=(const DataPlaneInstance &) = delete;
    static DataPlaneInstance *get_instance(const std::string &c_server_addr);
    static DataPlaneInstance *get_instance(int rank, const std::string &group, const std::string &c_server_addr, const std::string &dp_server_addr);

    DataPlaneInstance();
    DataPlaneInstance(int rank, int worker_id);
    void bind(const std::string &group, const std::string &c_server_addr, const std::string &dp_server_addr);
    std::string decode_filename(std::string full_path);
    ssize_t read(const std::string &filename, char* result, uint64_t offset, size_t n);
    size_t get_file_size(const std::string &filename);
    size_t get_file_size_from_id(int id);
    int get_target_class(const std::string &filename);
    ssize_t read_from_id(int file_id, char* result, uint64_t offset, size_t n);
    ssize_t read_from_id(int file_id, char* result);
    int get_target_class_from_id(int id);
    void start();
    int get_file_count();
    CollectedStats* collect_statistics();
};

#endif //THESIS_DATA_PLANE_INSTANCE_H
