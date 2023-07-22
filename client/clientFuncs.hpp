#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <queue>
#include <string>
#include <unistd.h>


int get_bytes_to_read(char* files_metadata);

void create_directory(char* filename);

char* get_directory(char* files_metadata);

char *get_file_name(char* files_metadata);

void create_file(char* filename);

void write_to_file(char* filename, char* data, int blocksize);

int get_files_number(char* buffer);

int get_block_size(char* buffer);