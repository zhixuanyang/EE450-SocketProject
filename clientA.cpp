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
#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
using namespace std;

#define TCPPortA 25994 //TCP Port of Central Server for ClientA
#define localhost "127.0.0.1" //localhost address
#define MAXDATASIZE 40000 //max number of bytes
char inputData[MAXDATASIZE]; //char[] to store input data
vector<string> pathVector; //vector to store path information
vector<string> inputVector; //vector to store input data
int sockfd_A; //socket for clientA
struct sockaddr_in clientAaddr; //address for clientA

//initialize Socket
// source Beej's guide: https://beej.us/guide/bgnet/html/
void initializeSocket() {
    sockfd_A = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd_A == -1) {
        perror("Client A Socket Failed!");
        close(sockfd_A);
        exit(1);
    }
    memset(clientAaddr.sin_zero, '\0', sizeof  clientAaddr.sin_zero);
    clientAaddr.sin_family = AF_INET;
    clientAaddr.sin_port = htons(TCPPortA);
    clientAaddr.sin_addr.s_addr = inet_addr(localhost);
    if (connect(sockfd_A, (struct sockaddr*) &clientAaddr, sizeof(clientAaddr)) == -1) {
        perror("Client A Connect Failed!");
        close(sockfd_A);
        exit(1);
    }
    printf("The client is up and running.\n");
}

//function to remove null from string
//source: https://www.cplusplus.com/reference/string/string/erase/
string removeNULL(string str) {
    str.erase(remove(str.begin(), str.end(), NULL), str.end());
    return str;
}

//function to extract path info from input data vector, and add them into a pathVector
void putPathInfoToVector(vector<string> input) {
    for (int i = 0; i < input.size() - 1; i++) {
        pathVector.push_back(input[i]);
    }
}
//split input data, and put them into a vector
void putInputToVector(string str) {
    char char_array[str.length() + 1];
    strcpy(char_array, str.c_str());
    int len = *(&char_array + 1) - char_array;
    string word = "";
    for (int i = 0; i < len; i++)
    {
        if (char_array[i] == ' ')
        {
            inputVector.push_back(removeNULL(word));
            word = "";
        }
        else {
            word = word + char_array[i];
        }
    }
    inputVector.push_back(removeNULL(word));
}
int main(int argc, char const *argv[]) {
    //call this function to initialize socket
    initializeSocket();
    //to detect input argument
    if (argc != 2) {
        perror("The Username is Missing!");
        close(sockfd_A);
        exit(1);
    }
    //send input data to central server
    // source Beej's guide: https://beej.us/guide/bgnet/html/
    if (send(sockfd_A, argv[1], strlen(argv[1]), 0) == -1) {
        perror("Client A Send Username Failed!");
        close(sockfd_A);
        exit(1);
    }
    printf("The client sent %s to the Central server.\n", argv[1]);
    int numbytes_A;
    if ((numbytes_A = recv(sockfd_A, inputData, MAXDATASIZE - 1, 0)) == -1) {
        perror("Received from Client A Failed!");
        close(sockfd_A);
        exit(1);
    }
    inputData[numbytes_A] = '\0'; //receive processed result from central server
    string input(inputData); //convert char[] to string
    putInputToVector(inputData); //put input data to input vector
    float score = std::stof(inputVector[inputVector.size() - 1]); //extract the score from input vector
    putPathInfoToVector(inputVector); //put path info to vector
    string userName_A = pathVector[0]; //userName from clientA
    string userName_B = pathVector[pathVector.size() - 1]; //userName from clientB
    if (score == -1.0) { //determine whether there is available path between two username
        printf("Found no compatibility for %s and %s.\n", userName_A.c_str(), userName_B.c_str());
    } else {
        printf("Found compatibility for %s and %s:\n", userName_A.c_str(), userName_B.c_str());
        string path = "";
        for (int i = 0; i < pathVector.size(); i++) {
            if (i == pathVector.size() - 1) {
                path = path + pathVector[i];
            } else {
                path = path + pathVector[i] + " --- ";
            }
        }
        printf("%s\n", path.c_str());
        printf("Matching Gap: %.2f\n", score);
    }
    //close the socket
    // source Beej's guide: https://beej.us/guide/bgnet/html/
    close(sockfd_A);
    return 0;
}