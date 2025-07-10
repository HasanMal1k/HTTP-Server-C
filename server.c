#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>     
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  

main(){

    // Building the main socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;

    // Making sure server_addr is empty before giving it's attributes values
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // All the IP and Port stuff configured, now we will bind with the socket
    bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    // sockaddr extends to sockaddr_in, bind only accepts that so we'll point it to the
    // address of our sockaddr_in

    listen(server_socket, 10);

    while(1){
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        // Our server kinda stops here, it'll only continue when the client connects to it 
        //and then server accepts
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);

        if(client_socket < 0){
            perror("Accept Failed");
            continue;
        }

        pid_t pid = fork();

        // Creates child process here,
        // pid == 0; Child Process
        // pid > 0 ; Parent Process
        // pid < 0 ; Error

        if(pid == 0){
            // In the child process we don't need our main socket anymore so we'll close it
            close(server_socket);

            // Function for handling the client
            handle_client(client_socket);

            // After function executes, we'll close client socket aswell
            close(client_socket);

            // Exit Loop
            exit(0);
        }
        else{
            // The parent won't be handling the clients so we can close the client server
            close(client_socket);
        }


    }



    return 0;
}