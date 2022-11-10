//
// Created by dantas on 19-10-2022.
//

#include "posix_libc_caller.h"

std::string PosixLibcCaller::m_lib_name { "libc.so.6" };
void* PosixLibcCaller::m_lib_handle { nullptr };
libc_metadata PosixLibcCaller::m_metadata_operations {};
libc_data PosixLibcCaller::m_data_operations {};
bool PosixLibcCaller::init_done { false };
std::mutex PosixLibcCaller::mutex_;


bool PosixLibcCaller::dlopen_library_handle ()
{
    // Dynamic loading of the libc library (referred to as 'libc.so.6').
    // loads the dynamic shared object (shared library) file named by the null-terminated string
    // filename and returns an opaque "handle" for the loaded object.
    m_lib_handle = ::dlopen (m_lib_name.data (), RTLD_LAZY);

    // return true if the m_lib_handle is valid, and false otherwise.
    return (m_lib_handle != nullptr);
}

//This must be called!!
void PosixLibcCaller::init(){
    std::lock_guard<std::mutex> lock(mutex_);
    if(init_done){
        return;
    }
    init_done = true;
    if(dlopen_library_handle()){
        m_metadata_operations.m_open_var = (libc_open_variadic_t) dlsym(m_lib_handle, "open");
        m_metadata_operations.m_open = (libc_open_t) dlsym(m_lib_handle, "open");
        m_metadata_operations.m_open64_var = (libc_open64_variadic_t) dlsym(m_lib_handle, "open64");
        m_metadata_operations.m_open64 = (libc_open64_t) dlsym(m_lib_handle, "open64");
        m_data_operations.m_pread = (libc_pread_t) dlsym(m_lib_handle, "pread");
        m_data_operations.m_pread64 = (libc_pread64_t) dlsym(m_lib_handle, "pread64");
        m_data_operations.m_mmap = (libc_mmap_t) dlsym(m_lib_handle, "mmap");
        m_metadata_operations.m_close = (libc_close_t) dlsym(m_lib_handle, "close");
    }
}

int PosixLibcCaller::open(const char *pathname, int flags, mode_t mode){
    return m_metadata_operations.m_open_var(pathname, flags, mode);
}

int PosixLibcCaller::open(const char *pathname, int flags){
    return m_metadata_operations.m_open(pathname, flags);
}

int PosixLibcCaller::open64(const char *pathname, int flags, mode_t mode){
    return m_metadata_operations.m_open64_var(pathname, flags, mode);
}

int PosixLibcCaller::open64(const char *pathname, int flags){
    return m_metadata_operations.m_open64(pathname, flags);
}

ssize_t PosixLibcCaller::pread(int fildes, void *result, size_t nbyte, off_t offset){
    return m_data_operations.m_pread(fildes, result, nbyte, offset);
}

ssize_t PosixLibcCaller::pread64(int fildes, void *result, size_t nbyte, off_t offset){
    return m_data_operations.m_pread64(fildes, result, nbyte, offset);
}

void *PosixLibcCaller::mmap (void *addr, size_t length, int prot, int flags, int fd, off_t offset){
    return m_data_operations.m_mmap(addr, length, prot, flags, fd, offset);
}

int PosixLibcCaller::close(int fildes){
    return m_metadata_operations.m_close(fildes);
}
