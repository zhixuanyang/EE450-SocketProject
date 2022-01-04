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
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <vector>
using namespace std;

#define localhost "127.0.0.1" //localhost address
#define serverSUDP 22994 //serverS port number
#define MAXDATASIZE 40000 //maximum bytes number
#define fileName "scores.txt" //file name
#define centralUDPPort 24994 //serverC port number
int sockfd_UDP; //serverS UDP
struct sockaddr_in UDPaddr; //serverS address
struct sockaddr_in address_C; //serverC address
std::string line; //string line to read file
char inputBuffer[MAXDATASIZE]; //char[] to store input data
struct sockaddr_in central_Address; //serverC address
socklen_t central_size; //size of serverC
unordered_map<string, int> scoreMap; //map to store node name and its score
vector<string> nodeVector; //vector to store input data
string senderBuffer; //string that contains result, and sent it to serverC

//initialize Socket
// source Beej's guide: https://beej.us/guide/bgnet/html/
void initializeSocket() {
    sockfd_UDP = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd_UDP == -1) {
        perror("Server S UDP Socket Failed!");
        exit(1);
    }
    memset(UDPaddr.sin_zero, '\0', sizeof  UDPaddr.sin_zero);
    UDPaddr.sin_family = AF_INET;
    UDPaddr.sin_port = htons(serverSUDP);
    UDPaddr.sin_addr.s_addr = inet_addr(localhost);
    if(::bind(sockfd_UDP, (struct sockaddr*) &UDPaddr, sizeof(UDPaddr)) == -1) {
        perror("Server S UDP Bind Failed!");
        exit(1);
    }
    printf("The ServerS is up and running using UDP on port %d.\n", serverSUDP);
}
//remove null from given string
//source: https://www.cplusplus.com/reference/string/string/erase/
string removeNULL(string str) {
    str.erase(remove(str.begin(), str.end(), NULL), str.end());
    return str;
}
//put input data to map, where key is the node name and value is its score
void putDataToMap(string str) {
    char char_array[str.length() + 1];
    strcpy(char_array, str.c_str());
    int len = *(&char_array + 1) - char_array;
    string word = "";
    string name;
    string value;
    for (int i = 0; i < len; i++)
    {
        if (char_array[i] == ' ')
        {
            name = removeNULL(word);
            word = "";
        }
        else {
            word = word + char_array[i];
        }
    }
    value = removeNULL(word);
    scoreMap.insert(make_pair(name, std::stoi(value)));
}
//split input data to vector
void splitDataToVector(string str) {
    char char_array[str.length() + 1];
    strcpy(char_array, str.c_str());
    int len = *(&char_array + 1) - char_array;
    string word = "";
    for (int i = 0; i < len; i++)
    {
        if (char_array[i] == ' ' && i != 0)
        {
            nodeVector.push_back(removeNULL(word));
            word = "";
        }
        else {
            word = word + char_array[i];
        }
    }
    nodeVector.push_back(removeNULL(word));
}
//reset vector and map
void resetDataStructures() {
    scoreMap.clear();
    nodeVector.clear();
}
int main() {
    //initialization
    initializeSocket();
    while (true) {
        central_size = sizeof(central_Address);
        int bytesReceived;
        //receive input from serverC
        if ((bytesReceived = recvfrom(sockfd_UDP, inputBuffer, sizeof(inputBuffer), 0, (struct sockaddr *)&central_Address, &central_size)) == -1) {
            perror("Receive From Central Server Failed!");
            exit(1);
        }
        inputBuffer[bytesReceived] = '\0';
        printf("The ServerS received a request from Central to get the scores.\n");
        //read file
        //source: https://www.cplusplus.com/doc/tutorial/files/
        std::ifstream file(fileName);
        if (file.is_open()) {
            while (std::getline(file, line)) {
                putDataToMap(line);
            }
        }
        //convert char[] to string
        string inputData(inputBuffer);
        inputData = removeNULL(inputData);
        splitDataToVector(inputData);
        senderBuffer = "";
        //put scores into an empty string according to the given input
        for (int i = 0; i < nodeVector.size(); i++) {
            if (scoreMap.count(nodeVector.at(i)) == 0) {
                senderBuffer = "";
                break;
            }
            if (i == nodeVector.size() - 1) {
                senderBuffer = senderBuffer + to_string(scoreMap[nodeVector.at(i)]);
            } else {
                senderBuffer = senderBuffer + to_string(scoreMap[nodeVector.at(i)]) + " ";
            }
        }
        //set up address
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        memset(address_C.sin_zero, '\0', sizeof  address_C.sin_zero);
        address_C.sin_family = AF_INET;
        address_C.sin_port = htons(centralUDPPort);
        address_C.sin_addr.s_addr = inet_addr(localhost);
        //send result to serverC
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        if (sendto(sockfd_UDP, senderBuffer.data(), senderBuffer.size(), 0, (struct sockaddr *) &address_C, sizeof(address_C)) == -1) {
            perror("Send Data to server Central Failed!");
            exit(1);
        }
        printf("The ServerS finished sending the scores to Central.\n");
        resetDataStructures();
    }
    //close the socket
    // source Beej's guide: https://beej.us/guide/bgnet/html/
    close(sockfd_UDP);
    return 0;
}
