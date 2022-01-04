#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <string>
#include <set>
#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>
using namespace std;

#define localhost "127.0.0.1" //localhost address
#define serverTUDP 21994 //UDP of serverT
#define serverSUDP 22994 //UDP of serverS
#define serverPUDP 23994 //UDP of serverP
#define UDPPort 24994 //UDP of serverC
#define TCPPortA 25994 //TCP port for clientA
#define TCPPortB 26994 //TCP port for clientB
#define MAXDATASIZE 1024 //max number of bytes for input data
#define MAXBUFFERSIZE 40000 //max number of bytes for received data

int sockfd_A; //socket for clientA
struct sockaddr_in clientAaddr; //address of clientA
int sockfd_B; //socket for clientB
struct sockaddr_in clientBaddr; //address of clientB
int sockfd_UDP; //socket for serverC UDP
struct sockaddr_in UDPaddr; //address of serverC
int socket_A; //child socket for clientA
struct sockaddr_in address_A; //address of childA
int socket_B; //child socket for clientB
struct sockaddr_in address_B; //address of childB
struct sockaddr_in address_P; //address of serverP
struct sockaddr_in address_S; //address of serverS
struct sockaddr_in address_T; //address of serverT
char Username_A[MAXDATASIZE]; //char[] to store username from clientA
char Username_B[MAXDATASIZE]; //char[] to store username from clientB
char serverTInput[MAXBUFFERSIZE]; //char[] to store result from serverT
char serverSInput[MAXBUFFERSIZE]; //char[] to store result from serverS
char serverPInput_Score[MAXBUFFERSIZE]; //char[] to store score result from serverP
char serverPInput_Path[MAXBUFFERSIZE]; //char[] to store path result from serverP
string senderData; //string to store result to clientA and B
struct sockaddr_in serverT_Address; //UDP address for serverT
struct sockaddr_in serverS_Address; //UDP address for serverS
struct sockaddr_in serverP_Address; //UDP address for serverP
socklen_t serverT_size; //size of serverT address
socklen_t serverS_size; //size of serverS address
socklen_t serverP_size; //size of serverP address
set<string> nodeSet; //set to store unique node from serverT data
vector<string> setVector; //vector to store set info
string sendbuffer; //string to store given node from clientA and B

//initialize Socket for clientA
// source Beej's guide: https://beej.us/guide/bgnet/html/
void initializeSocket_clientA() {
    sockfd_A = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd_A == -1) {
        perror("Client A Socket Failed!");
        exit(1);
    }
    memset(clientAaddr.sin_zero, '\0', sizeof  clientAaddr.sin_zero);
    clientAaddr.sin_family = AF_INET;
    clientAaddr.sin_port = htons(TCPPortA);
    clientAaddr.sin_addr.s_addr = inet_addr(localhost);
    if (::bind(sockfd_A, (struct sockaddr*) &clientAaddr, sizeof(clientAaddr)) == -1) {
        perror("Client A Bind Failed!");
        exit(1);
    }
    if (listen(sockfd_A, 5) == -1) {
        perror("Client A Listen Failed!");
        exit(1);
    }
}
//initialize Socket for clientB
// source Beej's guide: https://beej.us/guide/bgnet/html/
void initializeSocket_clientB() {
    sockfd_B = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd_B == -1) {
        perror("Client B Socket Failed!");
        exit(1);
    }
    memset(clientBaddr.sin_zero, '\0', sizeof  clientBaddr.sin_zero);
    clientBaddr.sin_family = AF_INET;
    clientBaddr.sin_port = htons(TCPPortB);
    clientBaddr.sin_addr.s_addr = inet_addr(localhost);
    if(::bind(sockfd_B, (struct sockaddr*) &clientBaddr, sizeof(clientBaddr)) == -1) {
        perror("Client B Bind Failed!");
        exit(1);
    }
    if (listen(sockfd_B, 5) == -1) {
        perror("Client B Listen Failed!");
        exit(1);
    }
}
//initialize Socket for serverC UDP
// source Beej's guide: https://beej.us/guide/bgnet/html/
void initializeSocket_UDP() {
    sockfd_UDP = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd_UDP == -1) {
        perror("UDP Socket Failed!");
        exit(1);
    }
    memset(UDPaddr.sin_zero, '\0', sizeof  UDPaddr.sin_zero);
    UDPaddr.sin_family = AF_INET;
    UDPaddr.sin_port = htons(UDPPort);
    UDPaddr.sin_addr.s_addr = inet_addr(localhost);
    if(::bind(sockfd_UDP, (struct sockaddr*) &UDPaddr, sizeof(UDPaddr)) == -1) {
        perror("UDP Bind Failed!");
        exit(1);
    }
}
//Set up addresses for serverT, S, P
// source Beej's guide: https://beej.us/guide/bgnet/html/
void setUpAddresses() {
    memset(address_P.sin_zero, '\0', sizeof  address_P.sin_zero);
    address_P.sin_family = AF_INET;
    address_P.sin_port = htons(serverPUDP);
    address_P.sin_addr.s_addr = inet_addr(localhost);
    memset(address_S.sin_zero, '\0', sizeof  address_S.sin_zero);
    address_S.sin_family = AF_INET;
    address_S.sin_port = htons(serverSUDP);
    address_S.sin_addr.s_addr = inet_addr(localhost);
    memset(address_T.sin_zero, '\0', sizeof  address_T.sin_zero);
    address_T.sin_family = AF_INET;
    address_T.sin_port = htons(serverTUDP);
    address_T.sin_addr.s_addr = inet_addr(localhost);
}

//remove null from given string
//source: https://www.cplusplus.com/reference/string/string/erase/
string removeNULL(string str) {
    str.erase(remove(str.begin(), str.end(), NULL), str.end());
    return str;
}
// split the result from serverT and add unique node to a set
void splitUserName(string str) {
    char char_array[str.length() + 1];
    strcpy(char_array, str.c_str());
    int len = *(&char_array + 1) - char_array;
    string word = "";
    for (int i = 0; i < len; i++)
    {
        if (char_array[i] == ' ')
        {
            if (nodeSet.count(removeNULL(word)) == 0  && word.length() != 0 && int(word[0]) != 0) {
                nodeSet.insert(removeNULL(word));
            }
            word = "";
        }
        else {
            word = word + char_array[i];
        }
    }
    if (nodeSet.count(removeNULL(word)) == 0 && word.length() != 0 && int(word[0]) != 0) {
        nodeSet.insert(removeNULL(word));
    }
}
//reset vectors and sets
void resetDataStructures() {
    setVector.clear();
    nodeSet.clear();
}

int main() {
    //initialization
    initializeSocket_clientA();
    initializeSocket_clientB();
    initializeSocket_UDP();
    printf("The Central server is up and running.\n");
    setUpAddresses();
    while (true) {
        socklen_t  clientA_TCP_len = sizeof(address_A);
        //accept the connection from clientA
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        socket_A = accept(sockfd_A, (struct sockaddr*) &address_A, &clientA_TCP_len);
        if (socket_A == -1) {
            perror("Accept Client A Failed!");
            exit(1);
        }
        int numbytes_A;
        //recv data from clientA
        if ((numbytes_A = recv(socket_A, Username_A, MAXDATASIZE - 1, 0)) == -1) {
            perror("Received from Client A Failed!");
            exit(1);
        }
        Username_A[numbytes_A] = '\0';
        printf("The Central server received input=\"%s\" from the client using TCP over port %d.\n", Username_A, TCPPortA);
        socklen_t  clientB_TCP_len = sizeof(address_B);
        //accept the connection from clientB
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        socket_B = accept(sockfd_B, (struct sockaddr*) &address_B, &clientB_TCP_len);
        if (socket_B == -1) {
            perror("Accept Client B Failed!");
            exit(1);
        }
        int numbytes_B;
        //recv data from clientA
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        if ((numbytes_B = recv(socket_B, Username_B, MAXDATASIZE - 1, 0)) == -1) {
            perror("Received from Client B Failed!");
            exit(1);
        }
        Username_B[numbytes_B] = '\0';
        printf("The Central server received input=\"%s\" from the client using TCP over port %d.\n", Username_B, TCPPortB);
        string usernameA_send(Username_A);
        string usernameB_send(Username_B);
        //combine given data, and sent it to serverT
        sendbuffer = usernameA_send + " " + usernameB_send;
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        if (sendto(sockfd_UDP, sendbuffer.data(), sendbuffer.size(), 0, (struct sockaddr *) &address_T, sizeof(address_T)) == -1) {
            perror("Send Data to server T Failed!");
            exit(1);
        }
        printf("The Central server sent a request to Backend-Server T.\n");
        serverT_size = sizeof(serverT_Address);
        //recv the result from serverT
        int serverTbytes;
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        if ((serverTbytes = recvfrom(sockfd_UDP, serverTInput, sizeof(serverTInput), 0, (struct sockaddr *)&serverT_Address, &serverT_size)) == -1) {
            perror("Receive From Server T Failed!");
            exit(1);
        }
        serverTInput[serverTbytes] = '\0';
        printf("The Central server received information from Backend-Server T using UDP over port %d.\n", UDPPort);
        string input_T(serverTInput); //convert char[] to array
        splitUserName(input_T); //split input, and add the input into a set to find out unique node
        std::set<string>::iterator iter;
        sendbuffer = "";
        //after get the node set, put unique nodes to a vector
        for (iter = nodeSet.begin(); iter != nodeSet.end(); ++iter) {
            setVector.push_back(*iter);
        }
        //loop over the vector, and prepare to send data to serverS
        for (int i = 0; i < setVector.size(); i++) {
            if (i == setVector.size() - 1) {
                sendbuffer = sendbuffer + setVector.at(i);
            } else {
                sendbuffer = sendbuffer + setVector.at(i) + " ";
            }
        }
        //send data to serverS
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        if (sendto(sockfd_UDP, sendbuffer.data(), sendbuffer.size(), 0, (struct sockaddr *) &address_S, sizeof(address_S)) == -1) {
            perror("Send Data to server S Failed!");
            exit(1);
        }
        printf("The Central server sent a request to Backend-Server S.\n");
        serverS_size = sizeof(serverS_Address);
        //recived data from serverS
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        int serverSbytes;
        if ((serverSbytes = recvfrom(sockfd_UDP, serverSInput, sizeof(serverSInput), 0, (struct sockaddr *)&serverS_Address, &serverS_size)) == -1) {
            perror("Receive From Server S Failed!");
            exit(1);
        }
        serverSInput[serverSbytes] = '\0';
        printf("The Central server received information from Backend-Server S using UDP over port %d.\n", UDPPort);
        //sent username_clientA to serverP
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        if (sendto(sockfd_UDP, usernameA_send.data(), usernameA_send.size(), 0, (struct sockaddr *) &address_P, sizeof(address_P)) == -1) {
            perror("Send usernameA to server P Failed!");
            exit(1);
        }
        //sent username_clientB to serverP
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        if (sendto(sockfd_UDP, usernameB_send.data(), usernameB_send.size(), 0, (struct sockaddr *) &address_P, sizeof(address_P)) == -1) {
            perror("Send usernameA to server P Failed!");
            exit(1);
        }
        //sent pathInfo from serverT to serverP
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        string serverT_Sender(serverTInput);
        if (sendto(sockfd_UDP, serverT_Sender.data(), serverT_Sender.size(), 0, (struct sockaddr *) &address_P, sizeof(address_P)) == -1) {
            perror("Send serverT data to server P Failed!");
            exit(1);
        }
        //sent scoreInfo from serverS to serverP
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        string serverS_Sender(serverSInput);
        if (sendto(sockfd_UDP, serverS_Sender.data(), serverS_Sender.size(), 0, (struct sockaddr *) &address_P, sizeof(address_P)) == -1) {
            perror("Send serverS data to server P Failed!");
            exit(1);
        }
        printf("The Central server sent a processing request to Backend-Server P.\n");
        serverP_size = sizeof(serverP_Address);
        //receive Path result from serverP
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        int serverPbytes;
        if ((serverPbytes = recvfrom(sockfd_UDP, serverPInput_Path, sizeof(serverPInput_Path), 0, (struct sockaddr *)&serverP_Address, &serverP_size)) == -1) {
            perror("Receive From Server T Failed!");
            exit(1);
        }
        serverPInput_Path[serverPbytes] = '\0';
        //receive Matching Gap result from serverP
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        if ((serverPbytes = recvfrom(sockfd_UDP, serverPInput_Score, sizeof(serverPInput_Score), 0, (struct sockaddr *)&serverP_Address, &serverP_size)) == -1) {
            perror("Receive From Server T Failed!");
            exit(1);
        }
        serverPInput_Score[serverPbytes] = '\0';
        printf("The Central server received the results from backend server P.\n");
        string pathInfo(serverPInput_Path);
        string scoreInfo(serverPInput_Score);
        senderData = pathInfo + " " + scoreInfo; //combine Path Result and Matching gap
        //send combined result to clientA
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        if (send(socket_A, senderData.data(), senderData.size(), 0) == -1) {
            perror("Send Result to clientA Failed!");
            exit(1);
        }
        printf("The Central server sent the results to client A.\n");
        //send combined result to clientB
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        if (send(socket_B, senderData.data(), senderData.size(), 0) == -1) {
            perror("Send Result to clientA Failed!");
            exit(1);
        }
        printf("The Central server sent the results to client B.\n");
        resetDataStructures();
    }
    //close sockets
    close(sockfd_UDP);
    close(socket_A);
    close(socket_B);
    return 0;
};

