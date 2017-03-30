#include <stdio.h>
#include <string.h>
#include "ext2.h"
#include "ext2_utils.h"

int main(int argc, char ** argv){
    // find the target path and source path from argvs
    char * target_path, * source_path;
    char target_file[256], source_file[256];
    unsigned int s_file, t_path, t_file;
    // the new one we need to add to the target
    struct ext2_dir_entry_2 new_entry;
    
    // first of all check the format of the input see if it is valid
    if(argc != 4) {
        fprintf(stderr, "Usage: ext2_cp <image file name> <source path> <target path>\n");
        exit(1);
    }
    
    // then check if the provied path are valid, both of them
    if(!is_valid(argv[3]) || !is_valid(argv[2]) ) {
        fprintf(stderr, "invaid abs path\n");
        return ENOENT;
    }
    
    // let's open the disk image
    if (ext2_open(argv[1]) == -1){
        exit(1);
    }
    //let's find the path of both sorce and target
    
    target_path = malloc(strlen(argv[3]));
    get_last_entry(argv[3], target_file, target_path);
    
    source_path = malloc(strlen(argv[2]));
    get_last_entry(argv[2], source_file, source_path);
    
    //we need to find which source file are we copying
    s_file = get_inode_index(argv[2]);
    // also we need the path to target file
    // find target file parent first
    t_path = get_inode_index(target_path);
    // then find the file( it could be a dir or the file already exists)
    t_file = get_inode_index(argv[3]);
    
    // we don't really need target_apth and sourch path anymore so again
    // LET IT GO~~~~~ LET IT GO~~~ CAN'T HOLD IT BACK ANYMORE~~~ 
    // WE ALL LOVE SOME BGM EH?
    
    free(target_path);
    free(source_path);
    
    // so t_path should really be a dir and s_file should be a file. Other wise i am little upset
    if((t_path && (get_inode(t_path)->i_mode & EXT2_S_IFDIR)) && (s_file && (get_inode(s_file)->i_mode & EXT2_S_IFREG))){
        // best case file doesn't exist
        if(!t_file){
            // get a new inode
            t_file = allocate_inode();
            // get whatever in s_file to t_file
            copy_inode(t_file, s_file);
            
            // wee need to do somthing to the target parents give by them a new entry!!!
            new_entry.file_type = EXT2_FT_REG_FILE;
            new_entry.inode = t_file;
            new_entry.name_len = strlen(target_file);
            // now let us add it
            add_entry(get_inode(t_path), &new_entry, target_file);
        }else if(get_inode(t_file)->i_mode & EXT2_S_IFDIR){ 
            // if it is a dir let's copy the ting to it and with same name
            if(!find_entry_inode(get_inode(t_file), source_file)){
                t_path = t_file;
                t_file = allocate_inode();
                copy_inode(t_file, s_file);
                // same as above
                new_entry.file_type = EXT2_FT_REG_FILE;
                new_entry.inode = t_file;
                new_entry.name_len = strlen(source_file);

                add_entry(get_inode(t_path), &new_entry, source_file);
            }
            // sfile already exist
            else{
                fprintf(stderr, "file already exist\n");
                return ENOENT;
            }
        }else{
            fprintf(stderr, "file already exist\n");
            return ENOENT;
        }
    }else{
        fprintf(stderr, "INVALID PATH\n");
        return ENOENT;
    }
    
    return 0;   
}
