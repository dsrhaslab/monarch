//
// Created by dantas on 16/07/21.
//

#ifndef THESIS_TMONARCH_H
#define THESIS_TMONARCH_H

#define _GNU_SOURCE 1

#include <mutex>
#include <dlfcn.h>
#include "../../logic/data_plane.h"
#include "../../logic/remote/remote_stage_builder.h"
#include "../../logic/parser/configuration_parser.h"
#include "../../logic/root/storage_drivers/libraries/libc_operations_enums.h"
#include "../../logic/root/storage_drivers/libraries/libc_operation_headers.h"

#define CONFIGS_PATH "MONARCH_CONFIGS_PATH"

typedef int (*libc_mprotect_t) (void *addr, size_t len, int prot);
typedef int (*libc_munmap_t) (void *addr, size_t length);
typedef int (*libc_brk_t) (void *addr);
typedef void* (*libc_sbrk_t) (intptr_t increment);

class TMonarch {
private:
    DataPlane* data_plane{nullptr};
    std::string m_lib_name { "libc.so.6" };
    void* m_lib_handle { nullptr };
    bool init_done = false;

public:
    TMonarch(){
        data_plane = ConfigurationParser::parse(std::getenv(CONFIGS_PATH));
        data_plane->init(true);
        data_plane->start();
        init_done = true;
    }

    ~TMonarch(){
        delete data_plane;
    }

    // dlopen_library_handle call. (...)
    bool dlopen_library_handle ()
    {
        // Dynamic loading of the libc library (referred to as 'libc.so.6').
        // loads the dynamic shared object (shared library) file named by the null-terminated string
        // filename and returns an opaque "handle" for the loaded object.
        this->m_lib_handle = ::dlopen (this->m_lib_name.data (), RTLD_LAZY);

        // return true if the m_lib_handle is valid, and false otherwise.
        return (this->m_lib_handle != nullptr);
    }

    inline int open(const char *pathname, int flags, mode_t mode){
        if(!init_done){
            if(!this->m_lib_handle){
                if(dlopen_library_handle()){
                    return ((libc_open_variadic_t)dlsym(this->m_lib_handle, "open"))(pathname, flags, mode);
                }else{
                    return ((libc_open_variadic_t)dlsym(RTLD_NEXT, "open"))(pathname, flags, mode);
                }
            }else{
                return ((libc_open_variadic_t)dlsym(this->m_lib_handle, "open"))(pathname, flags, mode);
            }
        }
        return data_plane->open(pathname, flags, mode, false);
    }

    inline int open(const char *pathname, int flags){
        if(!init_done){
            if(!this->m_lib_handle){
                if(dlopen_library_handle()){
                    return ((libc_open_t)dlsym(this->m_lib_handle, "open"))(pathname, flags);
                }else{
                    return ((libc_open_t)dlsym(RTLD_NEXT, "open"))(pathname, flags);
                }
            }else{
                return ((libc_open_t)dlsym(this->m_lib_handle, "open"))(pathname, flags);
            }
        }
        return data_plane->open(pathname, flags, false);
    }

    inline int open64(const char *pathname, int flags, mode_t mode){
        if(!init_done){
            if(!this->m_lib_handle){
                if(dlopen_library_handle()){
                    return ((libc_open64_variadic_t)dlsym(this->m_lib_handle, "open64"))(pathname, flags, mode);
                }else{
                    return ((libc_open64_variadic_t)dlsym(RTLD_NEXT, "open64"))(pathname, flags, mode);
                }
            }else{
                return ((libc_open64_variadic_t)dlsym(this->m_lib_handle, "open64"))(pathname, flags, mode);
            }
        }
        return data_plane->open(pathname, flags, mode, true);
    }

    inline int open64(const char *pathname, int flags){
        if(!init_done){
            if(!this->m_lib_handle){
                if(dlopen_library_handle()){
                    return ((libc_open64_t)dlsym(this->m_lib_handle, "open64"))(pathname, flags);
                }else{
                    return ((libc_open64_t)dlsym(RTLD_NEXT, "open64"))(pathname, flags);
                }
            }else{
                return ((libc_open64_t)dlsym(this->m_lib_handle, "open64"))(pathname, flags);
            }
        }
        return data_plane->open(pathname, flags, true);
    }

    inline ssize_t pread(int fildes, void *result, size_t nbyte, uint64_t offset){
        if(!init_done){
            if(!this->m_lib_handle){
                if(dlopen_library_handle()){
                    return ((libc_pread_t)dlsym(this->m_lib_handle, "pread"))(fildes, result, nbyte, offset);
                }else{
                    return ((libc_pread_t)dlsym(RTLD_NEXT, "pread"))(fildes, result, nbyte, offset);
                }
            }else{
                return ((libc_pread_t)dlsym(this->m_lib_handle, "pread"))(fildes, result, nbyte, offset);
            }
        }
        return data_plane->pread(fildes, static_cast<char *>(result), nbyte, offset, false);
    }

    inline ssize_t pread64(int fildes, void *result, size_t nbyte, uint64_t offset){
        if(!init_done){
            if(!this->m_lib_handle){
                if(dlopen_library_handle()){
                    return ((libc_pread64_t)dlsym(this->m_lib_handle, "pread64"))(fildes, result, nbyte, offset);
                }else{
                    return ((libc_pread64_t)dlsym(RTLD_NEXT, "pread64"))(fildes, result, nbyte, offset);
                }
            }else{
                return ((libc_pread64_t)dlsym(this->m_lib_handle, "pread64"))(fildes, result, nbyte, offset);
            }
        }
        return data_plane->pread(fildes, static_cast<char *>(result), nbyte, offset, true);
    }

    inline int close(int fildes){
        if(!init_done){
            if(!this->m_lib_handle){
                if(dlopen_library_handle()){
                    return ((libc_close_t)dlsym(this->m_lib_handle, "close"))(fildes);
                }else{
                    return ((libc_close_t)dlsym(RTLD_NEXT, "close"))(fildes);
                }
            }else{
                return ((libc_close_t)dlsym(this->m_lib_handle, "close"))(fildes);
            }
        }
        return data_plane->close(fildes);
    }


    inline int brk(void *addr){
        if(!this->m_lib_handle){
            if(dlopen_library_handle()){
                return ((libc_brk_t)dlsym(this->m_lib_handle, "brk"))(addr);
            }else{
                return ((libc_brk_t)dlsym(RTLD_NEXT, "brk"))(addr);
            }
        }else{
            return ((libc_brk_t)dlsym(this->m_lib_handle, "brk"))(addr);
        }
    }

    inline void* sbrk(intptr_t increment){
        if(!this->m_lib_handle){
            if(dlopen_library_handle()){
                return ((libc_sbrk_t)dlsym(this->m_lib_handle, "sbrk"))(increment);
            }else{
                return ((libc_sbrk_t)dlsym(RTLD_NEXT, "sbrk"))(increment);
            }
        }else{
            return ((libc_sbrk_t)dlsym(this->m_lib_handle, "sbrk"))(increment);
        }
    }

    inline void *mmap (void *addr, size_t length, int prot, int flags, int fd, off_t offset){
        if(!init_done){
            if(!this->m_lib_handle){
                if(dlopen_library_handle()){
                    return ((libc_mmap_t)dlsym(this->m_lib_handle, "mmap"))(addr, length, prot, flags, fd, offset);
                }else{
                    return ((libc_mmap_t)dlsym(RTLD_NEXT, "mmap"))(addr, length, prot, flags, fd, offset);
                }
            }else{
                return ((libc_mmap_t)dlsym(this->m_lib_handle, "mmap"))(addr, length, prot, flags, fd, offset);
            }
        }
        return data_plane->mmap(addr, length, prot, flags, fd, offset);
    }

    inline int munmap(void *addr, size_t length){
        if(!this->m_lib_handle){
            if(dlopen_library_handle()){
                return ((libc_munmap_t)dlsym(this->m_lib_handle, "munmap"))(addr, length);
            }else{
                return ((libc_munmap_t)dlsym(RTLD_NEXT, "munmap"))(addr, length);
                }
            }else{
                return ((libc_munmap_t)dlsym(this->m_lib_handle, "munmap"))(addr, length);
        }
    }

    inline int mprotect(void *addr, size_t len, int prot){
        if(!this->m_lib_handle){
            if(dlopen_library_handle()){
                return ((libc_mprotect_t)dlsym(this->m_lib_handle, "mprotect"))(addr, len, prot);
            }else{
                return ((libc_mprotect_t)dlsym(RTLD_NEXT, "mmap"))(addr, len, prot);
                }
            }else{
                return ((libc_mprotect_t)dlsym(this->m_lib_handle, "mmap"))(addr, len, prot);
        }
    }

};
#endif //THESIS_TMONARCH_H
