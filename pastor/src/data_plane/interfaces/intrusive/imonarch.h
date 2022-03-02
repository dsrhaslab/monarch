//
// Created by dantas on 27/11/20.
//

#ifndef THESIS_IMONARCH_H
#define THESIS_IMONARCH_H

#include <mutex>
#include "../../logic/data_plane.h"
#if defined(INCLUDE_GRPC)
#include "../../logic/remote/remote_stage_builder.h"
#endif
#include "absl/base/call_once.h"

//https://stackoverflow.com/questions/19133552/crop-an-image-from-a-byte-array
//https://stackoverflow.com/questions/63880081/how-to-convert-a-torch-tensor-into-a-byte-string
//https://stackoverflow.com/questions/54137790/convert-from-io-bytesio-to-a-bytes-like-object-in-python3-6

//Not trully singleton
class IMonarch {
private:
    int rank_;
    int worker_id_;
    DataPlane* data_plane;

    #if defined(INCLUDE_GRPC)
    RemoteStageBuilder* remote_handler;
    #endif

    static absl::once_flag once_;
    static IMonarch* instance;

public:
    IMonarch(IMonarch &dpi) = delete;
    void operator=(const IMonarch &) = delete;
    static IMonarch *get_instance();

    ~IMonarch();
    IMonarch();
    IMonarch(int rank, int worker_id);

    #if defined(INCLUDE_GRPC)
    static IMonarch *get_instance(const std::string &c_server_addr);
    static IMonarch *get_instance(int rank, const std::string &group, const std::string &c_server_addr, const std::string &dp_server_addr);
    void server_attach_init(const std::string &group, const std::string &c_server_addr, const std::string &dp_server_addr);
    #endif

    void standalone_init();
    absl::string_view decode_filename(absl::string_view full_path);
    ssize_t read(const std::string &filename, char* result, uint64_t offset, size_t n);
    size_t get_file_size(const std::string &filename);
    size_t get_file_size_from_id(int id);
    int get_target_class(const std::string &filename);
    ssize_t read_from_id(int file_id, char* result, uint64_t offset, size_t n);
    ssize_t read_from_id(int file_id, char* result);
    int get_target_class_from_id(int id);
    void start();
    int get_file_count();
};

#endif //THESIS_IMONARCH_H
