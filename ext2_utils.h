#ifndef EXT2_UTILS_H
#define EXT2_UTILS_H

#include "ext2.h"
#include <stdlib.h>
#include <errno.h>

#define BLOCK_PTR(i) (disk + (i)*EXT2_BLOCK_SIZE)
#define GROUP_PTR(i) (disk + (i)*super_block.s_blocks_per_group*EXT2_BLOCK_SIZE)

extern unsigned char * disk;
extern struct ext2_super_block super_block;

unsigned int allocate_block();
unsigned int allocate_inode();

void copy_inode(unsigned int dest, unsigned int source);
int add_entry(struct ext2_inode * inode, struct ext2_dir_entry_2 * dir_entry, char * entry_name);
int remove_inode(struct ext2_inode * parent, struct ext2_inode * target, unsigned int tfile_index);

int is_valid(const char* filepath);
int is_dir(const char* filepath);
int ext2_open(const char * disk_image);

void get_last_entry(const char * filepath, char * last_entry, char * rest);
unsigned int get_inode_index(const char * filepath);
struct ext2_inode * get_inode(unsigned int index);
struct ext2_dir_entry_2 * find_entry_inode(const struct ext2_inode * inode, const char * entry_name);

#endif 
