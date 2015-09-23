/*
FTP Client
Joshua Lee (A0111987X)

To compile on sunfire use: g++ -std=c++11 -lsocket -lnsl main.cpp
*/

#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <cstdlib>
#include <strings.h>
#include <stdexcept>

using namespace std;

struct connData {
    struct sockaddr_in serverAddress;
    int socketPort;
    bool connected;
};

connData connectToServer(string server);
string getCurrentDir(int socketPort, string currentDir);
void sendCommand(int socketPort, string command);
char *getReply(int socketPort);
void login(int socketPort, string username, string password);
int enterPassiveMode(int socketPort);
string printDirectory(sockaddr_in serverAddress, int socketPort, bool print);
string changeDirectory(int socketPort, string directory, string currentDir);
void list(sockaddr_in serverAddress, int socketPort, bool isFile);
void listLocalFiles();
void uploadFile(sockaddr_in serverAddress, int socketPort, string fileName);
void downloadFile(sockaddr_in serverAddress, int socketPort, string fileName);

int main() {
    bool quit = false;
    bool connected = false;
    string input = "";
    string currentDir = "";
    int choice = 0;
    struct sockaddr_in serverAddress;
    int socketPort;

    cout << "*** Welcome to some FTP Client ***" << endl;

    while (!quit) {
        cout << "---" << endl;
        if (connected) {
            cout << "You are currently connected. " << endl;
            cout << currentDir;
        } else {
            cout << "You are currently not connected. " << endl;
        }
        cout << "*** Choices: ***" << endl;
        if (!connected) {
            cout << "1. Connect to FTP server" << endl;
            cout << "2. Quit" << endl;
            cout << "Enter option (1 - 2): ";
        } else {
            cout << "1. Disconnect from FTP server" << endl;
            cout << "2. Print remote directory" << endl;
            cout << "3. Change remote directory" << endl;
            cout << "4. List all files in remote directory" << endl;
            cout << "5. List all files in local directory" << endl;
            cout << "6. Upload file to remote directory" << endl;
            cout << "7. Download file from remote directory" << endl;
            cout << "8. Quit" << endl;
            cout << "Enter option (1 - 8): ";
        }
        getline(cin, input);
        stringstream(input) >> choice;
        if (choice != 0) {
            cout << "---" << endl;
            if (connected) {
                switch (choice) {
                    case 1: {
                        cout << "Disconnecting" << endl;
                        close(socketPort);
                        connected = false;
                        break;
                    }
                    case 2: {
                        cout << "Printing remote directory" << endl;
                        printDirectory(serverAddress, socketPort, true);
                        break;
                    }
                    case 3: {
                        cout << "Changing remote directory" << endl;
                        list(serverAddress, socketPort, false);
                        cout << "Enter in directory name: ";
                        string directory;
                        getline(cin, directory);
                        currentDir = changeDirectory(socketPort, directory, currentDir);
                        break;
                    }
                    case 4: {
                        cout << "Listing all files in remote directory" << endl;
                        list(serverAddress, socketPort, true);
                        break;
                    }
                    case 5: {
                        cout << "Listing all files in local directory: " << endl;
                        listLocalFiles();
                        break;
                    }
                    case 6: {
                        cout << "Uploading file to remote directory" << endl;
                        cout << "Listing all files in local directory: " << endl;
                        listLocalFiles();
                        cout << "Enter in file name: ";
                        string fileName;
                        getline(cin, fileName);
                        uploadFile(serverAddress, socketPort, fileName);
                        break;
                    }
                    case 7: {
                        cout << "Downloading file from remote directory" << endl;
                        cout << "Listing all files in remote directory" << endl;
                        list(serverAddress, socketPort, true);
                        cout << "Enter in file name: ";
                        string fileName;
                        getline(cin, fileName);
                        downloadFile(serverAddress, socketPort, fileName);
                        break;
                    }
                    case 8: {
                        cout << "Quitting..." << endl;
                        close(socketPort);
                        quit = true;
                        break;
                    }
                    default: {
                        cout << "Unknown choice!" << endl;
                        break;
                    }
                }
            } else {
                switch (choice) {
                    case 1: {
                        cout << "Enter in server address: ";
                        string address;
                        getline(cin, address);
                        cout << "Enter in username (leave blank for anonymous login): ";
                        string username;
                        getline(cin, username);
                        cout << "Enter in password (leave blank for anonymous login): ";
                        string password;
                        getline(cin, password);
                        cout << "Connecting" << endl;
                        connData connectionData = connectToServer(address);
                        serverAddress = connectionData.serverAddress;
                        socketPort = connectionData.socketPort;
                        connected = connectionData.connected;
                        if (connected) {
                            login(socketPort, username, password);
                            currentDir = getCurrentDir(socketPort, currentDir);
                            printf("Connected to %s:%d\n",
                                   inet_ntoa(serverAddress.sin_addr), ntohs(serverAddress.sin_port));
                        } else {
                            cout << "Unable to connect to server" << endl;
                        }
                        break;
                    }
                    case 2: {
                        cout << "Quitting..." << endl;
                        quit = true;
                        break;
                    }
                    default: {
                        cout << "Unknown choice!" << endl;
                        break;
                    }
                }
            }
        } else {
            cout << "Invalid choice!" << endl;
        }
    }
    exit(0);
}

connData connectToServer(string server) {
    struct hostent *host = gethostbyname(server.c_str());
    int socketPort;
    struct sockaddr_in serverAddress;
    if ((socketPort = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(21); // FTP port
    serverAddress.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(serverAddress.sin_zero),8);
    if (connect(socketPort, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }
    printf("Server Response: %s", getReply(socketPort));
    connData connectionData;
    connectionData.serverAddress = serverAddress;
    connectionData.socketPort = socketPort;
    connectionData.connected = true;
    return connectionData;
}

string getCurrentDir(int socketPort, string currentDir) {
    sendCommand(socketPort, "TYPE A");
    printf("Server Response: %s", getReply(socketPort));
    sendCommand(socketPort, "PWD");
    string reply = string(getReply(socketPort));
    cout << "Server response: " << reply;
    if (reply.compare(0, 3, "257") == 0) {
        return reply.substr(reply.find("\"/"));
    } else {
        return currentDir;
    }
}

void sendCommand(int socketPort, string command) {
    command += "\r\n"; // end off commands properly
    char send_data[1024];
    strncpy(send_data, command.c_str(), sizeof(send_data));
    send_data[sizeof(send_data) - 1] = 0;
    send(socketPort, send_data, strlen(send_data), 0);
}

char *getReply(int socketPort) {
    char* receivedData = new char[1024];
    int bytesReceived = (int) recv(socketPort,receivedData, 1024, 0);
    if (bytesReceived < 0) {
        cout << "Error reading from socket" << endl;
    } else {
        receivedData[bytesReceived] = '\0';
    }
    return receivedData;
}

void login(int socketPort, string username, string password) {
    if (username.compare("") == 0) {
        username = "anonymous";
    }
    if (password.compare("") == 0) {
        username = "anonymous";
    }
    sendCommand(socketPort, "USER " + username);
    printf("Server Response: %s", getReply(socketPort));
    sendCommand(socketPort, "PASS " + password);
    printf("Server Response: %s", getReply(socketPort));
}

int enterPassiveMode(int socketPort) {
    int dataPort = 0;
    sendCommand(socketPort, "EPSV");
    string replyString = string(getReply(socketPort));
    cout << "Server response: " << replyString;
    long frontPosition = replyString.find("(|||");
    long backPosition = replyString.find("|)");
    try {
        dataPort = stoi(replyString.substr(frontPosition + 4, backPosition - frontPosition - 4));
    } catch (const invalid_argument& e) {
        cout << "Could not get data port from server" << endl;

    }
    return dataPort;
}

string printDirectory(sockaddr_in serverAddress, int socketPort, bool print) {
    int dataSocket;
    if ((dataSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    int dataPort = enterPassiveMode(socketPort);
    struct sockaddr_in dataServerAddress;
    dataServerAddress.sin_family = AF_INET;
    dataServerAddress.sin_port = htons(dataPort);
    dataServerAddress.sin_addr = serverAddress.sin_addr;
    bzero(&(dataServerAddress.sin_zero),8);
    if (connect(dataSocket, (struct sockaddr *)&dataServerAddress, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }
    string directoryList = "";
    sendCommand(socketPort, "LIST");
    printf("Server Response: %s", getReply(socketPort));
    string reply = getReply(dataSocket);
    directoryList += reply;
    while (reply.compare("") != 0) {
        if (print) {
            cout << reply;
        }
        reply = getReply(dataSocket);
        directoryList += reply;
    }
    close(dataSocket);
    printf("Server Response: %s", getReply(socketPort));
    return directoryList;
}

string changeDirectory(int socketPort, string directory, string currentDir) {
    sendCommand(socketPort, "CWD " + directory);
    getReply(socketPort);
    return getCurrentDir(socketPort, currentDir);
}

void list(sockaddr_in serverAddress, int socketPort, bool isFile) {
    string replyString = printDirectory(serverAddress, socketPort, false);
    char reply[replyString.length()];
    strncpy(reply, replyString.c_str(), sizeof(reply));
    reply[sizeof(reply) - 1] = 0;
    char * file = strtok(reply, "\n");
    cout << "List of file(s): " << endl;
    while (file != NULL) {
        string filename = string(file);
        if (isFile) {
            if (filename.compare(0, 1, "d") != 0 && filename.length() > 50) {
                cout << filename.substr(49) << endl;
            }
        } else {
            if (filename.compare(0, 1, "d") == 0 && filename.length() > 50) {
                cout << filename.substr(49) << endl;
            }
        }
        file = strtok(NULL, "\n");
    }
}

void listLocalFiles() {
    DIR* dir = opendir(".");
    if (!dir) {
        perror("opendir");
        exit(1);
    }
    struct dirent* entry;
    struct stat dir_stat;
    while ((entry = readdir(dir)) != NULL) {
        stat(entry->d_name, &dir_stat);
        if (!S_ISDIR(dir_stat.st_mode)) {
            printf("%s\n", entry -> d_name);
        }
    }
    if (closedir(dir) == -1) {
        perror("closedir");
        exit(1);
    }
}

void uploadFile(sockaddr_in serverAddress, int socketPort, string fileName) {
    ifstream file;
    file.open(fileName, ios::binary|ios::in);
    if (file.is_open()) {
        int dataSocket;
        if ((dataSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(1);
        }
        int dataPort = enterPassiveMode(socketPort);
        struct sockaddr_in dataServerAddress;
        dataServerAddress.sin_family = AF_INET;
        dataServerAddress.sin_port = htons(dataPort);
        dataServerAddress.sin_addr = serverAddress.sin_addr;
        bzero(&(dataServerAddress.sin_zero),8);
        if (connect(dataSocket, (struct sockaddr *)&dataServerAddress, sizeof(struct sockaddr)) == -1) {
            perror("connect");
            exit(1);
        }
        sendCommand(socketPort, "TYPE I");
        printf("Server Response: %s", getReply(socketPort));
        sendCommand(socketPort, "STOR " + fileName);
        printf("Server Response: %s", getReply(socketPort));
        char buffer[2];
        while(file.read((char *)&buffer, sizeof(buffer))) {
          send(dataSocket, (char *)&buffer, sizeof(buffer), 0);
        }
        close(dataSocket);
        file.close();
        printf("Server Response: %s", getReply(socketPort));
    } else {
        cout << "Cannot open file" << endl;
    }
}

void downloadFile(sockaddr_in serverAddress, int socketPort, string fileName) {
    ofstream file;
    file.open(fileName, ios::binary|ios::out);
    if (file.is_open()) {
        int dataSocket;
        if ((dataSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(1);
        }
        int dataPort = enterPassiveMode(socketPort);
        struct sockaddr_in dataServerAddress;
        dataServerAddress.sin_family = AF_INET;
        dataServerAddress.sin_port = htons(dataPort);
        dataServerAddress.sin_addr = serverAddress.sin_addr;
        bzero(&(dataServerAddress.sin_zero),8);
        if (connect(dataSocket, (struct sockaddr *)&dataServerAddress, sizeof(struct sockaddr)) == -1) {
            perror("connect");
            exit(1);
        }
        sendCommand(socketPort, "TYPE I");
        printf("Server Response: %s", getReply(socketPort));
        sendCommand(socketPort, "RETR " + fileName);
        printf("Server Response: %s", getReply(socketPort));
        char receivedData[1024];
        int bytesReceived;
        while ((bytesReceived = (int) recv(dataSocket,receivedData, 1024, 0)) > 0) {
            file.write((char *)&receivedData, bytesReceived);
        }
        close(dataSocket);
        file.close();
        printf("Server Response: %s", getReply(socketPort));
    } else {
        cout << "Cannot open file" << endl;
    }
}
