//
// Created by dantas on 17/07/21.
//

#ifndef THESIS_POSIX_FILE_SYSTEM_H
#define THESIS_POSIX_FILE_SYSTEM_H

#include "tmonarch.h"

TMonarch monarch {};

/**
 * init_method: constructor of the PosixFileSystem.
 * This method is executed before the program executes its main (). Under shared objects, this
 * occurs at load time.
 * The method needs to use printf instead of std::cout due to a static initialization order problem.
 * (https://stackoverflow.com/questions/16746166/using-cout-in-constructor-gives-segmentation-fault)
 */
static __attribute__ ((constructor)) void init_method ()
{
    std::printf ("Monarch constructor\n");
    std::this_thread::sleep_for (std::chrono::seconds (1));
}

/**
 * destroy_method: destructor of the PosixFileSystem.
 * This method will execute once the main process (main ()) has returned or exit() is called.
 */
static __attribute__ ((destructor)) void destroy_method ()
{
    std::printf ("Monarch destructor\n");
}

/**
* open:
* @param path
* @param flags
* @param ...
* @return
*/
extern "C" int open (const char* path, int flags, ...);

/**
 * open64:
 * @param path
 * @param flags
 * @param ...
 * @return
 */
extern "C" int open64 (const char* path, int flags, ...);


/**
 * close:
 * @param fd
 * @return
 */
extern "C" int close (int fd);

/**
 * pread:
 * @param fd
 * @param buf
 * @param size
 * @param offset
 * @return
 */
extern "C" ssize_t pread (int fd, void* buf, size_t size, off_t offset);

/**
 * pread64:
 * @param fd
 * @param buf
 * @param size
 * @param offset
 * @return
 */
#if defined(__USE_LARGEFILE64)
extern "C" ssize_t pread64 (int fd, void* buf, size_t size, off64_t offset);
#endif


/**
 *
 * mmap
 * @param *addr
 * @param length
 * @param prot
 * @param flags
 * @param fd
 * @param offset
 *
 */

extern "C" void *mmap (void *addr, size_t length, int prot, int flags, int fd, off_t offset);


//extern "C" int munmap(void *addr, size_t length);


//extern "C" int mprotect(void *addr, size_t len, int prot);


#endif //THESIS_POSIX_FILE_SYSTEM_H
