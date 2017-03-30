#include <stdio.h>
#include <string.h>
#include "ext2.h"
#include "ext2_utils.h"

int main(int argc, char **argv){
    char * target_file_path;
    char target_file_name[256];
    unsigned int tpath_index, tfile_index;
	
	// check for arguments
	if(argc != 3){
	    fprintf(stderr, "Usage: ext2_rm <image file name> <target path>\n");
        exit(1);
	}
	
	// open the disk 
	if (ext2_open(argv[1]) == -1){
        return ENOENT;
    }
    
    // check target path
    if(!is_valid(argv[2]) ) {
        printf("invaid abs path\n");
        return ENOENT;
    }
    
    // find the file we want to rm
    target_file_path = malloc(strlen(argv[2]));
    get_last_entry(argv[2],target_file_name, target_file_path);
    
    // check the file is there or not
    
    if (! strlen(target_file_name)){
        fprintf(stderr, "Invaid target file name\n");
        return ENOENT;
    }
    
    if (is_dir(argv[2])){
        fprintf(stderr, "it is a dir. we don't support rm -r just now maybe never in the future. maybe in d69 or not.\n");
        return ENOENT;
    }
    
    // get inode index for target file that i want to delete
    tfile_index = get_inode_index(argv[2]);
    tpath_index = get_inode_index(target_file_path);
    free(target_file_path);

    // check the path
    if(!tpath_index){
        printf("No such file or directory\n");
        return ENOENT;
    }    
    // check if the target is a regular file
    if(tfile_index){
        if(! (get_inode(tfile_index)->i_mode & EXT2_S_IFREG)){
            printf("%s not a file\n", target_file_name);
            return ENOENT;
        }
    }
    // now just remove it
    remove_inode(get_inode(tpath_index), get_inode(tfile_index), tfile_index);

    return 0;
}
