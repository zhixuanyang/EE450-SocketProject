#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
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

#define TCPPortB 26994 //TCP Port of Central Server for ClientB
#define localhost "127.0.0.1" //localhost address
#define MAXDATASIZE 40000 //max number of bytes
char inputData[MAXDATASIZE]; //to store received data from central server
vector<string> pathVector; //vector to store path information
vector<string> inputVector; //vector to store input data
int sockfd_B; //socket for clientB
struct sockaddr_in clientBaddr; //address for clientB

//initialize Socket
// source Beej's guide: https://beej.us/guide/bgnet/html/
void initializeSocket() {
    sockfd_B = socket(PF_INET, SOCK_STREAM, 0); //create socket
    if (sockfd_B == -1) {
        perror("Client B Socket Failed!");
        close(sockfd_B);
        exit(1);
    }
    memset(clientBaddr.sin_zero, '\0', sizeof  clientBaddr.sin_zero); //set up address
    clientBaddr.sin_family = AF_INET;
    clientBaddr.sin_port = htons(TCPPortB);
    clientBaddr.sin_addr.s_addr = inet_addr(localhost);
    if (connect(sockfd_B, (struct sockaddr*) &clientBaddr, sizeof(clientBaddr)) == -1) { //connect to central server
        perror("Client B Connect Failed!");
        close(sockfd_B);
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
    //call this function to initialize Socket
    initializeSocket();
    //to detect input argument
    if (argc != 2) {
        perror("The Username is Missing!");
        close(sockfd_B);
        exit(1);
    }
    //send input data to central server
    // source Beej's guide: https://beej.us/guide/bgnet/html/
    if (send(sockfd_B, argv[1], strlen(argv[1]), 0) == -1) {
        perror("Client B Send Username Failed!");
        close(sockfd_B);
        exit(1);
    }
    printf("The client sent %s to the Central server.\n", argv[1]);
    // receive result from serverC
    // source Beej's guide: https://beej.us/guide/bgnet/html/
    int numbytes_B;
    if ((numbytes_B = recv(sockfd_B, inputData, MAXDATASIZE - 1, 0)) == -1) {
        perror("Received from Client A Failed!");
        close(sockfd_B);
        exit(1);
    }
    inputData[numbytes_B] = '\0'; //receive processed result from central server
    string input(inputData); //convert char[] to string
    putInputToVector(inputData); //put input data to input vector
    float score = std::stof(inputVector[inputVector.size() - 1]); //extract the score from input vector
    putPathInfoToVector(inputVector); //put path info to vector
    string userName_A = pathVector[0]; //userName from clientA
    string userName_B = pathVector[pathVector.size() - 1]; //userName from clientB
    if (score == -1.0) { //determine whether there is available path between two username
        printf("Found no compatibility for %s and %s.\n", userName_B.c_str(), userName_A.c_str());
    } else {
        printf("Found compatibility for %s and %s:\n", userName_B.c_str(), userName_A.c_str());
        string path = "";
        for (int i = pathVector.size() - 1; i >= 0; i--) {
            if (i == 0) {
                path = path + pathVector[i];
            } else {
                path = path + pathVector[i] + " --- ";
            }
        }
        printf("%s\n", path.c_str());
        printf("Matching Gap: %.2f\n", score);
    }
    //close the socket
    close(sockfd_B);
    return 0;
}