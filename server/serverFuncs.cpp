#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <dirent.h>
#include <netinet/in.h>
#include <pthread.h>
#include <queue>
#include <sys/types.h>
#include <unistd.h>
#include <string>

using namespace std;


struct clientData {
  char *server_path;
  char* client_path;
  int sock;
};


extern queue<clientData> filesQ;
extern int buffer_size;
extern int pool_size;
extern int queue_size;
extern pthread_mutex_t mutex_queue;
extern pthread_cond_t cond_nonempty;
extern pthread_cond_t cond_nonefull;
extern vector<pair<int, int>> mutex_indexing;
extern vector<pthread_mutex_t> mutexes;



//Getting specific mutex based on socket number 
int get_mutex_index(int socket){

  for (int i = 0; i < mutex_indexing.size(); i++){
    if (mutex_indexing[i].first == socket){
        return mutex_indexing[i].second;
    }
  }
    return -1;
} 


//Counting total bytes of a file
int count_bytes(char *filename) {

    int bytes;
    FILE* file;

    file = fopen(filename, "rb");
    //Going to end of file
    fseek(file, 0, SEEK_END);

    //Getting total bytes
    bytes = ftell(file);

    fclose(file);

    return bytes;
}


//Getting path based on clients direcory file system
char* connect_client_path(char* server_path, char* client_path){

    char* con_path;
    while ((con_path = strstr(server_path, client_path)) != NULL){
        server_path = con_path + strlen(client_path);
    }
    return server_path;
}


//This recursive function has 2 purposes
//To find to number of files that will be copied to client
//To push into the queue the files that must be copied
int copy_files(int sock, char buffer[], char* client_dir, int total_files, bool save){
    
    struct dirent* diren;
    DIR* directory = opendir(buffer);
    
    if (directory == NULL){
        perror("Dir open failed");
    }

    while ((diren = readdir(directory)) != NULL){
      
        //If the file type is directory then call function again for the inside directory
        if (diren->d_type == DT_DIR){
            if (strcmp(diren->d_name, ".") && strcmp(diren->d_name, "..")){
                char *next_dir = new char[strlen(buffer) + strlen(diren->d_name) + strlen("/") + 1];
                strcpy(next_dir, buffer);
                strcat(next_dir, "/");
                strcat(next_dir, diren->d_name);
                total_files = copy_files(sock, next_dir, client_dir, total_files, save);
                delete [] next_dir;
            } 
        }
        else{   //if the file type is not directory then add it into the queue (if the queue has space)
            
            //also counting total bytes. Only if boolean save = true then this function is executed to also push into queue
            total_files++;

            if (save){

                clientData data;
                data.server_path = new char[strlen(buffer) + strlen(diren->d_name) + strlen("/") + 1];
                data.client_path = new char[strlen(buffer) + strlen(diren->d_name) + strlen(client_dir) + strlen("/") + 1];
                strcpy(data.server_path, buffer);
                strcat(data.server_path, "/");
                strcat(data.server_path, diren->d_name);

                strcpy(data.client_path, client_dir);
                strcat(data.client_path, connect_client_path(buffer, client_dir));
                strcat(data.client_path, "/");
                strcat(data.client_path, diren->d_name);

                data.sock = sock;

                //Locking mutex for the queue
                pthread_mutex_lock(&mutex_queue);

                //If queue does not have space then waiting in condition variable
                while (filesQ.size() >= queue_size){
                    pthread_cond_wait(&cond_nonempty, &mutex_queue);
                }

                printf("[Thread: %zu]: Adding file %s to the queue...\n",pthread_self(), data.server_path);
                filesQ.push(data);
                pthread_cond_broadcast(&cond_nonefull); //Signaling that the queue has an element ready to be handled
                pthread_mutex_unlock(&mutex_queue);

            }
        }
    }

    closedir(directory);

    return total_files;
}

//This is the communication thread
void *communication_thread(void* sock) {

    int socket = *(int *)sock;

    char copy_dir_buffer[4096];

    read(socket, copy_dir_buffer, 4096);

    DIR* server_dir;
    server_dir = opendir(copy_dir_buffer);

    if (!server_dir){
        printf("[Thread: %zu]: Directory %s does not exist on the Server\n",pthread_self(), copy_dir_buffer);
        return 0;
    }

    closedir(server_dir);


    if (copy_dir_buffer[strlen(copy_dir_buffer) - 1] == '/') {
        copy_dir_buffer[strlen(copy_dir_buffer) - 1] = 0;
    }
    
    char client_dir[strlen(copy_dir_buffer)];
    strcpy(client_dir, copy_dir_buffer);
    char* client_path = client_dir;
   
    char splitted[2] = "/";

    char* get_path = strtok_r(client_path, splitted, &client_path);
    char* client_dir_copy = get_path;

    while (get_path != NULL) {
        client_dir_copy = get_path;  //directory of client may be diffrent than servers
        get_path = strtok_r(client_path, splitted, &client_path);
    }

    //Calling copy_files function to count the number of files that will be copied to the client
    int total_files = copy_files(socket, copy_dir_buffer, client_dir_copy, 0, false);
    
    string transfer_rate = to_string(buffer_size);
    string files_number = to_string(total_files);

    string metadata = transfer_rate + " " + files_number;

    //Sending to client the tranfer rate that are going to communicate and the number of files
    write(socket, metadata.c_str(), 20);

    printf("[Thread: %zu]: About to scan directory %s\n",pthread_self(), copy_dir_buffer);

    //Now calling copy_files function to push files into the queue
    copy_files(socket, copy_dir_buffer, client_dir_copy, 0, true);

    return 0;
}

//This function sends all the data of a specific file to the client
void send_file_content(int socket, int blocksize, clientData& file) {

	FILE* fp = fopen(file.server_path, "rb");  //Reading in binary mode to support all types of files
    

    printf("[Thread: %zu]: About to read file %s\n",pthread_self(), file.server_path);
	
    char read_file[blocksize];
    memset(read_file, 0, blocksize);

    uint32_t bytes;

	//Writing all the content of the file to the socket with size blocksize
    while (!feof(fp)) {

        bytes = fread(read_file, 1, sizeof(read_file), fp);
        uint32_t b_send = htonl(bytes);
     
        write(socket, &b_send, sizeof(b_send));

        write(socket, read_file, blocksize);
        
        memset(read_file, 0, blocksize);

	}

    delete [] file.server_path;
    delete [] file.client_path;
    
	fclose(fp);
}

//Worker thread
void *worker_thread(void* args) {    

    char read_file[4096];

	while (true) {
        clientData data;

        //Checking if queue has any files
        pthread_mutex_lock(&mutex_queue);
        while(filesQ.size() <= 0){
            pthread_cond_wait(&cond_nonefull, &mutex_queue);
        }

        //If the queue has a file then pop it 
        data = filesQ.front();
        filesQ.pop();

        pthread_mutex_unlock(&mutex_queue);
        pthread_cond_signal(&cond_nonempty); //Signal that the queue has some space 

        printf("[Thread: %zu]: Received task: <%s, %d>\n",pthread_self(), data.server_path, data.sock);

        memset(read_file, 0, 4096);

        int bytes = count_bytes(data.server_path);

        int metadata_size = bytes;
        strcpy(read_file, to_string(metadata_size).c_str());
        strcat(read_file, " ");
        strcat(read_file, data.client_path);

        int index = get_mutex_index(data.sock);

        pthread_mutex_lock(&mutexes[index]);

        //Sending metadata to the client before sending the data of the file
        //Sending bytes size of the file and the client path that should be created
        write(data.sock, read_file, 4096);

        //Send content of the file to client
        if (bytes > 0){
            send_file_content(data.sock, buffer_size, data);
        }
        else{
            delete [] data.server_path;
            delete [] data.client_path;
        }
        pthread_mutex_unlock(&mutexes[index]);
   
    }

    return 0;
}

