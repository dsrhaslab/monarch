//
// Created by dantas on 19/10/20.
//

#ifndef DATA_STORAGE_DRIVER_H
#define DATA_STORAGE_DRIVER_H

#include <iostream>
#include <unistd.h>
#include <string>
#include <mutex>
#include <functional>
#include <vector>
#include <condition_variable>
#include "../../metadata/file_info.h"
#include "../../metadata/file.h"
#include "utils/status.h"

class DataStorageDriverBuilder;
enum DriverType : unsigned int;

class DataStorageDriver {
public:
    virtual Status<ssize_t> read(File* f) = 0;
    virtual Status<ssize_t> read(FileInfo* fi, char* result, uint64_t offset, size_t n, bool _64_option) = 0;
    virtual Status<ssize_t> write(File* f) = 0;
    virtual Status<ssize_t> remove(FileInfo* fi) = 0;
    virtual File* remove_for_copy(FileInfo* fi) = 0;
    virtual bool alloc_type() = 0;
    virtual bool in_memory_type() = 0;
    virtual bool file_system_type() = 0;
    virtual void create_environment(std::vector<std::string>& dirs, bool enable_write) = 0;
    virtual std::vector<std::string> configs() = 0;
    virtual std::string prefix() = 0;
    static DataStorageDriverBuilder create(DriverType type);
};

class BaseDataStorageDriver : public DataStorageDriver {
protected:
    int level;
    size_t block_size = INT32_MAX;
    std::string storage_prefix;

public:
    friend class DataStorageDriverBuilder;
    virtual ssize_t sizeof_content(FileInfo* fi) = 0;
    bool alloc_type() override;
    std::vector<std::string> configs() override;
    std::string prefix() override;
};

class AllocableDataStorageDriver : public DataStorageDriver {
protected:
    BaseDataStorageDriver* base_storage_driver;
    size_t current_size = 0;
    size_t max_storage_size;
    float max_storage_occupation_threshold = 0.8;

public:
    friend class DataStorageDriverBuilder;
    AllocableDataStorageDriver(BaseDataStorageDriver* base_storage_driver);
    AllocableDataStorageDriver(AllocableDataStorageDriver* to_copy);

    virtual Status<ssize_t> allocate_storage(FileInfo* fi);
    virtual Status<ssize_t> conditional_allocate_storage(FileInfo* fi);
    virtual Status<ssize_t> free_storage(FileInfo* fi);
    virtual size_t resize(size_t new_size);
    virtual size_t current_storage_size();
    [[nodiscard]] size_t get_max_storage_size() const;
    Status<ssize_t> read(FileInfo* fi, char* result, uint64_t offset, size_t n, bool _64_option) override;
    Status<ssize_t> read(File* f) override;
    Status<ssize_t> write(File* f) override;
    Status<ssize_t> remove(FileInfo* fi) override;
    File* remove_for_copy(FileInfo* fi) override;
    virtual bool becomesFull(FileInfo* fi);
    bool occupation_threshold_reached() const;
    ssize_t sizeof_content(FileInfo* fi);
    bool alloc_type() override {return true;};
    bool in_memory_type() override;
    bool file_system_type() override;
    virtual bool blocking_type() {return false;};
    virtual bool eventual_type() {return false;};
    std::vector<std::string> configs() override;
    void create_environment(std::vector<std::string>& dirs, bool enable_write) override;
    std::string prefix() override;
    BaseDataStorageDriver* get_base_storage_driver();
};


class BlockingAllocableDataStorageDriver : public AllocableDataStorageDriver {
    std::unique_ptr<std::mutex> mutex;
    std::unique_ptr<std::condition_variable> is_full;
    int waiting;

public:
    BlockingAllocableDataStorageDriver(BaseDataStorageDriver *baseStorageDriver);
    BlockingAllocableDataStorageDriver(AllocableDataStorageDriver* base);
    const std::unique_ptr<std::mutex> &get_mutex() const;
    const std::unique_ptr<std::condition_variable> &get_cond_var() const;
    bool blocking_type() override;
    Status<ssize_t> allocate_storage(FileInfo* fi) override;
    Status<ssize_t> conditional_allocate_storage(FileInfo* fi) override;
    Status<ssize_t> free_storage(FileInfo* fi) override;
    size_t resize(size_t new_size) override;
    [[nodiscard]] size_t current_storage_size();
    bool becomesFull(FileInfo* fi);
    std::vector<std::string> configs() override;
};

class EventualAllocableDataStorageDriver : public AllocableDataStorageDriver {
    size_t placeholder_current_size = 0;

public:
    EventualAllocableDataStorageDriver(AllocableDataStorageDriver* base) : AllocableDataStorageDriver(base){}
    bool eventual_type() override;
    Status<ssize_t> eventual_allocate_storage(FileInfo* fi);
    Status<ssize_t> eventual_free_storage(FileInfo* fi);
    [[nodiscard]] size_t placeholder_current_storage_size();
    bool eventual_becomesFull(FileInfo* fi);
    std::vector<std::string> configs() override;
};

//TODO batch write_operation driver. Stores write and remove in a queue to execute them in a batch like manner
#endif //DATA_STORAGE_DRIVER_H