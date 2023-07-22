#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include "clientFuncs.hpp"


using namespace std;
#define SOCKET_BUFFER 4096


void perror_exit(string);

int main(int argc, char *argv[]) {

    //Checking and passing arguments to variables
    if (argc != 7){
        perror_exit("Invalid Arguments");
    }

    char ip_address[16];
    char directory[4096];
    int port;

    for (int argument = 1; argument < 7; argument++) {

        if (strcmp(argv[argument], "-i") == 0) {
            strcpy(ip_address, argv[argument + 1]);
        }

        if (strcmp(argv[argument], "-p") == 0) {
            port = atoi(argv[argument + 1]);
        }

        if (strcmp(argv[argument], "-d") == 0) {
            strcpy(directory, argv[argument + 1]);
        }

    }

    int sock;

    struct sockaddr_in server;
    struct sockaddr* serverptr = (struct sockaddr *)&server;

    struct hostent* rem;
    
    //Creating a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror_exit("Socket");
    }
    
    //Getting server address
    if ((rem = gethostbyname(ip_address)) == NULL){
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }
    
    server.sin_family = AF_INET;
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(port);

    //Connnect to server
    if (connect(sock, serverptr, sizeof(server)) < 0){
        perror_exit("Connection Failed");
    }

    cout << "Connecting to " << ip_address << " on port " << port << endl;

    //Send desired directory
    if (write(sock, directory, SOCKET_BUFFER) < 0){
        perror_exit("Writing Socket Failure");
    }

    //Getting metadata of server
    char init_metadata[20];
    read(sock, init_metadata, 20);
    
    //Metadata block size
    int block_size = get_block_size(init_metadata);
    
    //Metadata expected files from server
    int expected_files = get_files_number(init_metadata);  
    
    char files_metadata[4096];
    char buffer[block_size];

    int files = 0;
    
    while(1){

        memset(buffer, 0, block_size);
        memset(files_metadata, 0, 4096);
        
        //For every file sever sends some metadata first
        read(sock, files_metadata, 4096);
        
        files++;

        if (files > expected_files){
            break;
        }

        //getting total bytes to read metadata
        int total_bytes = get_bytes_to_read(files_metadata);

        //getting directory from metadata 
        char *makedir = get_directory(files_metadata);
        
        //creating the directory
        create_directory(makedir);
        
        //getting filename from metadata
        char *filename = get_file_name(files_metadata);

        cout << "Received: " << filename << endl;

        //creating the file
        create_file(filename);


        //if total bytes is 0 then there is no data to write in the file
        if (total_bytes == 0){
            delete [] makedir;
            delete[] filename;

            if ((files + 1) > expected_files) {
              break;
            }

            continue;
        }
        
        int bytes = 0;
        int sum_bytes = 0;
        int counter = 0;

        uint32_t ntohl_num, get_bytes;

        char to_write[block_size];
        memset(to_write, 0, block_size);
        //reading number of bytes of data that the server is going to send
        while(read(sock, &get_bytes, sizeof(get_bytes)) > 0){

            ntohl_num = ntohl(get_bytes);
            
            bytes = ntohl_num;

            if (bytes > 0){

                //Reading actual file content data from server
                int has_read;
                int total_read = 0;

                //Checking if Client got all data that expected
                while((has_read = read(sock, buffer, block_size - total_read)) > 0){
                    
                    memcpy(to_write + total_read, buffer, has_read);
                    total_read += has_read;
                    
                    if (total_read >= bytes){
                        break;
                    }

                    memset(buffer, 0, block_size);
                }
                
                //Then writing the content to the clients file
                write_to_file(filename, to_write, bytes);
                sum_bytes += bytes;
                counter++;

                
                //If total bytes that was send == total bytes from metadta 
                //that was sent from server then continue to the next file if exists
                if (sum_bytes >= total_bytes){
                    break;
                }
            }

            memset(to_write, 0, block_size);
            memset(buffer, 0, block_size);
        }

        delete [] makedir;
        delete[] filename;

        if((files + 1) > expected_files) {
            break;
        }
    
    
    }
    
    return 0;
}


//Perror --> exit
void perror_exit(string message){
    perror(message.c_str());
    exit(EXIT_FAILURE);
}