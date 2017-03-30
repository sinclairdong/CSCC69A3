#include <stdio.h>
#include <string.h>
#include "ext2.h"
#include "ext2_utils.h"


int main(int argc, char **argv) {
	char * target_file_path;
	char target_file_name[256];
	unsigned int tpath_index, s_file_index;
	struct ext2_dir_entry_2 new_entry;
	struct ext2_inode * source_file_inode;

    // check for argument format
	if(argc != 4) {
        fprintf(stderr, "Usage: ext2_ln <image file name> <source path> <target path>\n");
        exit(1);
    }
    
    
    // open the disk
    if (ext2_open(argv[1]) == -1){
        exit(1);
    }

    // check source path and target path
    if(!is_valid(argv[2]) || !is_valid(argv[3]) ) {
        printf("invaid abs path\n");
        return ENOENT;
    }

    // get target file path and name
    target_file_path = malloc(strlen(argv[3]));
    get_last_entry(argv[3],target_file_name, target_file_path);

    // check if it is vaild target file name
    if (strlen(target_file_name) == 0){
        fprintf(stderr, "Invaid target file name\n");
        return ENOENT;
    } 
    
    // check weater it is a dir
    if(is_dir(argv[3])){
        fprintf(stderr, "Invaid target file name\n");
        return ENOENT;
    }
    
    // find inode index for both source file and target file
    s_file_index = get_inode_index(argv[2]);
    tpath_index = get_inode_index(target_file_path);
    free(target_file_path);
    
    // check if source path and target path are valid
    if(!s_file_index || !tpath_index){
        printf("No such file or directory\n");
        return ENOENT;
    }
    
    // check if the target file had already existed
    if(get_inode_index(argv[3])){
        if((get_inode(get_inode_index(argv[3]))->i_mode & EXT2_S_IFREG)){
            printf("target file already exist\n");
            return EEXIST;
        }
    }
    
    // check if the source path lead to a file
    source_file_inode = get_inode(s_file_index);
    if(!(source_file_inode->i_mode & EXT2_S_IFREG) ){
        fprintf(stderr, "it is not a file\n");
        return EISDIR;
    }
    // add a new entry to the parent for target
    new_entry.file_type = EXT2_FT_REG_FILE;
    // link to source file
    new_entry.inode = s_file_index;
    new_entry.name_len = strlen(target_file_name);

    source_file_inode->i_links_count++;

    add_entry(get_inode(tpath_index), &new_entry, target_file_name);

	return 0;
}
