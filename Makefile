#Compiles Server and Client. Server and Client are ready to run then.

all:
	g++ -o ./server/dataServer ./server/dataServer.cpp ./server/serverFuncs.cpp -lpthread
	g++ -o ./client/remoteClient ./client/remoteClient.cpp ./client/clientFuncs.cpp