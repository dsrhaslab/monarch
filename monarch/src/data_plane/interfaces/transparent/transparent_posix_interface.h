//
// Created by dantas on 16/07/21.
//

#ifndef MONARCH_TRANSPARENT_POSIX_INTERFACE_H
#define MONARCH_TRANSPARENT_POSIX_INTERFACE_H

#define _GNU_SOURCE 1

#include <mutex>
#include <dlfcn.h>

#include "../../data_governance/services/transparent_metadata_container_service.h"
#include "../../parser/configuration_parser.h"
#include "../../utils/logging/singleton_logger.h"

#define CONFIGS_PATH "MONARCH_CONFIGS_PATH"

class TransparentPosixInterface {
private:
    absl::once_flag once_;
    Monarch* monarch{nullptr};
    TransparentMetadataContainerService transparent_metadata_container_service;

    bool constructor_called = false;

    Logger *debug_logger;

    void print_debug_open(int fildes, const char *pathname, bool stray, bool _64_option){
        std::string stray_str = stray ? "Stray " : "";
        std::string _64_option_str = _64_option ? "64" : "";
        debug_write(stray_str
                    + "open"
                    + _64_option_str
                    + "("
                    + std::string(pathname)
                    + ") = "
                    + std::to_string(fildes)
                    + ". PID= "
                    + std::to_string(getpid())
        );
    }

    void print_debug_pread(int fildes, size_t nbytes, off_t offset, bool stray, bool _64_option){
        std::string stray_str = stray ? "Stray " : "";
        std::string _64_option_str = _64_option ? "64" : "";
        debug_write(stray_str
                    + "pread"
                    + _64_option_str
                    + "("
                    + std::to_string(fildes)
                    + ", ..., "
                    + std::to_string(nbytes)
                    + ", "
                    + std::to_string(offset)
                    + ". PID= "
                    + std::to_string(getpid())
        );
    }

    void print_debug_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset, bool stray){
        std::string stray_str = stray ? "Stray " : "";
        std::ostringstream address;
        address << addr;
        std::string addr_name = address.str();
        debug_write(stray_str
                    + "mmap("
                    + addr_name
                    + ", "
                    + std::to_string(length)
                    + ", ..., "
                    + std::to_string(prot)
                    + ", "
                    + std::to_string(flags)
                    + ", "
                    + std::to_string(fd)
                    + ", "
                    + std::to_string(offset)
                    + "). PID= "
                    + std::to_string(getpid())
        );
    }

    void print_debug_close(int fildes, int result, bool stray){
        std::string stray_str = stray ? "Stray " : "";
        debug_write(stray_str
                    + "close("
                    + std::to_string(fildes)
                    + ") = "
                    + std::to_string(result)
                    + ". PID= "
                    + std::to_string(getpid())
        );
    }


    void debug_write(const std::string& msg){
        debug_logger->_write("[TransparentInterface] " + msg);
    }

public:
    TransparentPosixInterface(){
        monarch = ConfigurationParser::parse(std::getenv(CONFIGS_PATH));
        constructor_called = true;
        PosixLibcCaller::init();
        debug_logger = SingletonLogger::get_instance();
    }

    ~TransparentPosixInterface(){
        delete monarch;
    }

    inline int open(const char *pathname, int flags, mode_t mode){
        if(!constructor_called){
            return PosixLibcCaller::open(pathname, flags, mode);
        }
        absl::call_once(once_, [this](){
            monarch->init();
            monarch->start_added_services();
        });
        if(auto* info = monarch->open(pathname, flags, mode, false).return_value){
            int fd = info->file_descriptor_state->get_file_descriptor(info->storage_level);
            transparent_metadata_container_service.store_fildes(fd, info);
            if(debug_logger->is_activated()) {
                print_debug_open(fd, pathname, false, false);
            }
            return fd;
        }
        int fd = PosixLibcCaller::open(pathname,  flags, mode);
        if(debug_logger->is_activated()) {
            print_debug_open(fd, pathname, true, false);
        }
        return fd;
    }

    inline int open(const char *pathname, int flags){
        if(!constructor_called){
            return PosixLibcCaller::open(pathname, flags);
        }
        absl::call_once(once_, [this](){
            monarch->init();
            monarch->start_added_services();
        });
        if(auto* info = monarch->open(pathname, flags,  false).return_value){
            int fd = info->file_descriptor_state->get_file_descriptor(info->storage_level);
            transparent_metadata_container_service.store_fildes(fd, info);
            if(debug_logger->is_activated()) {
                print_debug_open(fd, pathname, false, false);
            }
            return fd;
        }
        int fd = PosixLibcCaller::open(pathname,  flags);
        if(debug_logger->is_activated()) {
            print_debug_open(fd, pathname, true, false);
        }
        return fd;
    }

    inline int open64(const char *pathname, int flags, mode_t mode){
        if(!constructor_called){
            return PosixLibcCaller::open(pathname, flags, mode);
        }
        absl::call_once(once_, [this](){
            monarch->init();
            monarch->start_added_services();
        });
        if(auto* info = monarch->open(pathname, flags, mode, true).return_value){
            int fd = info->file_descriptor_state->get_file_descriptor(info->storage_level);
            transparent_metadata_container_service.store_fildes(fd, info);
            if(debug_logger->is_activated()) {
                print_debug_open(fd, pathname, false, true);
            }
            return fd;
        }
        int fd = PosixLibcCaller::open(pathname,  flags, mode);
        if(debug_logger->is_activated()) {
            print_debug_open(fd, pathname, true, true);
        }
        return fd;
    }

    inline int open64(const char *pathname, int flags){
        if(!constructor_called){
            return PosixLibcCaller::open(pathname, flags);
        }
        absl::call_once(once_, [this](){
            monarch->init();
            monarch->start_added_services();
        });
        if(auto* info = monarch->open(pathname, flags, true).return_value){
            int fd = info->file_descriptor_state->get_file_descriptor(info->storage_level);
            transparent_metadata_container_service.store_fildes(fd, info);
            if(debug_logger->is_activated()) {
                print_debug_open(fd, pathname, false, true);
            }
            return fd;
        }
        int fd = PosixLibcCaller::open(pathname,  flags, true);
        if(debug_logger->is_activated()) {
            print_debug_open(fd, pathname, true, true);
        }
        return fd;
    }

    inline ssize_t pread(int fildes, void *result, size_t nbyte, off_t offset){
        if(!constructor_called ){
            return PosixLibcCaller::pread(fildes, result, nbyte, offset);
        }
        if(auto* info = transparent_metadata_container_service.get_metadata(fildes)){
            if(debug_logger->is_activated()){
                print_debug_pread(fildes, nbyte, offset, false, false);
            }
            return monarch->read(info, static_cast<char *>(result), nbyte, offset, false);
        }
        if(debug_logger->is_activated()){
            print_debug_pread(fildes, nbyte, offset, true, false);
        }
        return PosixLibcCaller::pread(fildes, result, nbyte, offset);
    }

    inline ssize_t pread64(int fildes, void *result, size_t nbyte, off_t offset){
        if(!constructor_called ){
            return PosixLibcCaller::pread64(fildes, result, nbyte, offset);
        }
        if(auto* info = transparent_metadata_container_service.get_metadata(fildes)){
            if(debug_logger->is_activated()){
                print_debug_pread(fildes, nbyte, offset, false, true);
            }
            return monarch->read(info, static_cast<char *>(result), nbyte, offset, true);
        }
        if(debug_logger->is_activated()){
            print_debug_pread(fildes, nbyte, offset, true, true);
        }
        return PosixLibcCaller::pread(fildes, result, nbyte, offset);
    }

    inline void *mmap (void *addr, size_t length, int prot, int flags, int fd, off_t offset){
        if(!constructor_called){
            return PosixLibcCaller::mmap(addr, length, prot, flags, fd, offset);
        }
        if(auto* info = transparent_metadata_container_service.get_metadata(fd)){
            if(debug_logger->is_activated()){
                print_debug_mmap(addr, length, prot, flags, fd, offset, false);
            }
            return monarch->mmap(addr, length, prot, flags, info, offset);
        }
        if(debug_logger->is_activated()){
            print_debug_mmap(addr, length, prot, flags, fd, offset, true);
        }
        return PosixLibcCaller::mmap(addr, length, prot, flags, fd, offset);
    }

    inline int close(int fildes){
        if(!constructor_called && transparent_metadata_container_service.get_metadata(fildes) == nullptr){
            return PosixLibcCaller::close(fildes);
        }
        if(auto* info = transparent_metadata_container_service.get_metadata(fildes)) {
            int res = monarch->close(info);
            transparent_metadata_container_service.remove_fildes(fildes);
            if(debug_logger->is_activated()) {
                print_debug_close(fildes, res, false);
            }
            return res;
        }
        int res = PosixLibcCaller::close(fildes);
        if(debug_logger->is_activated()) {
            print_debug_close(fildes, res, true);
        }
        return res;
    }
};

#endif //MONARCH_TRANSPARENT_POSIX_INTERFACE_H