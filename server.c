#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>     
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  

int main(){

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

// passing an in because file descriptors, unix jargon
void handle_client(int client_socket) {
    // Buffer to receive HTTP request
    char buffer[4096];
    recv(client_socket, buffer, sizeof(buffer), 0);  // Read request from socket
    printf("Request:\n%s\n", buffer);

    // Variables to hold method (e.g., GET) and path (e.g., /index.html)
    char method[8], path[1024];
    sscanf(buffer, "%s %s", method, path);  // Parse method and path from the request line

    // Only support GET requests
    if (strcmp(method, "GET") != 0) {
        send(client_socket, "HTTP/1.1 405 Method Not Allowed\r\n\r\n", 36, 0);
        return;
    }

    // Map "/" to "index.html", otherwise prepend "." to path
    // We're basically checking what file to send here, default is index.html
    char filepath[1024];
    if (strcmp(path, "/") == 0)
        strcpy(filepath, "index.html");
    else
        snprintf(filepath, sizeof(filepath), ".%s", path);  // e.g., "/about.html" → "./about.html"

    // Open the requested file
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        // If file not found, return 404 Not Found
        char *not_found = "<h1>404 Not Found</h1>";
        char response[1024];
        sprintf(response,
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %ld\r\n\r\n%s",
                strlen(not_found), not_found);
        send(client_socket, response, strlen(response), 0);
        return;
    }

    // Actual process when file is available

    // Get file size
    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Allocate memory and read file into memory
    char *filedata = malloc(filesize + 1);
    fread(filedata, 1, filesize, fp);
    filedata[filesize] = '\0';  // Null-terminate the string
    fclose(fp);

    // Create and send HTTP response headers
    char header[1024];
    sprintf(header,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %ld\r\n\r\n", filesize);
    send(client_socket, header, strlen(header), 0);

    // Send the file content as the response body to the browser where it can be rendered
    send(client_socket, filedata, filesize, 0);

    // Free allocated memory
    free(filedata);
}
