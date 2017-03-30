#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include "ext2.h"
#include "ext2_utils.h"

extern void print_dir_entries(const struct ext2_inode * inode, int aflag);

int main(int argc, char **argv){
    // check for number of arguments see if it is valid
    if (argc != 3 && argc != 4){
        fprintf(stderr, "UsageL: ext2_ls <ext2 file system> <abs path on the file system> \n");
        exit(1);
    }
    
    // sort and characterize all argument information
    int aflag = 0;
    char *filepath;
    char *img_location;
    
    // store the name of file if the given path is a file
    char name[EXT2_NAME_LEN];
    
    // let's see do we have a -a or not
    if(argc == 4){
        if(strncmp(argv[2], "-a", 2) == 0){
            aflag = 1;
        }else{
            fprintf(stderr, "invalid opt arg");
            exit(1);
        }
    }
    
    // assign each pointer with right information
    img_location = argv[1];
    filepath = argv[aflag + 2];
    
    // open the img file and read it
    if (ext2_open(img_location) == -1){
        return ENOENT;    
    }
    
    // find witch directory we need to ls from the root
    char * path = malloc(strlen(filepath));
    unsigned int inode_index = get_inode_index(filepath);
    get_last_entry(filepath,name,path);
    
    if(inode_index){
        // use the index to find the actrual node
        struct ext2_inode * inode = get_inode(inode_index);
        if(inode){
            // see if it is a dir
            if (inode->i_mode & EXT2_S_IFDIR){
                 print_dir_entries(inode, aflag);
            }
            else{
                printf("%s\n", name);
            }
        }
    }
    else{
         printf("no such file or diretory\n");
         return ENOENT;
    }
    return 0;
}

/*
 * print all entry in a given function
 * return void
 */
void print_dir_entries(const struct ext2_inode * inode, int aflag) {
    int i;
    for (i = 0; i < 12; i++) {
        if (inode->i_block[i]) {
            // find the list of content
            const unsigned char * list = disk + (inode->i_block[i]) * EXT2_BLOCK_SIZE;
            struct ext2_dir_entry_2 * dir_entry;
            // use the pointer to find the current position
            const unsigned char * list_ptr = list;
            const unsigned char * list_end = list + EXT2_BLOCK_SIZE;
            while (list_ptr < list_end){
                dir_entry = (struct ext2_dir_entry_2 *)list_ptr;
                if(aflag){
                    if(dir_entry->name_len != 0){
                        printf("%.*s\n",dir_entry->name_len, dir_entry->name);
                    }
                    
                }
                else{
                    if(strncmp(dir_entry->name, "..", dir_entry->name_len) != 0){
                        if(dir_entry->name_len != 0){
                            printf("%.*s\n",dir_entry->name_len, dir_entry->name);
                        }
                    }
                }
                list_ptr += dir_entry->rec_len;
            }
        }
    }
}
