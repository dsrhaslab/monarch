//
// Created by dantas on 15/08/22.
//

#include "monarch.h"
#include "monarch_builder.h"
#include "../data_governance/services/file_descriptors_managers/shareable_file_descriptors_manager.h"
#include "../utils/logging/singleton_logger.h"
#include "hierarchical/storage_drivers/file_systems/posix/posix_file_system_driver.h"

MonarchBuilder Monarch::create(){
    return MonarchBuilder{};
}

Monarch::Monarch(){
    epoch_size_signal = 0;
    private_debug_enabled = true;
    debug_logger = SingletonLogger::get_instance();
    profiler = ProfilerProxy::get_instance();
}

HierarchicalStage* Monarch::get_hierarchical_stage(){
    return hierarchical_stage;
}

MetadataContainerService* Monarch::get_metadata_container_service(){
    return metadata_container;
}

Status<ssize_t> Monarch::remove(Info* i, int level){
    return hierarchical_stage->get_driver(level)->remove(i);
}

File* Monarch::remove_for_copy(Info* i, int level){
    return hierarchical_stage->get_memory_buffer_driver(level)->remove_for_copy(i);
}

inline absl::string_view Monarch::decode_filename(absl::string_view full_path){
    std::string prefix(hierarchical_stage->get_source_driver()->get_prefix());
    return absl::StripPrefix(full_path, prefix);
}

inline Status<Info*> Monarch::get_metadata(const char *pathname){
    std::string s_pathname(pathname);
    std::string decoded_path = std::string(decode_filename(s_pathname));
    if(decoded_path.length() == s_pathname.length()){
        return Status<Info*>{NOT_FOUND, nullptr};
    }
    Info* i = metadata_container->get_metadata(decoded_path);
    return Status<Info*>{i};
}

Status<Info*> Monarch::open(const char *pathname, int flags, mode_t mode, bool _64_option){
    Status<Info*> res = get_metadata(pathname);
    if(res.return_value != nullptr) {
        RequestArguments req_args{res.return_value->storage_level, true, _64_option};
        base_open(res.return_value, flags, mode, req_args);
    }
    return res;
}

inline int Monarch::base_open(Info* i, int flags, mode_t mode, const RequestArguments& req_args){
    //TODO change place
    if(i->file_descriptor_state->get_type() == FileDescriptorsStateType::SHAREABLE){
        if(req_args.client_req_){
            return ShareableFileDescriptorsManager::client_submit_open(i, req_args.level_, [this, str=i->name.c_str(), flags, mode, &req_args](){
                return open_from_storage(str, flags, mode, req_args);
            });
        }else{
            return ShareableFileDescriptorsManager::background_submit_open(i, req_args.level_, [this, str=i->name.c_str(), flags, mode, &req_args](){
                return open_from_storage(str, flags, mode, req_args);
            });
        }
    }else{
        //int fd = open_from_storage(i->name.c_str(), flags, mode, req_args);
        //i->file_descriptor_state->set_file_descriptor(fd, req_args.level_);
        //return fd;
        std::cerr << "Unsupported FileDescriptorsStateType" << std::endl;
        exit(0);
    }
}

inline int Monarch::open_from_storage(const char *pathname, int flags, mode_t mode, const RequestArguments& req_args){
    print_open(pathname, req_args);
    auto* driver = hierarchical_stage->get_driver(req_args.level_);
    if(driver->get_type() == StorageDriverType::MEMORY_BUFFER){
        return static_cast<MemoryBufferDriver*>(driver)->generate_file_descriptor();
    }else {
        if(req_args._64_option_){
            return static_cast<PosixFileSystemDriver*>(driver)->open64_(pathname, flags, mode);
        }else{
            return static_cast<PosixFileSystemDriver*>(driver)->open_(pathname, flags, mode);
        }
    }
}

Status<Info*> Monarch::open(const char *pathname, int flags, bool _64_option){
    Status<Info*> res = get_metadata(pathname);
    if(res.return_value != nullptr) {
        RequestArguments req_args{res.return_value->storage_level, true, _64_option};
        base_open(res.return_value, flags, req_args);
    }
    return res;
}

inline int Monarch::base_open(Info* i, int flags, const RequestArguments& req_args){
    if(i->file_descriptor_state->get_type() == FileDescriptorsStateType::SHAREABLE){
        if(req_args.client_req_){
            return ShareableFileDescriptorsManager::client_submit_open(i, req_args.level_, [this, str=i->name.c_str(), flags, &req_args](){
                return open_from_storage(str, flags, req_args);
            });
        }else{
            return ShareableFileDescriptorsManager::background_submit_open(i, req_args.level_, [this, str=i->name.c_str(), flags, &req_args](){
                return open_from_storage(str, flags, req_args);
            });
        }
    }else{
        //int fd = open_from_storage(i->name.c_str(), flags, req_args);
        //i->file_descriptor_state->set_file_descriptor(fd, req_args.level_);
        //return fd;
        std::cerr << "Unsupported FileDescriptorsStateType" << std::endl;
        exit(0);
    }
}

inline int Monarch::open_from_storage(const char *pathname, int flags, const RequestArguments& req_args){
    print_open(pathname, req_args);
    auto* driver = hierarchical_stage->get_driver(req_args.level_);
    if(driver->get_type() == StorageDriverType::MEMORY_BUFFER){
        return static_cast<MemoryBufferDriver*>(driver)->generate_file_descriptor();
    }else {
        if(req_args._64_option_){
            return static_cast<PosixFileSystemDriver*>(driver)->open64_(pathname, flags);
        }else{
            return static_cast<PosixFileSystemDriver*>(driver)->open_(pathname, flags);
        }
    }
}

inline void Monarch::print_open(const char *pathname, const RequestArguments& req_args){
    if(profiler->is_activated()){
        if(req_args.client_req_){
            profiler->submit_client_metadata_request("open", req_args.level_);
        }else{
            profiler->submit_background_metadata_request("open", req_args.level_);
        }
    }
    if(debug_is_activated()){
        if(req_args.client_req_){
            debug_write("Client submitted open with pathname " + std::string(pathname) + " on level " + std::to_string(req_args.level_));
        }else{
            debug_write("Background threads submitted open with pathname " + std::string(pathname) + " on level " + std::to_string(req_args.level_));
        }
    }
}

ssize_t Monarch::read(Info* i, char* result, size_t n, off_t offset, bool _64_option){
    ssize_t read_bytes;
    if(profiler->is_activated()) {
        TimeSubmission ts;
        ts.set_start();
        read_bytes = base_read(i, result, n, offset, {i->storage_level, true, _64_option});
        ts.set_end();
        profiler->submit_client_read_time(ts);
        if(metadata_container->is_train_file(i->name)) {
            check_epoch_finish(read_bytes);
        }
    }else{
        read_bytes = base_read(i, result, n, offset, {i->storage_level, true, _64_option});
    }
    return read_bytes;
}

inline void Monarch::check_epoch_finish(size_t n){
    std::unique_lock<std::mutex> ul(epoch_change_mutex);
    epoch_size_signal += n;
    if(epoch_size_signal == metadata_container->get_train_full_size()){
        profiling_service->signal_finished_epoch();
        epoch_size_signal = 0;
    }
}

inline ssize_t Monarch::base_read(Info* i, char* result, size_t n, off_t offset, const RequestArguments& req_args){
    size_t requested_size = get_request_size(i,  n, offset, req_args.level_);
    if(requested_size == 0){
        return 0;
    }
    Status<ssize_t> read_status = read_from_storage(i, result, requested_size, offset, req_args);
    //Started reading must be called after read so that the first read opens de file if fs driver.
    if(read_status.state == SUCCESS) {
        //This also sets placed_state as "started"
        if(control_handler->check_placement_validity(i)){
            auto* f = new File(i, offset, requested_size);
            if(f->is_full_read){
                memcpy(f->content, result, requested_size);
            }
            control_handler->place(f);
        }
    }
    return read_status.return_value;
}

inline size_t Monarch::get_request_size(Info* i, size_t n, off_t offset, int storage_level){
    if(offset >= i->size) {
        if(debug_is_activated()){
            print_debug_offset_overflow(i, offset, "read");
        }
        return 0;
    }
    //it's crucial that we always use the assigned storage_level "snapshot", since it can be changes in parallel
    size_t requested_size = n + offset > i->size ? i->size - offset : n;
    if(debug_is_activated()){
        print_debug_client_read(storage_level, requested_size, offset, i->name);
    }
    return requested_size;
}

void Monarch::print_debug_client_read(int storage_level, size_t requested_size, off_t offset, const std::string& name){
    debug_write("client reading from level "
                + std::to_string(storage_level)
                + " file " + name
                + " with offset: "
                + std::to_string(offset)
                + " and size: "
                + std::to_string(requested_size));
}

void Monarch::print_debug_offset_overflow(Info* i, off_t offset, const std::string& operation){
    debug_write("Tried to perform a "
                + operation
                + " from offset: "
                + std::to_string(offset)
                + " which goes beyond file size: "
                + std::to_string(i->size)
                + " name: "
                + i->name);
}

inline Status<ssize_t> Monarch::read_from_storage(Info* i, char* result, size_t n, off_t offset, const RequestArguments& req_args){
    Status<ssize_t> s;
    if(profiler->is_activated()){
        StatSubmission ss;
        ss.set_start();
        s = base_read_from_storage(i, result, n, offset, req_args);
        ss.set_end();
        ss.set_n_bytes(n);
        if(req_args.client_req_){
            profiler->submit_storage_client_read(ss, req_args.level_);
        }else{
            profiler->submit_storage_background_read(ss, req_args.level_);
        }
    }else{
        s = base_read_from_storage(i, result, n, offset, req_args);
    }
    return s;
}

inline Status<ssize_t> Monarch::base_read_from_storage(Info* i, char* result, size_t n, off_t offset, const RequestArguments& req_args){
    if(debug_is_activated()){
        if(req_args.client_req_){
            debug_write("Client submitted read for file " +  i->name + " with fd " + std::to_string(i->file_descriptor_state->get_file_descriptor(req_args.level_)) + " on level " + std::to_string(req_args.level_));
        }else{
            debug_write("Background threads submitted read for file " +  i->name + " with fd " + std::to_string(i->file_descriptor_state->get_file_descriptor(req_args.level_))  + " on level " + std::to_string(req_args.level_));
        }
    }
    Status<ssize_t> s;
    auto driver = hierarchical_stage->get_driver(req_args.level_);
    //TODO the intrusive API needs to call open + pread (we don't want an extra if here)
    if(hierarchical_stage->subtype_is(StorageDriverSubType::POSIX, req_args.level_)){
        if(req_args._64_option_){
            //TODO get fd. If shared not available we need to store the fd anyways
            s = static_cast<PosixFileSystemDriver*>(driver)->pread64_(i->file_descriptor_state->get_file_descriptor(req_args.level_), result, n, offset);
        }else{
            s = static_cast<PosixFileSystemDriver*>(driver)->pread_(i->file_descriptor_state->get_file_descriptor(req_args.level_), result, n, offset);
        }
    }else{
        s = driver->read(i, result, n, offset);
    }
    if(s.state == NOT_FOUND){
        std::cerr << "File : " << i->name << " could not be found anywhere. Requested level: " + std::to_string(req_args.level_) + ". Exiting...\n";
        exit(1);
    }
    return s;
}

void* Monarch::mmap(void* addr, size_t length, int prot, int flags, Info* i, off_t offset){
    void* res;
    if(profiler->is_activated()) {
        TimeSubmission ts;
        ts.set_start();
        res = base_mmap(addr, length, prot, flags, i, offset, i->storage_level);
        ts.set_end();
        profiler->submit_client_read_time(ts);
        if(metadata_container->is_train_file(i->name)) {
            check_epoch_finish(length);
        }
    }else{
        res = base_mmap(addr, length, prot, flags, i, offset, i->storage_level);
    }
    return res;
}

inline void* Monarch::base_mmap(void* addr, size_t length, int prot, int flags, Info* i, off_t offset, int storage_level){
    size_t requested_size = get_request_size(i, length, offset, storage_level);
    if(requested_size == 0){
        return (void *) -1;
    }
    Status<void*> read_status = nullptr;
    if(control_handler->uses_async_calls()) {
        read_status = mmap_from_storage(addr, length, prot, flags, i, offset, storage_level);
        //Started reading must be called after read so that the first read opens de file if fs driver.
        if(read_status.state == SUCCESS) {
            //This also sets placed_state as "started"
            if(control_handler->check_placement_validity(i)){
                //Since mmap does not do any I/O, Monarch brings the entire sample
                auto* f = new File(i, offset, 1);
                control_handler->place(f);
            }
        }
    }else {
        if(control_handler->check_placement_validity(i)){
            //Monarch brings the entire sample synchronously
            auto* f = new File(i, offset, 1);
            control_handler->place(f);
            //update variable with new storage
            storage_level = i->storage_level;
        }
        read_status = mmap_from_storage(addr, length, prot, flags, i, offset, storage_level);
    }
    return read_status.return_value;
}

inline Status<void*> Monarch::mmap_from_storage(void *addr, size_t length, int prot, int flags, Info* i, off_t offset, int level){
    Status<void*> s;
    if(profiler->is_activated()){
        StatSubmission ss;
        ss.set_start();
        s = base_mmap_from_storage(addr, length, prot, flags, i, offset, level);
        ss.set_end();
        ss.set_n_bytes(length);
        profiler->submit_storage_client_read(ss, level);
    }else{
        s = base_mmap_from_storage(addr, length, prot, flags, i, offset, level);
    }
    return s;
}

inline Status<void*> Monarch::base_mmap_from_storage(void *addr, size_t length, int prot, int flags, Info* i, off_t offset, int level){
    if(debug_is_activated()){
        debug_write("Client submitted mmap on file " + i->name + " with fd " + std::to_string(i->file_descriptor_state->get_file_descriptor(level)) + " on level " + std::to_string(level));
    }
    Status<void*> s;
    auto driver = hierarchical_stage->get_driver(level);
    if(hierarchical_stage->subtype_is(StorageDriverSubType::POSIX, level)){
        s = static_cast<PosixFileSystemDriver*>(driver)->mmap_(addr, length, prot, flags, i->file_descriptor_state->get_file_descriptor(level), offset);
        if(s.return_value == MAP_FAILED){
            std::cerr << "MAP_FAILED. errno: "
                + std::string(std::strerror(errno))
                + ". on file: "
                + i->name
                + " fd: " << std::endl;
        }
    }else{
        driver->read(i, (char*) addr, length, offset);
        s = {addr};
    }
    if(s.state == NOT_FOUND){
        std::cerr << "File : " << i->name << " could not be found anywhere. Requested level: " + std::to_string(level) + ". Exiting...\n";
        exit(1);
    }
    return s;
}

Status<ssize_t> Monarch::write(File* f, int level){
    Status<ssize_t> s;
    if(profiler->is_activated()){
        StatSubmission write_submission;
        write_submission.set_start();
        s = base_write(f, level);
        write_submission.set_end();
        write_submission.set_n_bytes(f->requested_size);
        profiler->submit_storage_write(write_submission, level);
    }else{
        s = base_write(f, level);
    }
    return s;
}

inline Status<ssize_t> Monarch::base_write(File* f, int level){
    if(debug_is_activated()){
        debug_write("Background threads submitted write on file " + f->info->name + " with fd " + std::to_string(f->info->file_descriptor_state->get_file_descriptor(level)) + " on level " + std::to_string(level));
    }
    if(hierarchical_stage->subtype_is(StorageDriverSubType::POSIX, level)){
        ssize_t res = hierarchical_stage->get_posix_file_system_driver(level)->pwrite_(
                f->info->file_descriptor_state->get_file_descriptor(level),
                f->content, f->requested_size, f->offset);
        delete f;
        return {res};
    }
    return hierarchical_stage->get_driver(level)->write(f);
}


int Monarch::close(Info* i){
    return base_close(i, {i->storage_level, true});
}

inline int Monarch::base_close(Info* i, const CloseRequestArguments& req_args){
    if(i->file_descriptor_state->get_type() == FileDescriptorsStateType::SHAREABLE){
        if(req_args.client_req_){
            return ShareableFileDescriptorsManager::client_submit_close(i, req_args.level_, [this, &req_args](int fd, int target_level){
                return close_from_storage(fd, target_level, req_args);
            });
        }else{
            return ShareableFileDescriptorsManager::background_submit_close(i, req_args.level_, hierarchical_stage->get_source_level(),
        [this, &req_args](int fd, int target_level){
                return close_from_storage(fd, target_level, req_args);
            });
        }
    }else{
        //level here doesn't matter
        //i->file_descriptor_state->set_file_descriptor(-1, req_args.level_);
        //return close_from_storage(i->file_descriptor_state->get_file_descriptor(req_args.level_), req_args);
        std::cerr << "Unsupported FileDescriptorsStateType" << std::endl;
        exit(0);
    }
}

//Don't use req_args.level_. Target level might be different.
inline int Monarch::close_from_storage(int fd, int target_level, const CloseRequestArguments& req_args){
    //If the driver type is a memory buffer the operation has no effect, but we still log its occurrence
    if(profiler->is_activated()){
        if(req_args.client_req_){
            profiler->submit_client_metadata_request("close", target_level);
        }else{
            profiler->submit_background_metadata_request("close", target_level);
        }
    }
    if(debug_is_activated()){
        if(req_args.client_req_){
            debug_write("Client submitted close with fd " + std::to_string(fd) + " on level " + std::to_string(target_level));
        }else{
            debug_write("Background threads submitted close with fd " + std::to_string(fd) + " on level " + std::to_string(target_level));
        }
    }
    if(hierarchical_stage->subtype_is(StorageDriverSubType::POSIX, target_level)){
        return hierarchical_stage->get_posix_file_system_driver(target_level)->close_(fd);
    }
    return 0;
}

void Monarch::init(){
    auto required_driver_control_type = control_handler->prepare_environment(this);

    switch (required_driver_control_type)
    {
        case StorageDriverStateType::BLOCKING:
        hierarchical_stage->make_blocking_driver_wrappers();
        break;
        case StorageDriverStateType::EVENTUAL:
        hierarchical_stage->make_eventual_driver_wrappers();
        break;
    default:
        break;
    }

    //TODO check if we have the new memory buffer and apply changes to the metadata
    //Changes to metadata are done here.
    metadata_container->metadata_type_.metadata_control_type = MetadataControlType::PLACED;
    metadata_container->init();

    if(debug_is_activated()) {
        debug_write("Metadata container initialized!");
    }

    auto dirs = metadata_container->get_dirs_for_environment();
    hierarchical_stage->init(dirs);

    bool becomes_full = false;
    if(hierarchical_stage->has_staging_levels()) {
        if(debug_is_activated()) {
            debug_write(std::to_string(hierarchical_stage->get_source_level()) + " staging levels specified");
        }
        if (control_handler->get_placement_policy() == PlacementPolicy::PUSH_DOWN) {
            becomes_full = metadata_container->get_full_size() > hierarchical_stage->get_full_capacity();
        } else {
            becomes_full = metadata_container->get_full_size() > hierarchical_stage->get_capacity(0);
        }
        std::string b_f = becomes_full ? "true" : "false";
        if(debug_is_activated()){
            debug_write("First storage level becomes full: " + b_f );
        }
    }else{
        if(debug_is_activated()) {
            debug_write("No staging levels found for the current configuration");
        }
    }
    start_added_services();
}

void Monarch::start_added_services(){
    if(profiling_service){
        profiling_service->start();
    }
}

void Monarch::print(){
    //TODO
}

inline void Monarch::debug_write(const std::string& msg){
    debug_logger->_write("[Monarch] " + msg);
}

inline bool Monarch::debug_is_activated(){
    return debug_logger->is_activated() && private_debug_enabled;
}

Monarch::~Monarch(){
    delete profiling_service;
}