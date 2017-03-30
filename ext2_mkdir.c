#include <string.h>
#include <stdio.h>
#include "ext2.h"
#include "ext2_utils.h"

int main(int argc, char **argv){
    // the new path where the new dir will be
    char * dir_path;
    // the name of the new dir upto 250 char long
    char dir_name[256];
    
    unsigned int parent_index, new_dir_index;
    
    // the new inode new entry for parent and the parent inode for ..
    struct ext2_inode new_dir_inode;
	struct ext2_dir_entry_2 new_dir;
	struct ext2_inode * parent_inode;
	
	// first and formost check the number of argument is correct or not
	if(argc != 3){
	    fprintf(stderr, "Usage: ext2_mkdir <image file name> <file path>\n");
        exit(1);
	}
	
	// open the disk file and map it into memory
	if (ext2_open(argv[1]) == -1){
    	exit(1);
    }
	
	// check the file path
	
	if(!is_valid(argv[2])){
    	fprintf(stderr, "Invaid abs path\n");
        return ENOENT;
    }
    
    // find the parent path and the new dir name
    dir_path = malloc(strlen(argv[2]));
    get_last_entry(argv[2],dir_name,dir_path);
    
    // check if it is a vaild dirs
    if (strlen(dir_name) == 0){
    	printf("invaid dir name\n");
    	return ENOENT;
    } 
    parent_index = get_inode_index(dir_path);
    
    // check if the second last entry is a valid dir
    if((!parent_index) || !((get_inode(parent_index)->i_mode & EXT2_S_IFDIR))){
    	printf("mkdir: %s: No such file or directory\n", dir_path);
    	return ENOENT;
    }
    
    // check if the dir is already in parent dir
	parent_inode = get_inode(parent_index);
    if (find_entry_inode(parent_inode, dir_name)){
    	printf("dir already exist\n");
    	return EEXIST;
    } 
    
    
    // we don't need it anymore, so 
    // LET IT GO~~~~ LET IT GO~~~ CAN'T HOLD IT BACK ANYMORE~~
    free(dir_path);
    
    // creat a new dir inode
    new_dir_index = allocate_inode();
    // set links the only link is from the parent node
    new_dir_inode.i_links_count = 1;
    // so it is a dir
    new_dir_inode.i_mode |= EXT2_S_IFDIR;
    
    // set the block pointer to 0 because when it just got created it must be empty
    int i;
    for(i = 0; i < 15; i++){
        new_dir_inode.i_block[i] = 0;
    }
    
    // set new dir
	*(get_inode(new_dir_index)) = new_dir_inode;
	new_dir.inode = new_dir_index;
	new_dir.name_len = strlen(dir_name);
	new_dir.file_type = EXT2_FT_DIR;
	
	//add new dir to it's parent
	add_entry(parent_inode, &new_dir, dir_name);
	
	//add . and .. to the new dir which is itself and parent
	struct ext2_dir_entry_2 itself;
	struct ext2_dir_entry_2 daddy;

	itself.inode = new_dir_index;
	itself.name_len = 1;
	itself.file_type = EXT2_FT_DIR;

	daddy.inode = parent_index;
	daddy.name_len = 2;
	daddy.file_type = EXT2_FT_DIR;

	add_entry(get_inode(new_dir_index), &itself, ".");
	add_entry(get_inode(new_dir_index), &daddy, "..");

	return 0;
}
