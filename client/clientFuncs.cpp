#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <queue>
#include <string>
#include <sys/stat.h>
#include <unistd.h>


using namespace std;

//Getting total bytes to read from server metadata
int get_bytes_to_read(char* files_metadata){

    char metadata[strlen(files_metadata)];
    strcpy(metadata, files_metadata);
    char splitted[2] = " ";
    char* meta = strtok(metadata, splitted);

    return atoi(meta);
}

//Creates a spesific directory
void create_directory(char* filename){
    
    char path_counter[strlen(filename)];
    strcpy(path_counter, filename);
    char splitted[2] = "/";
    
    int counter = 0;
    
    char* name = strtok(path_counter, splitted);
    counter++;

    while (name != NULL) {
        name = strtok(NULL, splitted);
        if (name != NULL)
            counter++;
    }
    
    char path_name[strlen(filename)];
    strcpy(path_name, filename);
    char whole_path[strlen(filename)];
    memset(whole_path, 0, strlen(filename));

    name = strtok(path_name, splitted);
    strcpy(whole_path, name);
    strcat(whole_path, "/");
    mkdir(whole_path, 0777);

    
    for (int i = 1; i <= counter - 1; i ++){
        name = strtok(NULL, splitted);
        strcat(whole_path, name);
        strcat(whole_path, "/");
        mkdir(whole_path, 0777);
    }

}

//Getting name of firectory from the metadata that was sent
char* get_directory(char* files_metadata){

    char metadata[strlen(files_metadata)];
    strcpy(metadata, files_metadata);

    char splitted[2] = " ";
    char *token = strtok(metadata, splitted);
    token = strtok(NULL, splitted);
    
    char* directory = new char[strlen(token) + 1];

    for (int i = strlen(token); i > 0; i--){
        if (token[i] != '/'){
            token[i] = 0;
        }
        else{
            break;
        }
    }

    strcpy(directory, token);

    return directory;
}

//Getting filename from metadata
char* get_file_name(char* files_metadata){

    char metadata[strlen(files_metadata)];
    strcpy(metadata, files_metadata);
    char splitted[2] = " ";
    char *meta = strtok(metadata, splitted);
    meta = strtok(NULL, splitted);

    char *filename = new char[strlen(meta) + 1];
    strcpy(filename, meta);

    return filename;
}

//Creating spesific file. If this file already exists then it is deleted and created again
void create_file(char* filename){

    unlink(filename);
    
    FILE* file;
    file = fopen(filename, "wb");
    fclose(file);

}

//Writing data to a spesific file. Data are coming from server
void write_to_file(char* filename, char* data, int block_size){

    FILE* file;

    file = fopen(filename, "ab");

    if (file == NULL){
        perror("Error Writing to file");
        exit(EXIT_FAILURE);
    }

    fwrite(data, 1, block_size, file);
    fclose(file);

}

//Getting blocksize that the server client is going to communicate with
int get_block_size(char* buffer){

    char metadata[strlen(buffer)];
    strcpy(metadata, buffer);
    char splitted[2] = " ";
    char* meta = strtok(metadata, splitted);

    return atoi(meta);
}

//Getting expected total files from metadata
int get_files_number(char* buffer){

    char metadata[strlen(buffer)];
    strcpy(metadata, buffer);
    char splitted[2] = " ";
    char* meta = strtok(metadata, splitted);
    meta = strtok(NULL, splitted);

    return atoi(meta);
}