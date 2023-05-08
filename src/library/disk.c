/* disk.c: SimpleFS disk emulator */

#include "sfs/disk.h"
#include "sfs/logging.h"

#include <fcntl.h>
#include <unistd.h>

/* Internal Prototyes */

bool disk_sanity_check(Disk *disk, size_t blocknum, const char *data);

/* External Functions */

/**
 *
 * Opens disk at specified path with the specified number of blocks by doing
 * the following:
 *
 *  1. Allocates Disk structure and sets appropriate attributes.
 *
 *  2. Opens file descriptor to specified path.
 *
 *  3. Truncates file to desired file size (blocks * BLOCK_SIZE).
 *
 * @param       path        Path to disk image to create.
 * @param       blocks      Number of blocks to allocate for disk image.
 *
 * @return      Pointer to newly allocated and configured Disk structure (NULL
 *              on failure).
 **/
Disk *disk_open(const char *path, size_t blocks)
{
    // malloc
    Disk *disk = malloc(sizeof(Disk));
    if (!disk)
    {
        error("failed on malloc for Disk");
        goto cleanup;
    }

    disk->blocks = BLOCK_SIZE;
    disk->reads++;
    disk->writes++;
    // FIXME: Should I modify disk->mounted here ?
    disk->mounted = true;
    disk->fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (disk->fd == -1)
    {
        error("failed to open file %s", path);
        goto cleanup_free_disk;
    }

    if (ftruncate(disk->fd, blocks * BLOCK_SIZE) == -1)
    {
        error("failed to truncate file %s", path);
        goto cleanup_close_fd;
    }

    return disk;

cleanup_close_fd:
    close(disk->fd);
cleanup_free_disk:
    free(disk);
cleanup:
    return NULL;
}
/**
 * Close disk structure by doing the following:
 *
 *  1. Close disk file descriptor.
 *
 *  2. Report number of disk reads and writes.
 *
 *  3. Releasing disk structure memory.
 *
 * @param       disk        Pointer to Disk structure.
 */
void disk_close(Disk *disk)
{
    if (close(disk->fd) == -1)
        error("failed to close fd");
    disk->reads--;
    disk->writes--;
    free(disk);
}

/**
 * Read data from disk at specified block into data buffer by doing the
 * following:
 *
 *  1. Performing sanity check.
 *
 *  2. Seeking to specified block.
 *
 *  3. Reading from block to data buffer (must be BLOCK_SIZE).
 *
 * @param       disk        Pointer to Disk structure.
 * @param       block       Block number to perform operation on.
 * @param       data        Data buffer.
 *
 * @return      Number of bytes read.
 *              (BLOCK_SIZE on success, DISK_FAILURE on failure).
 **/
ssize_t disk_read(Disk *disk, size_t block, char *data)
{
    if (!disk_sanity_check(disk, block, data))
    {
        error("disk_read: disk_sanity_check failed");
        return DISK_FAILURE;
    }
    off_t offset = (off_t)block * BLOCK_SIZE;
    if (lseek(disk->fd, offset, SEEK_SET) == -1)
    {
        error("disk_read: lseek failed: failed to seek to offset %d", offset);
        return DISK_FAILURE;
    }

    ssize_t nread = read(disk->fd, data, BLOCK_SIZE);
    if (nread == -1)
    {
        error("disk_read: read failed: failed to read at offset [%d]", offset);
        return DISK_FAILURE;
    }
    else if (nread != (ssize_t)BLOCK_SIZE)
    {
        error("disk_read: read incomplete (%zd/%zu bytes)", nread, BLOCK_SIZE);
        return DISK_FAILURE;
    }

    return nread;
}

/**
 * Write data to disk at specified block from data buffer by doing the
 * following:
 *
 *  1. Performing sanity check.
 *
 *  2. Seeking to specified block.
 *
 *  3. Writing data buffer (must be BLOCK_SIZE) to disk block.
 *
 * @param       disk        Pointer to Disk structure.
 * @param       block       Block number to perform operation on.
 * @param       data        Data buffer.
 *
 * @return      Number of bytes written.
 *              (BLOCK_SIZE on success, DISK_FAILURE on failure).
 **/
ssize_t disk_write(Disk *disk, size_t block, char *data)
{
    if (!disk_sanity_check(disk, block, data))
    {
        error("failed on disk_sanity_check");
        return DISK_FAILURE;
    }
    off_t offset = (off_t)block * BLOCK_SIZE;
    if (lseek(disk->fd, offset, SEEK_SET) == -1)
    {
        error("failed on lseek: failed to seek to offset %d", offset);
        return DISK_FAILURE;
    }
    ssize_t nwrite = write(disk->fd, data, BLOCK_SIZE);
    if (nwrite == -1)
    {
        error("disk_read: read failed: failed to read at offset [%d]", offset);
        return DISK_FAILURE;
    }
    else if (nwrite != (ssize_t)BLOCK_SIZE)
    {
        error("failed on write: write incomplete (%zd/%zu bytes)", nwrite, BLOCK_SIZE);
        return DISK_FAILURE;
    }
    return nwrite;
}

/* Internal Functions */

/**
 * Perform sanity check before read or write operation:
 *
 *  1. Check for valid disk.
 *
 *  2. Check for valid block.
 *
 *  3. Check for valid data.
 *
 * @param       disk        Pointer to Disk structure.
 * @param       block       Block number to perform operation on.
 * @param       data        Data buffer.
 *
 * @return      Whether or not it is safe to perform a read/write operation
 *              (true for safe, false for unsafe).
 **/
bool disk_sanity_check(Disk *disk, size_t block, const char *data)
{
    if (!disk)
    {
        error("disk should not be NULL");
        return false;
    }
    if (disk->fd == -1)
    {
        error("invalid fd: %d", disk->fd);
        return false;
    }
    // disk block shuold be sufficient
    if (disk->blocks < block)
    {
        error("disk->blocks (%lu) < block (%lu)", disk->blocks, block);
        return false;
    }
    if (!data)
    {
        error("data shuold not be NULL");
        return false;
    }
    // no one is reading or writing on disk
    // FIXME: What is the correct reference count ?
    if (disk->reads >= 1 || disk->writes >= 1)
    {
        error("someone is read / writing Disk");
        return false;
    }

    return true;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
