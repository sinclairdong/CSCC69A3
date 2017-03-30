#include "ext2_utils.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

unsigned char * disk;
struct ext2_super_block super_block;


/*
 * check if the given path is a dir base on the last char of the string
 * return 1 for it is ending with / thus it is a dir, and 0 for uncertain
 */
int is_dir(const char* filepath){
	int i = 0;
	while(filepath[i] != '\0'){
		if(filepath[i] == '/' && filepath[i+1] == '\0'){
			return 1;
		}
		i++;
	}
	return 0;
}

/*
 * check if the file path starting with
 * 1 for vaild, 0 not.
 */

int is_valid(const char* filepath){
	if(strncmp(filepath, "/", 1) == 0){
		return 1;
	}
	return 0;
}

/*
 * intialize disk with given file path
 */
int ext2_open(const char * disk_image) {

	int fd = open(disk_image, O_RDWR);
	// map the file into memory
	disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	// if the map fails
    if(disk == MAP_FAILED) {
	   perror("mmap");
	   return -1;
    }
    super_block = *(struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    
    return 0;
}

void unset_bitmap(unsigned int * bitmap, int index) {
  bitmap += (index / 32);
  *bitmap &= (unsigned int)( ~(1 << (index % 32)) );
}

/*
 * return the inode base on the given index
 */
struct ext2_inode * get_inode(unsigned int index)
{
	struct ext2_group_desc * gd = (struct ext2_group_desc *)(disk + 2 * 1024);
	char * inode_start = (char *)(disk + EXT2_BLOCK_SIZE * gd->bg_inode_table);
	return (struct ext2_inode *)(inode_start + (sizeof (struct ext2_inode) * (index- 1)));
}

/*
 * search for a specific entry in a gievn dir_list 
 * return NULL if it doesn't exist
 */
struct ext2_dir_entry_2 * search_entry(const unsigned char * dir_list, const char * name) {
	struct ext2_dir_entry_2 * dir_entry;
	const unsigned char * len = dir_list + EXT2_BLOCK_SIZE;
	const unsigned char * current = dir_list;
	// loop through the list to see if there is an entry with the same name
	while(current < len){
		dir_entry = (struct ext2_dir_entry_2 *)current;
		// check if they have the same name
		if ((strlen(name) <= dir_entry->name_len) && (strncmp(dir_entry->name, name, dir_entry->name_len) == 0)){
			return dir_entry;
		}
		// move forward
		current += dir_entry->rec_len;
	}
	
	return NULL;
}


/*
 * find a specific entry in a givn inode, return the entry if it is found.
 * Otherwise return NULL
 */
struct ext2_dir_entry_2 * find_entry_inode(const struct ext2_inode * inode, const char * entry) {

	int i, j, k;

	struct ext2_dir_entry_2 * dir_entry;
	unsigned int len = (EXT2_BLOCK_SIZE / sizeof (unsigned int));


	for (i = 0; i < 14; i++) {
		// direct block
		if (inode->i_block[i] && i < 12) {
			dir_entry = search_entry(BLOCK_PTR(inode->i_block[i]), entry);
			if (dir_entry && strncmp(dir_entry->name,entry, dir_entry->name_len) == 0){
				return dir_entry;
			} 
		}
		
		// single indirect block
		if(inode->i_block[i] && i == 12){
			unsigned int * block_ptrs = (unsigned int *)BLOCK_PTR(inode->i_block[12]);
			
			for(j = 0; j < len; j++ ){
				dir_entry = search_entry(BLOCK_PTR(block_ptrs[j]), entry);
				if (dir_entry && strncmp(dir_entry->name,entry, dir_entry->name_len) == 0){
					return dir_entry;
				} 
			}
		}
		// double indirect block
		if(inode->i_block[i] && i == 13){
			unsigned int ** ind_block_ptrs = NULL;
			
			for(j = 0; j < len; j++){
				ind_block_ptrs[j] = ((unsigned int *)BLOCK_PTR(inode->i_block[13])) + j;
				if(ind_block_ptrs[j]){
					for(k = 0; k < len; k++) {
						dir_entry = search_entry(BLOCK_PTR(ind_block_ptrs[j][k]), entry);
						if (dir_entry && strncmp(dir_entry->name,entry, dir_entry->name_len) == 0) {
							return dir_entry;
						}
					}
				}	
			}
		}
	}

	return NULL;
}

/*
 * return the index of the inode based on the given path if exist.
 * return 0 otherwise
 */
unsigned int get_inode_index(const char * abs_path){
	// initial the inode index with root inode
	unsigned inode_index = EXT2_ROOT_INO;
	char name[256];
	int len = strlen(abs_path);
	struct ext2_dir_entry_2 * dir_entry;
	// current char position in the abs_path
	int position = 0;
	while(position < len) {

		int i = 0;

		while (abs_path[position] != '/' && position < len) {
			name[i] = abs_path[position];
			i++;
			position++;
		}
		name[i] = '\0';
		if (strlen(name)) {
			dir_entry = find_entry_inode(get_inode(inode_index), name);
			// no such file or dir
			if (dir_entry == NULL) {
				return 0;
			} 
			inode_index = dir_entry->inode;
			//current inode is not dir and not reach the end of abs_path
			if (!(get_inode(inode_index)->i_mode & EXT2_S_IFDIR) && position < len) {
				return 0;
			}
		}
		position++;
	}
	return inode_index;
}

/*
 * store name of the last entry bass on the given file path in last_entry 
 * store rest of the path in rest
 */
void get_last_entry(const char * filepath, char * last_entry, char * rest) {

	int position = 0; //tracing the inital position of last entry
	int flen = 0;
	int plen = strlen(filepath);
	int i = 0;
	
	while (i < plen) {
	
		if (filepath[i] != '/') {
			position = i;
			flen = 0;
			
			while(filepath[i] != '/' && i < plen) {
				flen++;
				i++;
			}
			
		} else {
			i++;
		}
	}
	strncpy(last_entry, filepath + position, flen);
	last_entry[flen] = '\0';
	strncpy(rest, filepath, position);
	rest[position] = '\0';
}

/*
 * allocate an empty block
 */
unsigned int allocate_block() {
	//iterate through all block groups
	unsigned int block_len = super_block.s_blocks_count;
	unsigned int curr_block = 0;

	while (curr_block < block_len) {
		
		struct ext2_group_desc * gd = (struct ext2_group_desc *)(disk + (curr_block + 2) * EXT2_BLOCK_SIZE);
		
		if (gd->bg_free_blocks_count > 0) {
			// find the block bitmap
			unsigned char * bitmap = (disk + (gd->bg_block_bitmap) * EXT2_BLOCK_SIZE);
			unsigned int i, j, block_position;
			int bit_position = -1;
			// find free block in the 128 blocks
			for (i = 0; i < 16; i++) {
				for(j = 0; j < 8; j++) {
					// check if the block is a free block
					if ( !((bitmap[i] >> j) & 1) ) {
						bit_position = 8 * i + j;
						break;
					}
				}
			}
			
			// the disk is full means no free blocks anymore
			if (bit_position < 0){
				return 0;
			} 

			block_position = bit_position + curr_block + super_block.s_first_data_block;

	
			unsigned char * byte = bitmap + bit_position / 8;
			*byte |= 1 << (bit_position % 8);
			
			gd->bg_free_blocks_count--;
			return block_position;
		}
		curr_block += super_block.s_blocks_per_group;
	}
	return 0;
}

/*
 * allocate free inode
 * if exist, return the inode index otherwise return 0;
 */
unsigned int allocate_inode() {
	//iterate all block groups
	unsigned int end_block = super_block.s_blocks_count;
	unsigned int curr_block = 0;
	unsigned int curr_group = 0;

	while (curr_block < end_block) {
		
		struct ext2_group_desc * gd = (struct ext2_group_desc *)(disk + (curr_block + 2) * EXT2_BLOCK_SIZE);
		
		if (gd->bg_free_blocks_count > 0) {
			// get inode bit map
			unsigned char * bitmap = (disk + (gd->bg_inode_bitmap) * EXT2_BLOCK_SIZE);
			unsigned int i, j, inode_index;
			int bit_position = -1;

			// try to find a free inode
			for (i = 0; i < 4; i++) {
				for(j = 0; j < 8; j++) {
					// see if it is a free inode
					if ( !((bitmap[i] >> j) & 1) ) {
						bit_position = 8 * i + j;
						break;
					}
				}
			}
			// it is full
			if (bit_position < 0){
				return 0;
			} 
			inode_index = bit_position + curr_group * super_block.s_inodes_per_group + 1;
			unsigned char * byte = bitmap + bit_position / 8;
			*byte |= 1 << (bit_position % 8);
			gd->bg_free_inodes_count--;
			return inode_index;
		}
		curr_block += super_block.s_blocks_per_group;
		curr_group += 1;
	}
	return 0;
}

/*
 * copy source file to target path
 */
void copy_inode(unsigned int target, unsigned int source){

	unsigned int i;
	struct ext2_inode * target_inode = get_inode(target);
	struct ext2_inode * source_inode = get_inode(source);
	unsigned int len = (EXT2_BLOCK_SIZE / sizeof (unsigned int));

	*target_inode = *source_inode;
	target_inode->i_links_count = 1;
	// set the mode to file 
	target_inode->i_mode |= EXT2_S_IFREG;

	for (i = 0; i < 12; i++) {
		// block is used in source then copy
		if (source_inode->i_block[i]) {
			
			target_inode->i_block[i] = allocate_block();
			
			memcpy(BLOCK_PTR(target_inode->i_block[i]), BLOCK_PTR(source_inode->i_block[i]), EXT2_BLOCK_SIZE);
		} else {
			target_inode->i_block[i] = 0;
		}
	} 
	// indirect blocks
	if (source_inode->i_block[12]) {

		target_inode->i_block[12] = allocate_block();

		unsigned int * block_ptrs1 = (unsigned int *)BLOCK_PTR(source_inode->i_block[12]);
		unsigned int * block_ptrs2 = (unsigned int *)BLOCK_PTR(target_inode->i_block[12]);

		for(i = 0; i < len; i++) {
			if (block_ptrs1[i]) {
				block_ptrs2[i] = allocate_block();
				memcpy(BLOCK_PTR(block_ptrs2[i]), BLOCK_PTR(block_ptrs1[i]), EXT2_BLOCK_SIZE);
			} else {
				block_ptrs2[i] = 0;
			}
		}
	}
}

/*
 * remove the given file inode from dir inode
 */
int remove_inode(struct ext2_inode * parent, struct ext2_inode * target, unsigned int tfile_index){

    struct ext2_group_desc * gd = (struct ext2_group_desc *)(disk + 2 * 1024);
    unsigned int inode_bitmap = gd->bg_inode_bitmap;
    unsigned int block_bitmap = gd->bg_block_bitmap;
    unsigned int * curr_block_pt = parent->i_block;
    unsigned int curr_block;
    
    int i, j, free_block_count = 0;;
    
    for (i = 0; i < 12 && parent->i_block[i]; i++) {
        curr_block = parent->i_block[i];
      
        // get the dir entry info
        struct ext2_dir_entry_2 * dir = (struct ext2_dir_entry_2 *)(disk + EXT2_BLOCK_SIZE * curr_block);
        // store pointer to previous directory entry
        struct ext2_dir_entry_2 * pre_dir;

        int position = 0;
        
        while (position < EXT2_BLOCK_SIZE) {

            position += dir->rec_len;

            if (dir->inode == tfile_index) {
                // check if the entry a file 
                if ((unsigned int) dir->file_type == EXT2_FT_REG_FILE) {
                    target->i_links_count--;
                    // this is the last link
                    if (target->i_links_count == 0) {
                        // unset bitmap for inode
                        unset_bitmap((unsigned int *) (disk + EXT2_BLOCK_SIZE * inode_bitmap), 
                        tfile_index - 1);
                        // unset bitmap for block
                        unset_bitmap((unsigned int *) (disk + EXT2_BLOCK_SIZE * block_bitmap), target->i_block[i] - 1);
                        // count the free block
                        for (j = 0; j < 12 && target->i_block[j]; j++) {
                            target->i_block[j] = 0;
                            free_block_count++;
                        }

                        target->i_blocks = 0;

                        // update free blocks and free inode
                        gd->bg_free_blocks_count += free_block_count;
                        gd->bg_free_inodes_count++;

                        target->i_size = 0;
                    }

                    pre_dir->rec_len = dir->rec_len + pre_dir->rec_len;
                    return 0;
                } 
                else {
                    return EISDIR;
                }
            }
            pre_dir = dir;
            dir = (void *) dir + dir->rec_len;
        }
        curr_block_pt++;
    }
    return 0;
}


/*
 * add an entry of an dir
 */
int add_entry(struct ext2_inode * inode, struct ext2_dir_entry_2 * dir_entry, char * name) {

	unsigned int i;
	const unsigned int dir_entry_size = sizeof(struct ext2_dir_entry_2);

	for (i = 0; i < 12; i++) {

		if (inode->i_block[i]) {

			struct ext2_dir_entry_2 * curr_entry;
			unsigned char * list_ptr = BLOCK_PTR(inode->i_block[i]);
			unsigned char * list_end = list_ptr + EXT2_BLOCK_SIZE;

			while (list_ptr < list_end){
				curr_entry = (struct ext2_dir_entry_2 *)list_ptr;
				if (list_ptr + curr_entry->rec_len >= list_end) {

					unsigned int real_size = dir_entry_size + curr_entry->name_len;

					if (list_ptr + real_size + dir_entry->name_len + dir_entry_size < list_end) {
						curr_entry->rec_len = real_size;
						list_ptr += real_size;
						break;
					}
				}
				list_ptr += curr_entry->rec_len;
			} 
			if (list_ptr != list_end) {
				dir_entry->rec_len = list_end - list_ptr;
				(*(struct ext2_dir_entry_2 *)list_ptr) = *dir_entry;
				memcpy(list_ptr + dir_entry_size, name, dir_entry->name_len);
			} else {
				continue;
			}
		} else {
			inode->i_block[i] = allocate_block();
			dir_entry->rec_len = EXT2_BLOCK_SIZE;
			memcpy(BLOCK_PTR(inode->i_block[i]) + dir_entry_size, name, dir_entry->name_len);
			(*(struct ext2_dir_entry_2 *)BLOCK_PTR(inode->i_block[i])) = *dir_entry;
		}
		break;
	}
	return 0;
}

