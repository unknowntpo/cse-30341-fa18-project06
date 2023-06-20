/* fs.c: SimpleFS file system */

#include "sfs/fs.h"
#include "sfs/logging.h"
#include "sfs/utils.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

/**
 * Debug FileSystem by doing the following
 *
 *  1. Read SuperBlock and report its information.
 *
 *  2. Read Inode Table and report information about each Inode.
 *
 * @param       disk        Pointer to Disk structure.
 **/
void fs_debug(Disk *disk)
{
    Block block;

    /* Read SuperBlock */
    if (disk_read(disk, 0, (char *)&block.data) == DISK_FAILURE)
    {
        error("failed on disk_read for superblock");
        return;
    }

    SuperBlock sb = block.super;
    printf("SuperBlock:\n");
    printf("    %u blocks\n", sb.blocks);
    printf("    %u inode blocks\n", sb.inode_blocks);
    printf("    %u inodes\n", sb.inodes);

    /* Read Inodes */
    // printf("    %u inodes\n", block.);
    // for each inode block,
    // // for each inode
    // // // print status
    /* Skip super block */
    int inodeBlockOffSet = 1;
    for (int b = inodeBlockOffSet; b < inodeBlockOffSet + sb.inode_blocks; b++)
    {
        if (disk_read(disk, b, (char *)block.inodes) == DISK_FAILURE)
        {
            error("failed on disk_read at inodeBlockOffSet: %d", b);
            return;
        }

        for (int inode_idx = 0; inode_idx < INODES_PER_BLOCK; inode_idx++)
        {
            // // for each inode
            Inode inode = block.inodes[inode_idx];
            printf("inodes[%d][%d]: ", b - 1, inode_idx);
            // uint32_t valid;                      /* Whether or not inode is valid */
            // uint32_t size;                       /* Size of file */
            // uint32_t direct[POINTERS_PER_INODE]; /* Direct pointers */
            // uint32_t indirect;                   /* Indirect pointers */
            printf("    valid: %d\n", inode.valid);
            if (!inode.valid)
            {
                continue;
            }
            printf("    direct blocks:\t");
            print_direct_blocks(&inode.direct);
            printf("    indirect block location: block[%d]\n", inode.indirect);
        }
    }
}

void print_direct_blocks(uint32_t *pDirect)
{
    printf("[");
    for (int i = 0; i < POINTERS_PER_INODE; i++)
    {
        printf("%d", *(pDirect + i));
        printf(",");
    }
    printf("]\n");
}

void print_indirect_blocks(uint32_t *pIndir)
{
    printf("[");
    for (int i = 0; i < POINTERS_PER_BLOCK; i++)
    {
        if (*(pIndir + i) != 0)
        {
            printf("%d", *(pIndir + i));
            printf(",");
        }
        else
        {
            continue;
        }
    }
    printf("]\n");
}

/**
 * Format Disk by doing the following:
 *
 *  1. Write SuperBlock (with appropriate magic number, number of blocks,
 *  number of inode blocks, and number of inodes).
 *
 *  2. Clear all remaining blocks.
 *
 * Note: Do not format a mounted Disk!
 *
 * @param       disk        Pointer to Disk structure.
 * @return      Whether or not all disk operations were successful.
 **/
bool fs_format(Disk *disk)
{
    return false;
}

/**
 * Mount specified FileSystem to given Disk by doing the following:
 *
 *  1. Read and check SuperBlock (verify attributes).
 *
 *  2. Record FileSystem disk attribute and set Disk mount status.
 *
 *  3. Copy SuperBlock to FileSystem meta data attribute
 *
 *  4. Initialize FileSystem free blocks bitmap.
 *
 * Note: Do not mount a Disk that has already been mounted!
 *
 * @param       fs      Pointer to FileSystem structure.
 * @param       disk    Pointer to Disk structure.
 * @return      Whether or not the mount operation was successful.
 **/
bool fs_mount(FileSystem *fs, Disk *disk)
{
    if (disk->mounted)
    {
        error("disk is already mounted");
        return FS_FAILURE;
    }

    fs->meta_data.blocks = disk->blocks;

    // See doc of SuperBlock.blocks for more example about value of inode_blocks.
    fs->meta_data.inode_blocks = ceil((double)fs->meta_data.blocks / (double)10);
    fs->meta_data.inodes = INODES_PER_BLOCK * fs->meta_data.inode_blocks;

    Block block;
    ssize_t nread = disk_read(disk, 0, (char *)block.data);
    if (nread == DISK_FAILURE)
    {
        error("failed on disk_read for superblock");
        return false;
    }

    memcpy(&fs->meta_data, &block.data, sizeof(fs->meta_data));

    fs->meta_data = block.super;
    if (fs->meta_data.magic_number != MAGIC_NUMBER)
    {
        error("wrong magic number, got %x want %x", fs->meta_data.magic_number, MAGIC_NUMBER);
    };

    fs_debug(disk);

    // All blocks are free ?
    if (fs_build_free_block_map(fs, disk) == FS_FAILURE)
    {
        error("failed on fs_build_free_block_map");
        return false;
    };

    // fs->meta_data.inodes = disk->blocks;
    disk->mounted = true;

    fs->disk = disk;

    return true;
}

/*
[sb, i, i]
fs->meta_data.inode_blocks = 2
*/
int fs_build_free_block_map(FileSystem *fs, Disk *disk)
{
    fs->free_blocks = malloc(fs->meta_data.blocks * sizeof(fs->free_blocks));
    if (fs->free_blocks == NULL)
        return FS_FAILURE;

    // set all blocks to be free except superblock and inode blocks
    for (int i = 0; i < fs->meta_data.blocks; i++)
    {
        // i == 0: superblock, i == 1: inode block
        fs->free_blocks[i] = true;
    }

    // set super block to false;
    fs->free_blocks[0] = false;

    for (size_t i = 0; i < fs->meta_data.inode_blocks; i++)
    {
        size_t num_of_superblock = 1;
        fs->free_blocks[num_of_superblock + i] = false;
    }

    for (int i = 0; i < fs->meta_data.blocks; i++)
    {
        printf("before set: free_blocks[%d]: %d\n", i, fs->free_blocks[i]);
    }

    // FIXME: Why memset will cause seg fault ?
    // memset(&fs->free_blocks, true, fs->meta_data.blocks * sizeof(bool));

    Block block;
    /* Skip super block */
    int inodeBlockOffSet = 1;
    for (int b = inodeBlockOffSet; b < inodeBlockOffSet + fs->meta_data.inode_blocks; b++)
    {
        if (disk_read(disk, b, (char *)block.inodes) == DISK_FAILURE)
        {
            error("failed on disk_read at inodeBlockOffSet: %d", b);
            return FS_FAILURE;
        }

        for (int inode_idx = 0; inode_idx < INODES_PER_BLOCK; inode_idx++)
        {

            //         set fs->free_blocks(offset)
            // int offset = b * INODES_PER_BLOCK + inode_idx;
            Inode inode = block.inodes[inode_idx];
            if (inode.valid)
            {
                // fs->free_blocks[offset] = valid;
                // scan direct blocks
                for (int direct_idx = 0; direct_idx < POINTERS_PER_INODE; direct_idx++)
                {
                    uint32_t ptr = inode.direct[direct_idx];
                    if (ptr != 0)
                    {
                        // this block is in use
                        fs->free_blocks[ptr] = false;
                    }
                }

                if (inode.indirect > 0)
                {
                    // mark indirect blocks in-use
                    fs->free_blocks[inode.indirect] = false;
                    // read indirect block
                    Block indir_block;
                    if (disk_read(disk, inode.indirect, (char *)indir_block.pointers) == DISK_FAILURE)
                    {
                        error("failed on disk_read at indirect block: block_number: %d", b);
                        return FS_FAILURE;
                    }
                    // for every indir pointers inside indir_block
                    // set fs->free_blocks
                    for (int i = 0; i < POINTERS_PER_BLOCK; i++)
                    {
                        size_t ptr = indir_block.pointers[i];
                        if (ptr != 0)
                            // this block is in use
                            fs->free_blocks[ptr] = false;
                    }
                }
            }
        }
    }

    for (int i = 0; i < fs->meta_data.blocks; i++)
    {
        printf("free_blocks[%d]: %d\n", i, fs->free_blocks[i]);
    }

    return fS_SUCCESS;
}

/**
 * Unmount FileSystem from internal Disk by doing the following:
 *
 *  1. Set Disk mounted status and FileSystem disk attribute.
 *
 *  2. Release free blocks bitmap.
 *
 * @param       fs      Pointer to FileSystem structure.
 **/
void fs_unmount(FileSystem *fs)
{
}

/**
 * Allocate an Inode in the FileSystem Inode table by doing the following:
 *
 *  1. Search Inode table for free inode.
 *
 *  2. Reserve free inode in Inode table.
 *
 * Note: Be sure to record updates to Inode table to Disk.
 *
 * @param       fs      Pointer to FileSystem structure.
 * @return      Inode number of allocated Inode.
 **/
ssize_t fs_create(FileSystem *fs)
{
    size_t block_idx = fs->meta_data.inodes / INODES_PER_BLOCK;
    size_t lastIdx = fs->meta_data.inodes % INODES_PER_BLOCK;

    // FIXME: What is the maximum inodes in fs ?

    /*
        new inode location: [block_idx, lastIdx]
        Example:
        - INODES_PER_BLOCK = 3
        - block_idx = 4 / 3 = 1
        - lastIdx = 4 % 3 = 1
        [0, 1, 2] [3]
    */
}

/**
 * Remove Inode and associated data from FileSystem by doing the following:
 *
 *  1. Load and check status of Inode.
 *
 *  2. Release any direct blocks.
 *
 *  3. Release any indirect blocks.
 *
 *  4. Mark Inode as free in Inode table.
 *
 * @param       fs              Pointer to FileSystem structure.
 * @param       inode_number    Inode to remove.
 * @return      Whether or not removing the specified Inode was successful.
 **/
bool fs_remove(FileSystem *fs, size_t inode_number)
{
    return false;
}

/**
 * Return size of specified Inode.
 *
 * @param       fs              Pointer to FileSystem structure.
 * @param       inode_number    Inode to remove.
 * @return      Size of specified Inode (-1 if does not exist).
 **/
ssize_t fs_stat(FileSystem *fs, size_t inode_number)
{
    return -1;
}

/**
 * Read from the specified Inode into the data buffer exactly length bytes
 * beginning from the specified offset by doing the following:
 *
 *  1. Load Inode information.
 *
 *  2. Continuously read blocks and copy data to buffer.
 *
 *  Note: Data is read from direct blocks first, and then from indirect blocks.
 *
 * @param       fs              Pointer to FileSystem structure.
 * @param       inode_number    Inode to read data from.
 * @param       data            Buffer to copy data to.
 * @param       length          Number of bytes to read.
 * @param       offset          Byte offset from which to begin reading.
 * @return      Number of bytes read (-1 on error).
 **/
ssize_t fs_read(FileSystem *fs, size_t inode_number, char *data, size_t length, size_t offset)
{
    return -1;
}

/**
 * Write to the specified Inode from the data buffer exactly length bytes
 * beginning from the specified offset by doing the following:
 *
 *  1. Load Inode information.
 *
 *  2. Continuously copy data from buffer to blocks.
 *
 *  Note: Data is read from direct blocks first, and then from indirect blocks.
 *
 * @param       fs              Pointer to FileSystem structure.
 * @param       inode_number    Inode to write data to.
 * @param       data            Buffer with data to copy
 * @param       length          Number of bytes to write.
 * @param       offset          Byte offset from which to begin writing.
 * @return      Number of bytes read (-1 on error).
 **/
ssize_t fs_write(FileSystem *fs, size_t inode_number, char *data, size_t length, size_t offset)
{
    return -1;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
