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
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <limits>
#include <queue>
#include <cmath>
#include <list>
using namespace std;

#define localhost "127.0.0.1" //localhost address
#define serverPUDP 23994 //UDP port for serverP
#define centralUDPPort 24994 //UDP port for serverC
#define MAXDATASIZE 40000 //maximum number of bytes
int sockfd_UDP; //serverP UDP socket
struct sockaddr_in UDPaddr; //serverP address
struct sockaddr_in central_Address; //serverC address
socklen_t central_size; //size of serverC address
char clientA_input[MAXDATASIZE]; //char[] to store input from clientA
char clientB_input[MAXDATASIZE]; //char[] to store input from clientB
char input_T[MAXDATASIZE]; //char[] to store input from serverT
char input_S[MAXDATASIZE]; //char[] to store input from serverS
set<string> nodeSet; //set that stores extract unique node
vector<string> setVector; //store set elements into a vector
vector<int> scoreVector; //vector to store score info
unordered_map<string, int> scoreMap; //map with node name and its score
unordered_map<string, int> nodeMap; //map with node name with a ID (like Oliver 0, Victor 1, etc.)
unordered_map<int, string> inverseMap; //map with ID and its node name
vector<string> nodeVector; //vector to store node name
vector<string> start; //vector to store start point
vector<string> destination; //vector to store destination point
struct sockaddr_in address_C; //address of serverC
int currNum = 0; //index ID
typedef pair<int, float> vertex_pair; //vertex pair, which contains nodeID and its weight
string pathInfo; //string to store pathInfo and sent it to serverC
string shortestDistance; //string to store shortest distance and send it to serverC

//initialize socket
// source Beej's guide: https://beej.us/guide/bgnet/html/
void initializeSocket() {
    sockfd_UDP = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd_UDP == -1) {
        perror("Server P UDP Socket Failed!");
        exit(1);
    }
    memset(UDPaddr.sin_zero, '\0', sizeof  UDPaddr.sin_zero);
    UDPaddr.sin_family = AF_INET;
    UDPaddr.sin_port = htons(serverPUDP);
    UDPaddr.sin_addr.s_addr = inet_addr(localhost);
    if(::bind(sockfd_UDP, (struct sockaddr*) &UDPaddr, sizeof(UDPaddr)) == -1) {
        perror("Server P UDP Bind Failed!");
        exit(1);
    }
    printf("The ServerP is up and running using UDP on port %d.\n", serverPUDP);
}
//remove null from string
//source: https://www.cplusplus.com/reference/string/string/erase/
string removeNULL(string str) {
    str.erase(remove(str.begin(), str.end(), NULL), str.end());
    return str;
}
//split input data to vector, which every input data separated by a space
void splitInputToVector(string str) {
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
            nodeVector.push_back(removeNULL(word));
            word = "";
        }
        else {
            word = word + char_array[i];
        }
    }
    if (nodeSet.count(removeNULL(word)) == 0 && word.length() != 0 && int(word[0]) != 0) {
        nodeSet.insert(removeNULL(word));
    }
    nodeVector.push_back(removeNULL(word));
}
//add received input from serverS into a vector
void addToScoreVector(string str) {
    char char_array[str.length() + 1];
    strcpy(char_array, str.c_str());
    int len = *(&char_array + 1) - char_array;
    string word = "";
    for (int i = 0; i < len; i++)
    {
        if (char_array[i] == ' ')
        {
            word = removeNULL(word);
            scoreVector.push_back(std::stoi(word));
            word = "";
        }
        else {
            word = word + char_array[i];
        }
    }
    word = removeNULL(word);
    scoreVector.push_back(std::stoi(word));
}
//return the weight of edge
float getWeight(string source, string dest) {
    float sourceScore = (float) scoreMap[source];
    float destinationScore = (float) scoreMap[dest];
    float result = abs(sourceScore - destinationScore) / (sourceScore + destinationScore);
    return result;
}
//the dijkstra algorithm to find shortest path between two node
// modified from https://algs4.cs.princeton.edu/code/javadoc/edu/princeton/cs/algs4/DijkstraUndirectedSP.html
// and https://www.javatpoint.com/cpp-dijkstra-algorithm-using-priority-queue to know how priority queue works
// in c++
struct dijkstra {
    int num_v;
    list<pair<int, float> > *adj;
    vector<float> dist;
    vector<int> parent;
    //constructor
    dijkstra(int V) {
        num_v = V;
        adj = new list<pair<int, float> > [V];
    }
    //add an edge, in which v is the source and w is the destination
    void add_Edge(int v, int w, float weight) {
        adj[v].push_back(make_pair(w, weight));
        adj[w].push_back(make_pair(v, weight));
    }
    //use the priority queue to find out the shortest path
    void findShortestPath(int source) {
        priority_queue<vertex_pair, vector<vertex_pair>, greater<vertex_pair> > pq;
        dist.resize(num_v);
        for (int i = 0; i < dist.size(); i++) {
            dist[i] = numeric_limits<float>::infinity();
        }
        parent.resize(num_v);
        parent[source] = -1;
        pq.push(make_pair(0, source));
        dist[source] = 0;
        while (!pq.empty()) {
            int w = pq.top().second;
            pq.pop();
            list<vertex_pair>::iterator it;
            for (it = adj[w].begin(); it != adj[w].end(); it++) {
                int vertex = it->first;
                float weight = it->second;
                if (dist[vertex] > dist[w] + weight) {
                    dist[vertex] = dist[w] + weight;
                    pq.push(make_pair(dist[vertex], vertex));
                    parent[vertex] = w;
                }
            }
        }
    }
    //return the shortest distance between two nodes
    float getDistance(int dest) {
        return dist[dest];
    }
    //return the path between two nodes
    vector<int> getPathInfo(int dest) {
        vector<int> pathInfo;
        addPathToVector(pathInfo, dest);
        return pathInfo;
    }
    //add the shortest path to a vector, this function is a helper function of getPathInfo
    void addPathToVector(vector<int> &pathInfo, int dest)
    {
        if (parent[dest] == -1) {
            pathInfo.push_back(dest);
            return;
        }
        addPathToVector(pathInfo, parent[dest]);
        pathInfo.push_back(dest);
    }
};
//reset sets and vectors
void resetDataStructures() {
    nodeSet.clear();
    setVector.clear();
    scoreVector.clear();
    scoreMap.clear();
    nodeMap.clear();
    inverseMap.clear();
    nodeVector.clear();
    start.clear();
    destination.clear();
    currNum = 0;
}

int main() {
    //initialize socket
    initializeSocket();
    while (true) {
        central_size = sizeof(central_Address);
        //get clientA input from central
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        int bytesReceived;
        if ((bytesReceived = recvfrom(sockfd_UDP, clientA_input, sizeof(clientA_input), 0, (struct sockaddr *)&central_Address, &central_size)) == -1) {
            perror("Receive From Central Server Failed!");
            exit(1);
        }
        clientA_input[bytesReceived] = '\0';
        //get clientB input from central
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        if ((bytesReceived = recvfrom(sockfd_UDP, clientB_input, sizeof(clientB_input), 0, (struct sockaddr *)&central_Address, &central_size)) == -1) {
            perror("Receive From Central Server Failed!");
            exit(1);
        }
        clientB_input[bytesReceived] = '\0';
        //get serverT input from central
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        if ((bytesReceived = recvfrom(sockfd_UDP, input_T, sizeof(input_T), 0, (struct sockaddr *)&central_Address, &central_size)) == -1) {
            perror("Receive From Central Server Failed!");
            exit(1);
        }
        input_T[bytesReceived] = '\0';
        //get serverS input from central
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        if ((bytesReceived = recvfrom(sockfd_UDP, input_S, sizeof(input_S), 0, (struct sockaddr *)&central_Address, &central_size)) == -1) {
            perror("Receive From Central Server Failed!");
            exit(1);
        }
        input_S[bytesReceived] = '\0';
        printf("The ServerP received the topology and score information.\n");
        string inputT(input_T);
        splitInputToVector(inputT);//split input data to a vector and set
        std::set<string>::iterator iter;
        //extract the score info through a loop over set, and put node name and its ID into maps
        for (iter = nodeSet.begin(); iter != nodeSet.end(); ++iter) {
            setVector.push_back(*iter);
            nodeMap.insert(make_pair(*iter, currNum));
            inverseMap.insert(make_pair(currNum, *iter));
            currNum += 1;
        }
        string inputS(input_S);
        //add score info to a vector 
        addToScoreVector(inputS);
        //put node names and their scores into a map 
        for (int i = 0; i < setVector.size(); i++) {
            scoreMap.insert(make_pair(setVector.at(i), scoreVector.at(i)));
        }
        //split the node vector and put them into two vectors, one for start and another one for destination
        for (int i = 0; i < nodeVector.size(); i++) {
            if (int(nodeVector[i][0]) == 0) {
                continue;
            }
            if (i % 2 == 0) {
                start.push_back(nodeVector.at(i));
            } else {
                destination.push_back(nodeVector.at(i));
            }
        }
        //create the dijkstra object
        dijkstra graph(nodeSet.size());
        // over start and destination vectors, and use the data to add edges 
        for (int i = 0; i < start.size(); i++) {
            int first = nodeMap[start[i]];
            int second = nodeMap[destination[i]];
            float weight = getWeight(start[i], destination[i]);
            graph.add_Edge(first, second, weight);
        }
        string userNameA(clientA_input);
        string userNameB(clientB_input);
        //input from clientA as source, and do the search
        graph.findShortestPath(nodeMap[userNameA]);
        //get the shortest path between nodeA and nodeB
        float distanceToB = graph.getDistance(nodeMap[userNameB]);
        //send result to serverC
        if (distanceToB == numeric_limits<float>::infinity()) {
            //set up addresses
            // source Beej's guide: https://beej.us/guide/bgnet/html/
            memset(address_C.sin_zero, '\0', sizeof  address_C.sin_zero);
            address_C.sin_family = AF_INET;
            address_C.sin_port = htons(centralUDPPort);
            address_C.sin_addr.s_addr = inet_addr(localhost);
            float negative = -1.0; //I change the infinity to -1 because it saved lots of bytes
            string shortestDistance = to_string(negative); //convert the float to string, and sent the result later
            // if there is no path between two nodes, and the inputA and inputB into an pathInfo for display on clientA and B
            string pathInfo = userNameA + " " + userNameB;
            //send pathInfo to serverC
            // source Beej's guide: https://beej.us/guide/bgnet/html/
            if (sendto(sockfd_UDP, pathInfo.data(), pathInfo.size(), 0, (struct sockaddr *) &address_C, sizeof(address_C)) == -1) {
                perror("Send Data to server Central Failed!");
                exit(1);
            }
            //send shortest distance to serverC
            // source Beej's guide: https://beej.us/guide/bgnet/html/
            if (sendto(sockfd_UDP, shortestDistance.data(), shortestDistance.size(), 0, (struct sockaddr *) &address_C, sizeof(address_C)) == -1) {
                perror("Send Data to server Central Failed!");
                exit(1);
            }
            printf("The ServerP finished sending the results to the Central.\n");
        } else {
            //get the pathInfo
            vector<int> pathToB = graph.getPathInfo(nodeMap[userNameB]);
            pathInfo = "";
            //add the pathInfo to pathInfo variable
            for (int i = 0; i < pathToB.size(); i++) {
                if (i == pathToB.size() - 1) {
                    pathInfo = pathInfo + inverseMap[pathToB[i]];
                } else {
                    pathInfo = pathInfo + inverseMap[pathToB[i]] + " ";
                }
            }
            //convert the float to string, and sent the result later
            shortestDistance = to_string(distanceToB);
            pathInfo = removeNULL(pathInfo);
            shortestDistance = removeNULL(shortestDistance);
            //set up addresses
            // source Beej's guide: https://beej.us/guide/bgnet/html/
            memset(address_C.sin_zero, '\0', sizeof  address_C.sin_zero);
            address_C.sin_family = AF_INET;
            address_C.sin_port = htons(centralUDPPort);
            address_C.sin_addr.s_addr = inet_addr(localhost);
            //send pathInfo to serverC
            // source Beej's guide: https://beej.us/guide/bgnet/html/
            if (sendto(sockfd_UDP, pathInfo.data(), pathInfo.size(), 0, (struct sockaddr *) &address_C, sizeof(address_C)) == -1) {
                perror("Send Data to server Central Failed!");
                exit(1);
            }
            //send shortest distance to serverC
            // source Beej's guide: https://beej.us/guide/bgnet/html/
            if (sendto(sockfd_UDP, shortestDistance.data(), shortestDistance.size(), 0, (struct sockaddr *) &address_C, sizeof(address_C)) == -1) {
                perror("Send Data to server Central Failed!");
                exit(1);
            }
            printf("The ServerP finished sending the results to the Central.\n");
        }
        resetDataStructures();
    }
    //close the socket
    // source Beej's guide: https://beej.us/guide/bgnet/html/
    close(sockfd_UDP);
    return 0;
}