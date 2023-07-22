//clientData struct is used to store server path and client path that may be diffrent. And also stores spesific socket.

struct clientData {
	char* server_path;
	char* client_path;
	int sock;
};


void* communication_thread(void* sock);

void* worker_thread(void* sock);