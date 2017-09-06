# FileTransferTCP

	This program implements a client-server app that transfers a file over a TCP connection.

    Implemented client/server to be able to deliver a file over a BSD
    socket connection. The client connects to the server and begins
    sending the file in chunks until it is done reading. The server
    creates a new thread for the connection with the client and begins
    receiving the chunks of data and writing them to a file. A new thread
    is spawned for each client that is connected. The server continues to wait
    for clients to connect until it is terminated by a signal or error.
	
	The server accepts 2 arguments.
	
	To run the server:
	./server portNumber directory
	
	The client accepts 3 arguments.
	
	To run the client:
	./client IP portNumber fileToTransfer