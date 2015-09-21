#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

using namespace std;

struct connData {
    struct sockaddr_in serverAddress;
    int socketPort;
};

connData connectToServer(string server);

int main() {
    bool quit = false;
    bool connected = false;
    string input = "";
    int choice = 0;
    struct sockaddr_in serverAddress;
    int socketPort;

    while (!quit) {
        cout << "*** Welcome to some FTP Client ***" << endl;
        if (connected) {
            cout << "You are currently connected. " << endl;
        } else {
            cout << "You are currently not connected. " << endl;
        }
        cout << "*** Choices: ***" << endl;
        cout << "1. Connect to FTP server" << endl;
        cout << "7. Quit" << endl;
        cout << "Please enter in your choice: ";
        getline(cin, input);
        stringstream(input) >> choice;
        if (choice != 0) {
            cout << "Your choice is: " << choice << endl;
            switch (choice) {
                case 1: {
                    cout << "Connecting" << endl;
                    connData connectionData = connectToServer("");
                    serverAddress = connectionData.serverAddress;
                    socketPort = connectionData.socketPort;
                    connected = true;
                    printf("I am connected to %s:%d. \n",
                           inet_ntoa(serverAddress.sin_addr),ntohs(serverAddress.sin_port));
                    break;
                }
                case 7: {
                    cout << "Quitting..." << endl;
                    quit = true;
                    break;
                }
                default: {
                    cout << "Unknown choice!" << endl;
                    break;
                }
            }
        } else {
            cout << "Invalid choice!" << endl;
        }
    }

}

connData connectToServer(string server) {
    server = "home.jolly.sg";
    struct hostent *host = gethostbyname(server.c_str());
    int socketPort;
    struct sockaddr_in serverAddress;

    //create a Socket structure   - "Client Socket"
    if ((socketPort = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(21); // FTP port
    serverAddress.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(serverAddress.sin_zero),8);

    //connect to server at port 5000
    if (connect(socketPort, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }

    char receivedData[1024];
    int bytesReceived;
    bytesReceived = (int) read(socketPort,receivedData, 1024);
    if (bytesReceived < 0) {
        cout << "Error reading from socket" << endl;
    } else {
        receivedData[bytesReceived] = '\0';
        printf("Server Response: %s", receivedData);
    }

    connData connectionData;
    connectionData.serverAddress = serverAddress;
    connectionData.socketPort = socketPort;
    return connectionData;
}