#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <queue>
#include <vector>
#include "serverFuncs.hpp"


using namespace std;

void perror_exit(string);
void process_request(int);
void initilize_workers();

//Queue for all the files that are waiting to be handled
queue<clientData> filesQ;
int buffer_size;
int pool_size;
int queue_size;

//Queue mutex
pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_nonempty;
pthread_cond_t cond_nonefull;

//Mutex for every client (for every socket)
vector<pthread_mutex_t> mutexes;
vector<pair<int, int>> mutex_indexing;



int main(int argc, char* argv[]) {
    
    int port, sock, newsock;
    
    //Checking and passing parameters to variables

    if (argc != 9){  
        perror_exit("Invalid Arguments");
    }

    for (int argument = 1; argument < 9; argument++){

        if (strcmp(argv[argument], "-p") == 0){
            port = atoi(argv[argument + 1]);
        }

        if (strcmp(argv[argument], "-s") == 0) {
            pool_size = atoi(argv[argument + 1]);
        }

        if (strcmp(argv[argument], "-q") == 0) {
            queue_size = atoi(argv[argument + 1]);
        }

        if (strcmp(argv[argument], "-b") == 0) {
            buffer_size = atoi(argv[argument + 1]);
             
        }
        
    }

    //Initialize condition variables
    pthread_cond_init(&cond_nonempty, 0);
    pthread_cond_init(&cond_nonefull, 0);

    struct sockaddr_in server, client;
    socklen_t clientlen;

    struct sockaddr* serverptr = (struct sockaddr*)& server;
    struct sockaddr* clientptr = (struct sockaddr*)& client;

    struct hostent* rem;

    //Creating socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror_exit("Socket");
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    server.sin_port = htons(port);
    //Binding socket to address
    if (bind(sock, serverptr, sizeof(server)) < 0){
        perror_exit("Bind Socket");
    }

    //Listening for connections. Max queue connections is 1000
    if (listen(sock, 1000) < 0){
        perror_exit("Listen");
    }

    cout << "Listening for connections to port " << port << endl;
    
    //Creating worker threads based on the nummber of pool size
    initilize_workers();
    
    int sock_index = 0;
    int* sockets = new int[10];
    int sockets_size = 10;

    while(1){
        
        clientlen = sizeof(client);

        //Accepting new connections
        if ((newsock = accept(sock, clientptr, &clientlen)) < 0){
            perror_exit("Accept Connection");
        }

        //Getting clients name
        if ((rem = gethostbyaddr((char*) &client.sin_addr.s_addr, sizeof(client.sin_addr.s_addr), client.sin_family)) == NULL){
            herror("gethostbyaddr");
            exit(EXIT_FAILURE);
        }


        if (sockets_size == sock_index + 1){
            sockets = (int*) realloc(sockets, 2 * sockets_size * sizeof(int));
            sockets_size *= 2;
        }

        //storing sockets to an array 
        sockets[sock_index] = newsock;

        //Creating new mutex for the spesific client (for spesific socket)
        pthread_mutex_t new_mutex;
        pthread_mutex_init(&new_mutex, NULL);
        pair<int, int> sock_mut_index(newsock, mutex_indexing.size());
        mutex_indexing.push_back(sock_mut_index);
        mutexes.push_back(new_mutex);

        cout << "Accepted connection from " << rem->h_name << endl;

        //Creating new thread of communication worker for every new connection
        pthread_t com_thread;
        pthread_create(&com_thread, 0, communication_thread, &sockets[sock_index]);
        pthread_detach(com_thread);
        sock_index++;

    }
    
    return 0; 
}


//perror --> exit
void perror_exit(string message) {
    perror(message.c_str());
    exit(EXIT_FAILURE);
}


//Spawning all worker threads
void initilize_workers(){

    pthread_t workers[pool_size];
    
    for (int pool = 0; pool < pool_size; pool++) {
        pthread_create(&workers[pool], 0, worker_thread, NULL);
    }

}

