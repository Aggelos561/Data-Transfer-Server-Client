# Data Transfer - Server and Client

This repository contains a data transfer system implemented through a server and client application. The project is designed to efficiently transfer files from the server to the client, enabling seamless data communication between the two components. The system is specifically designed for reliable data transmission, ensuring that files are sent and received accurately and efficiently.

## How It Works

### Server

The server application is responsible for handling client connections and efficiently transferring files to connected clients. Upon execution, the server initializes a socket and binds it to the default IP address of the system. It then listens for incoming client connections on the specified port.

The server is designed to handle multiple client connections concurrently. It uses a thread pool, allowing it to create a fixed number of worker threads. These worker threads wait for file tasks to be pushed into the server's queue. When a client connects and requests a file transfer, the server creates a communication thread for that client. The communication thread is responsible for reading the client's requested directory and other essential metadata.

As the communication thread reads the client's request, it pushes new file tasks into the server's queue. The worker threads in the thread pool then process these tasks and send the requested files to the respective clients.

### Client

The client application connects to the server using the specified IP address and port number. Upon successful connection, the client reads essential metadata from the server, including the block size for data transfer and the total number of files to expect.

For each file received from the server, the client reads the metadata, which includes the total number of bytes for the file and the file's name. The client then creates the specified directory (extracted from the filename) if it does not already exist. If the file already exists, the client deletes it and recreates it.

The client then reads the data from the server in blocks of the specified block size. It writes the received data to the corresponding file. This process continues until all files are received from the server.

## Getting Started

To compile the entire project (both Server and Client), execute the following command in root project directory:

```bash
make all
```

To compile only the Server, navigate to the `./server/` directory and run:

```bash
make server
```

To compile only the Client, navigate to the `./client/` directory and run:

```bash
make client
```

## Executing the Server

To execute the Server with default arguments, go to the `./server/` directory and run:

```bash
make runServer
```

Alternatively, you can customize the Server's execution by specifying the following arguments:

```bash
./dataServer -p <port> -s <thread_pool_size> -q <queue_size> -b <block_size>
```

- `<port>`: The port number on which the Server will listen for incoming client connections.
- `<thread_pool_size>`: The number of threads in the Server's thread pool, determining the maximum number of clients that can be served concurrently.
- `<queue_size>`: The maximum number of file tasks that can be held in the server's queue before the worker threads start processing them.
- `<block_size>`: The size of data blocks that are transferred between the Server and Client.

## Executing the Client

To run the Client with default arguments, go to the `./client/` directory and use the following command:

```bash
make runClient
```

You can also customize the Client's execution by providing the following arguments:

```bash
./remoteClient -i <server_ip> -p <server_port> -d <directory>
```

- `<server_ip>`: The IP address of the Server to which the Client will connect.
- `<server_port>`: The port number on which the Server is listening for incoming connections.
- `<directory>`: The directory path (relative to the server's side) from which the Client requests files to be transferred.

The files and directories received from the Server will be copied into the `./client/` directory.

## Note

The project implements various efficient data transfer techniques, such as using a thread pool to handle multiple clients and using a queue to manage file tasks for worker threads. Additionally, the implementation is designed to handle relative paths correctly, ensuring smooth execution on both the server and client sides.

The data transfer system provides a reliable way to transfer files between a server and multiple clients, making it suitable for various applications where efficient and secure data communication is essential.
