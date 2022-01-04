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
#include <fstream>
#include <algorithm>
#include <vector>
#include <cstring>
#include <unordered_map>
#include <algorithm>
using namespace std;

#define localhost "127.0.0.1" //localhost address
#define serverTUDP 21994 //serverT port number
#define centralUDPPort 24994 //serverC port number
#define MAXDATASIZE 4096 //maximum number of bytes
#define fileName "edgelist.txt" //filename
char inputBuffer[MAXDATASIZE]; //char[] to store input data
struct sockaddr_in central_Address; //serverC address
socklen_t central_size; //address of serverC
vector<string> start; //vector to store start node
vector<string> destination; //vector to store destination node
vector<string> nodeVector; //vector to store node info
std::string line; //string line to read file
unordered_map<string, int> nodeMap; //map that store node name and its id
struct sockaddr_in address_C; //address of serverC
int currentNum = 0; //variable to set up ID
string result; //string object to store result, and sent to serverC
int sockfd_UDP; //serverT UDP
struct sockaddr_in UDPaddr; //address of serverT UDP

//initialize socket
// source Beej's guide: https://beej.us/guide/bgnet/html/
void initializeSocket() {
    sockfd_UDP = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd_UDP == -1) {
        perror("Server T UDP Socket Failed!");
        exit(1);
    }
    memset(UDPaddr.sin_zero, '\0', sizeof  UDPaddr.sin_zero);
    UDPaddr.sin_family = AF_INET;
    UDPaddr.sin_port = htons(serverTUDP);
    UDPaddr.sin_addr.s_addr = inet_addr(localhost);
    if(::bind(sockfd_UDP, (struct sockaddr*) &UDPaddr, sizeof(UDPaddr)) == -1) {
        perror("Server P UDP Bind Failed!");
        exit(1);
    }
    printf("The ServerT is up and running using UDP on port %d.\n", serverTUDP);
}
//remove null from string
//source: https://www.cplusplus.com/reference/string/string/erase/
string removeNULL(string str) {
    str.erase(remove(str.begin(), str.end(), NULL), str.end());
    return str;
}
//put input to vector and map
void putDataToVector(string str) {
    char char_array[str.length() + 1];
    strcpy(char_array, str.c_str());
    int len = *(&char_array + 1) - char_array;
    string word = "";
    for (int i = 0; i < len; i++)
    {
        if (char_array[i] == ' ')
        {
            start.push_back(removeNULL(word));
            if (nodeMap.count(removeNULL(word)) == 0) {
                nodeMap.insert(make_pair(removeNULL(word), currentNum));
                currentNum += 1;
            }
            word = "";
        }
        else {
            word = word + char_array[i];
        }
    }
    destination.push_back(removeNULL(word));
    if (nodeMap.count(removeNULL(word)) == 0) {
        nodeMap.insert(make_pair(removeNULL(word), currentNum));
        currentNum += 1;
    }
}
//disjoint set data structure to connect the nodes, and extract topology from given input
// To find the topology, this data structure is very useful because we can simply return the
// nodes that have same set number
// source: https://algs4.cs.princeton.edu/code/javadoc/edu/princeton/cs/algs4/QuickUnionUF.html
//         https://www.geeksforgeeks.org/union-find-algorithm-set-2-union-by-rank/
struct DisjointSet {
    vector<int> parent;
    vector<int> size;
    //constructor
    DisjointSet(int maxSize) {
        parent.resize(maxSize);
        size.resize(maxSize);
        for (int i = 0; i < maxSize; i++) {
            parent[i] = i;
            size[i] = 1;
        }
    }
    //find set of given node
    int find_set(int v) {
        if (v == parent[v])
            return v;
        return parent[v] = find_set(parent[v]);
    }
    // union two nodes, if two nodes are union. The set number of given nodes will be same, 
    // and their connected node will have same set number. 
    // To find the topology, this data structure is very useful because we can simply return the 
    // nodes that have same set number
    void union_set(int a, int b) {
        a = find_set(a);
        b = find_set(b);
        if (a != b) {
            if (size[a] < size[b])
                swap(a, b);
            parent[b] = a;
            size[a] += size[b];
        }
    }
};

//put input data into node vector
void putInputToVector(string str) {
    char char_array[str.length() + 1];
    strcpy(char_array, str.c_str());
    int len = *(&char_array + 1) - char_array;
    string word = "";
    for (int i = 0; i < len; i++)
    {
        if (char_array[i] == ' ')
        {
            nodeVector.push_back(word);
            word = "";
        }
        else {
            word = word + char_array[i];
        }
    }
    nodeVector.push_back(word);
}
//reset the data structure
void resetDataStructures() {
    start.clear();
    destination.clear();
    nodeVector.clear();
    nodeMap.clear();
    currentNum = 0;
}
int main() {
    //initialization
    initializeSocket();
    while (true) {
        central_size = sizeof(central_Address);
        int bytesReceived;
        //receive input from serverC
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        if ((bytesReceived = recvfrom(sockfd_UDP, inputBuffer, sizeof(inputBuffer), 0, (struct sockaddr *)&central_Address, &central_size)) == -1) {
            perror("Receive From Central Server Failed!");
            exit(1);
        }
        inputBuffer[bytesReceived] = '\0';
        printf("The ServerT received a request from Central to get the topology.\n");
        //read the file
        //source: https://www.cplusplus.com/doc/tutorial/files/
        std::ifstream file(fileName);
        if (file.is_open()) {
            while (std::getline(file, line)) {
                putDataToVector(line);
            }
        }
        string temp_Username(inputBuffer);
        //put input data into vector and map
        putInputToVector(temp_Username);
        //store inputs from clientA and clientB to two variables
        string userName_ClientA = removeNULL(nodeVector.at(0));
        string userName_ClientB = removeNULL(nodeVector.at(1));
        //set up address
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        memset(address_C.sin_zero, '\0', sizeof  address_C.sin_zero);
        address_C.sin_family = AF_INET;
        address_C.sin_port = htons(centralUDPPort);
        address_C.sin_addr.s_addr = inet_addr(localhost);
        //create the disjoint set object
        DisjointSet djSet(currentNum);
        //union the given input
        for (int i = 0; i < start.size(); i++) {
            djSet.union_set(nodeMap[start.at(i)], nodeMap[destination.at(i)]);
        }
        //the boolean array to see whether the given path has been visited or not
        bool visited[start.size()];
        for (int i = 0; i < start.size(); i++) {
            visited[i] = false;
        }
        //get the set number of inputs of clientA and clientB
        int unionNumber_ClientA = djSet.find_set(nodeMap[userName_ClientA]);
        int unionNumber_ClientB = djSet.find_set(nodeMap[userName_ClientB]);
        result = "";
        //add all the paths that related to the match node to a string
        for (int i = 0; i < start.size(); i++) {
            if (i == start.size() - 1 && djSet.find_set(nodeMap[start.at(i)]) == unionNumber_ClientA && djSet.find_set(nodeMap[destination.at(i)]) == unionNumber_ClientA && visited[i] == false) {
                result = result + start.at(i) + " " + destination.at(i);
                visited[i] = true;
            } else if (djSet.find_set(nodeMap[start.at(i)]) == unionNumber_ClientA && djSet.find_set(nodeMap[destination.at(i)]) == unionNumber_ClientA && visited[i] == false) {
                result = result + start.at(i) + " " + destination.at(i) + " ";
                visited[i] = true;
            } else if (i == start.size() - 1 && djSet.find_set(nodeMap[start.at(i)]) == unionNumber_ClientB && djSet.find_set(nodeMap[destination.at(i)]) == unionNumber_ClientB && visited[i] == false) {
                result = result + start.at(i) + " " + destination.at(i);
                visited[i] = true;
            } else if (djSet.find_set(nodeMap[start.at(i)]) == unionNumber_ClientB && djSet.find_set(nodeMap[destination.at(i)]) == unionNumber_ClientB && visited[i] == false) {
                result = result + start.at(i) + " " + destination.at(i) + " ";
                visited[i] = true;
            }
        }
        //sometimes there are null in the string, so use removeNULL function to remove them
        result = removeNULL(result);
        //send the result to serverC
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        if (sendto(sockfd_UDP, result.data(), result.size(), 0, (struct sockaddr *) &address_C, sizeof(address_C)) == -1) {
            perror("Send Data to server Central Failed!");
            exit(1);
        }
        printf("The ServerT finished sending the topology to Central.\n");
        resetDataStructures();
    }
    //close the socket
    // source Beej's guide: https://beej.us/guide/bgnet/html/
    close(sockfd_UDP);
    return 0;
}