#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iomanip>
#include <sstream>
#include <openssl/sha.h>
#include <pthread.h>
#include <fcntl.h> // for downloading/uploading
#include <sys/stat.h> //for stat - fileSize
#include <dirent.h>

using namespace std;

int THREAD_NUM = 4; // for multi peer download
struct ThreadArgs {
    string fileName, filePath, ip;
    long long offset, reqBytes;
    int port;
};

#define CHUNKSIZE 524288
#define THREAD_NUM 4
char chunkBuffer[CHUNKSIZE];

string userId, SERVER_IP;
int SERVER_PORT;
bool logged=false;
char buffer[1024];
map<string, string> filePathMap;
map<string, vector<int>>fileChunkMap;
// grpid - filename
map<string, string> downloadsMap;

void errorString(int err, string s){
    if(err < 0){ //error occured
        if(s != "") cout<<s;
        exit(1);
    }
    // return 0;
}

struct fileData{
    char fileName[200], filePath[300];
};

void loadFilePath(){
    FILE *filePath;
    filePath = fopen("./filePath.txt", "r");
    cout << "filePath: \n";
    if (filePath == NULL){
        fprintf(stderr, "\nError opening filePath.txt file\n");
        return;
    }
    fileData f;
    while (fread(&f, sizeof(struct fileData), 1, filePath)){
        filePathMap[f.fileName] = f.filePath;
        cout << f.fileName << " " << f.filePath << endl;
    }
    fclose(filePath);
}

vector<string> getArguments(string inputArg){
    vector<string> args;
    int i=0, j=0, n = inputArg.size();
    while(i<n){
        if(i == j && inputArg[i] == ' ')
            j++;
        else if(inputArg[i] == ' '){
            args.push_back(inputArg.substr(j, i-j));
            j = i+1;
        }
        i++;
    }
    args.push_back(inputArg.substr(j, i-j));
    return args;
}

struct Peers{
    string ip, port;
};

void sendFile(string args){
    vector<string> argsList = getArguments(args);
    string file = filePathMap[argsList[0]];
}


void enquiryChunks(map<int, vector<pair<string, string>> > &chunkMap, int n, struct Peers s[], string fileName, long long noChunks){
    // printf("[+][CLIENT]Client Socket is created.\n");
    for(int i=0; i<(n/2); i++){

        int client_Socket, ret;
        struct sockaddr_in serverAddr;
        char buffer[1024];
        client_Socket = socket(AF_INET, SOCK_STREAM, 0);
        errorString(client_Socket, "[-][CLIENT]Error in connection.\n");
        
        memset(&serverAddr, '\0', sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(stoi(s[i].port));
        serverAddr.sin_addr.s_addr = inet_addr(s[i].ip.c_str());
    
        ret = connect(client_Socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if(ret == -1){
            cout<<"[-][CLIENT]Error in connection.\n";
            return;
        }
        printf("[+][PEER]Connection build with Seeder: %s:%d, ", inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));
    
        string cmnd = "enquiry " + fileName;
    	write(client_Socket, cmnd.c_str(), cmnd.size());
    
        bzero(buffer, sizeof(buffer));
        // getting status of download
        read(client_Socket, buffer, 1024);
        string recieved(buffer);
        cout<<" enuiry reply: "<<recieved<<endl;
        if(recieved == "-1"){
            for(int j=0; j<noChunks; j++)
                chunkMap[j].push_back({s[i].port, s[i].ip});
        }
        else if(recieved != "-2"){
            vector<string> chunkNo = getArguments(recieved);
            int m = chunkNo.size();
            for(int j=0; j<m; j++)
                chunkMap[stoi(chunkNo[j])].push_back({s[i].ip, s[i].port});
        }

        close(client_Socket);  
    }
}

void downloadChunk(string fileName, string filePath, int offset, string ip, int port, long long reqBytes){
	
    int client_Socket, ret;
	struct sockaddr_in serverAddr;

	client_Socket = socket(AF_INET, SOCK_STREAM, 0);
	errorString(client_Socket, "[-][CLIENT]Error in connection.\n");
	
    memset(&serverAddr, '\0', sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

	ret = connect(client_Socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	errorString(ret, "[-][CLIENT]Error in connection.\n");
	// printf("[+][PEER]Connection build with Seeder");
    // sending file name
    string cmnd = "chunk "+ fileName + " " + to_string(offset) + " " + to_string(reqBytes);
    // cout<<cmnd<<" :sending for chunkDownload\n";
    write(client_Socket, cmnd.c_str(), cmnd.size());

    // receieving file
    fileName = filePath + fileName;
    char resPath[200];
    realpath(fileName.c_str(), resPath);
    string finalPath(resPath);

    // opening file change
    int fp = open(finalPath.c_str(), O_CREAT | O_RDWR, 0777);
    if (fp == 0) {
        printf("Error opening file");
        return;
    }

    long long bytesReceived = 0, tempRec;
    reqBytes = min(reqBytes, (long long )CHUNKSIZE);
    char fileBuffer[CHUNKSIZE], tempBuffer[CHUNKSIZE];
    
    while (1) {
        tempRec = recv(client_Socket, tempBuffer, CHUNKSIZE, 0);
        if(tempRec == 0){
            // cout<<"bytesrec: "<<bytesReceived<<endl;
            break;
        }
        for(int i=0; i<tempRec; i++)
            fileBuffer[bytesReceived + i] = tempBuffer[i];

        bytesReceived += tempRec;
        bzero(tempBuffer, sizeof(tempBuffer));
        // memset(fileBuffer, '0', sizeof(fileBuffer));
    }

    // pseek something 
    pwrite(fp, fileBuffer, bytesReceived, offset);

    close(client_Socket);  
}

vector<string> getArguments2(string inputArg){
    vector<string> args;
    int i=0, j=0, n = inputArg.size();
    while(i<n){
        if(i == j && inputArg[i] == ':')
            j++;
        else if(inputArg[i] == ':'){
            args.push_back(inputArg.substr(j, i-j));
            j = i+1;
        }
        i++;
    }
    args.push_back(inputArg.substr(j, i-j));
    return args;
}

string getFileName(string filePath){
    int n = filePath.size() , k = n - 1;
    while(k>=0 && filePath[k--] != '/');
    if(k>0)
        return filePath.substr(k + 2, n - k - 2);
    return filePath;
}

void downloadFromSeeder(vector<string> args){
    vector<string> fileDetails = getArguments2(args[0]);
    string fileName = getFileName(fileDetails[0]), fileSHA = fileDetails[1], fileSize = fileDetails[2];
    int n = args.size() - 2;

    struct Peers s[n/2]; 
    int j = 0;
    for(int i=1; i<(n+1); i += 2){
        s[j].ip = args[i];
        s[j++].port = args[i+1];
    }
    
    long long fileSizeINT = stol(fileSize);
    long long noChunks = fileSizeINT / CHUNKSIZE;
    if(fileSizeINT % CHUNKSIZE != 0)
        noChunks++;
    cout<<"no of chunks: "<<fileSize<<" "<<fileSizeINT<<" "<<noChunks<<endl;

    // chunkNo - <port, ip> 
    map<int, vector<pair<string, string>> > chunkMap;

    enquiryChunks(chunkMap, n, s, fileName, noChunks);
    cout<<"enquiry done returned from function\n";
    
    // for(int i=0; i<noChunks; i++){
    //     for(auto it : chunkMap[i])
    //         cout<<it.first<<" "<<it.second<<"|";
    //     cout<<endl;
    // }
    // pthread_t th[THREAD_NUM];

    char resPath[200];
    string tempPath = args[n+1] + fileName;
    realpath(tempPath.c_str(), resPath);
    string finalPath(resPath);
    filePathMap[fileName] = finalPath;
    downloadsMap[fileName] = "D(Downloading)";

    for(int i=0; i<noChunks; i++){
        // assumimg atleast one seeder is always there
        long long offset = CHUNKSIZE * i;
        if(i == noChunks - 1){
            downloadChunk(fileName, args[n+1], offset, chunkMap[i][0].second, stoi(chunkMap[i][0].first), stoi(fileSize) - i * CHUNKSIZE);
        }
        else{
            int l = chunkMap[i].size();
            if( l == 0){
                cout<<"File can not be downloaded not enough seeders online..\n";
                return;
            }
            int m = rand() % l;
            downloadChunk(fileName, args[n+1], offset, chunkMap[i][m].second, stoi(chunkMap[i][m].first), CHUNKSIZE);
        }

        fileChunkMap[fileName].push_back(i);
    } 
    
    // updating chunkMap
    fileChunkMap[fileName].push_back(-1);

    
    unsigned char c[SHA_DIGEST_LENGTH];
    unsigned char data[CHUNKSIZE];

    SHA_CTX mdContext;
    SHA_CTX mdContext1;
    FILE *upFile = fopen(finalPath.c_str(), "rb");
    // if (upFile == NULL){
    // }

    SHA1_Init(&mdContext);
    long long bytesRead = 0;
    while ( (bytesRead = fread(data, 1, 524288, upFile)) != 0){
        SHA1_Update(&mdContext, data, bytesRead);

        unsigned char c1[SHA_DIGEST_LENGTH];
        SHA1_Init(&mdContext1);
        SHA1_Update(&mdContext1, c1, bytesRead);
        SHA1_Final(c1, &mdContext1);
        stringstream ss;
        string s = "";
        for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
            s+=c1[i];
        
        // shas.push_back(s);
    }

    SHA1_Final(c, &mdContext);
    stringstream ss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
        ss << hex << setw(2) << setfill('0') << (int)c[i];
    // cout<<ss.str()<<endl;
    string generatedSHA = ss.str();  

    if(generatedSHA == fileSHA)
        downloadsMap[fileName] = "C(Complete)";

    // cout << "Total bytes read at the client end = " << sum << " bytes"<< endl;
    cout<<"Download completed, SHA matching: "<<generatedSHA<<" "<<fileSHA<<endl;
}

void addInCommand(string &cmnd){
    if(cmnd.substr(0, 11) == "create_user")
        return;
    else if(cmnd.substr(0, 5) == "login")
        cmnd += + " " + SERVER_IP + ' ' + to_string(SERVER_PORT);

    // after this login req in all functions
    else if(userId.size() == 0)
        cmnd = "NOT_LOGGED";
    else if(cmnd.substr(0, 6) == "logout" && userId.size()>0)
        cmnd += + " " + userId;
    else if(cmnd.substr(0, 12) == "create_group" && userId.size()>0)
        cmnd += + " " + userId;
    else if(cmnd.substr(0, 10) == "join_group" && userId.size()>0)
        cmnd += + " " + userId;
    else if(cmnd.substr(0, 11) == "list_groups" && userId.size()>0)
        cmnd += + " " + userId;
    else if(cmnd.substr(0, 13) == "list_requests" && userId.size()>0)
        cmnd += + " " + userId;
    else if(cmnd.substr(0, 14) == "accept_request" && userId.size()>0)
        cmnd += + " " + userId;
    else if(cmnd.substr(0, 11) == "upload_file" && userId.size()>0){

        vector<string> args = getArguments(cmnd);
        
        string fileName = getFileName(args[1]);
        char resPath[300];
        realpath(args[1].c_str(), resPath);
        cout<<fileName<<" "<<resPath<<endl;
        filePathMap[fileName] = resPath;
        fileChunkMap[fileName].push_back(-1);

        string filePath(resPath);
        ifstream fd(filePath, ios::ate | ios::binary);
        long long file_size = fd.tellg();
        fd.close();

        bool flag = true;
        unsigned char c[SHA_DIGEST_LENGTH];
        unsigned char data[CHUNKSIZE];

        SHA_CTX mdContext;
        SHA_CTX mdContext1;
        FILE *upFile = fopen(filePath.c_str(), "rb");
        if (upFile == NULL){
            flag = false;
            cmnd = "invalid";
        }
        SHA1_Init(&mdContext);
        long long bytesRead = 0;
        while (flag && (bytesRead = fread(data, 1, 524288, upFile)) != 0){
            SHA1_Update(&mdContext, data, bytesRead);

            unsigned char c1[SHA_DIGEST_LENGTH];
            SHA1_Init(&mdContext1);
            SHA1_Update(&mdContext1, c1, bytesRead);
            SHA1_Final(c1, &mdContext1);
            stringstream ss;
            string s = "";
            for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
                s+=c1[i];
            
            // shas.push_back(s);
        }

        SHA1_Final(c, &mdContext);
        stringstream ss;
        for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
            ss << hex << setw(2) << setfill('0') << (int)c[i];
        cout<<ss.str()<<endl;
        string fileSHA = ss.str();

        if(flag){
            struct stat sfile;
            stat(resPath, &sfile);
            string fileSize = to_string(sfile.st_size);
            cout<<"file_size: "<<file_size<<" "<<fileSize<<endl;
            cmnd += + " " + filePath + " " + fileSHA + " " + fileSize + " " + userId;

            FILE *filePathFile;
            filePathFile = fopen("./filePath.txt", "w+");
            fileData f;
            strcpy(f.fileName, fileName.c_str());
            strcpy(f.filePath, filePath.c_str());
            fwrite(&f, sizeof(struct fileData), 1, filePathFile);
            fclose(filePathFile);
        }
        // upload_file fileName/filePath grpId fileSize userId 
    }
    else if(cmnd.substr(0, 10) == "list_files" && userId.size()>0)
        cmnd += + " " + userId;
    else if(cmnd.substr(0, 13) == "download_file" && userId.size()>0)
        cmnd += + " " + userId;
    else if(cmnd.substr(0, 11) == "leave_group" && userId.size()>0)
        cmnd += + " " + userId;
    else if(cmnd.substr(0, 10) == "stop_share" && userId.size()>0)
        cmnd += + " " + userId;
}

string handleEnquiry(string args){
    vector<string> temp = getArguments(args);
    string fileName = temp[1], reply = "";

    vector<int> chunks = fileChunkMap[fileName];
    if(chunks.size() == 0 || logged == false)
        reply += "-2";
    else if(find(chunks.begin(), chunks.end(), -1) != chunks.end())
        reply += "-1";
    else if(find(chunks.begin(), chunks.end(), -2) != chunks.end())
        reply += "-2";
    else
        for(auto no : chunks)
            reply += to_string(no);
    return reply;
}

void handleChunkDownload(string fileName, long long offset, long long reqBytes){
    string filePath = filePathMap[fileName];

    //file opening change
    int fp = open(filePath.c_str(), 00);
    
    //sending file
    bzero(chunkBuffer, sizeof(chunkBuffer));
    
    // reading at offset 
    int nread = pread(fp, chunkBuffer, reqBytes, offset);
    cout<<nread<<" bytes read\n";
    
    // int nread = read(fp, fileBuffer, CHUNKSIZE);      
}

void *handleClient(void *pclient_clientSocket){
    int clientSocket = *((int *)pclient_clientSocket);
    free(pclient_clientSocket);
    // free(pclient_clientSocket);

    //receiving file name
    bzero(buffer, sizeof(buffer));
    read(clientSocket, buffer, 1024);
    string fileName(buffer), reply;
    // cout<<fileName<<" :received from peer"<<endl;
    if(fileName.substr(0, 7) == "enquiry"){
        string enReply = handleEnquiry(fileName);
        write(clientSocket, enReply.c_str(), enReply.size());
        close(clientSocket);  
        return NULL;
    }
    else if(fileName.substr(0, 5) == "chunk"){
        vector<string> tempArgs = getArguments(fileName);
        // cout<<"Bef: "<<sizeof(chunkBuffer)<<endl;
        handleChunkDownload(tempArgs[1], stoi(tempArgs[2]), stoi(tempArgs[3]));
        // cout<<"after: "<<sizeof(chunkBuffer)<<endl;

        write(clientSocket, chunkBuffer, stoi(tempArgs[3]));
        close(clientSocket);
        return NULL;
    }
    
    if(filePathMap.find(fileName) == filePathMap.end()){
        cout<<"Missing file\n";
        reply = "File Missing..";
        // updating status of download
        write(clientSocket, reply.c_str(), reply.size());
        return NULL;
    }

    // updating status of download
    reply = "Sending..";
    write(clientSocket, reply.c_str(), reply.size());

    string filePath = filePathMap[fileName];
    int fp = open(filePath.c_str(), 00);
    cout<<fp<<endl;
    //sending file
    char fileBuffer[CHUNKSIZE];
    int offset = 0;
    while(1){
        bzero(fileBuffer, sizeof(fileBuffer));
        // memset(fileBuffer, '0', sizeof(fileBuffer));
        int nread = read(fp, fileBuffer, CHUNKSIZE);
        offset += CHUNKSIZE;
        cout<<nread<<" "<<offset<<endl;
        if (nread > 0) {
            // printf("Sending \n");
            write(clientSocket, fileBuffer, sizeof(fileBuffer));
            // bzero(fileBuffer, sizeof(fileBuffer));
            // sum = sum + nread;
        }
        else if (nread == 0) {
            cout<<"EOF\n";
            // memset(fileBuffer, '0', sizeof(fileBuffer));
            string end = "EOF";
            write(clientSocket, end.c_str(), sizeof(end));
            break;
        }
        else{
            strcpy(fileBuffer, "Error Occured While Downloading..");
            write(clientSocket, fileBuffer, sizeof(fileBuffer));
            break;
        }
    }

        // close(client_Socket);  
    return NULL;
}

void *setupServer(void * pclient_SERVER_PORT){
    string ipPort((char *)pclient_SERVER_PORT);
    int found = ipPort.find_last_of(':');
    SERVER_IP = ipPort.substr(0, found);
    SERVER_PORT = stoi(ipPort.substr(found+1, ipPort.size()-found));

    int sockfd, ret;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    int clientSocket;
    struct sockaddr_in newAddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    errorString(sockfd, "[-][PEER]Error in connection.\n");
    // printf("[+][PEER]Server Socket is created.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP.c_str());

    ret = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    errorString(ret, "[-][PEER]Error in binding.\n");
    // printf("[+][PEER]Bind to port %d\n", SERVER_PORT);
    errorString(listen(sockfd, 10), "[-][PEER]Error in binding.\n");
    printf("[+][PEER]Binded to port %d and listening....\n", SERVER_PORT);

	while (1){
        clientSocket = accept(sockfd, (struct sockaddr *)&newAddr, &addr_size);
        errorString(clientSocket, "[-]Error in acceoting connection req");
        printf("[+]Connection accepted from %s:%d: ", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));

        pthread_t ptid;
        int *pclient = (int *)malloc(sizeof(int));
        *pclient = clientSocket;
        // pthread_create(&t, NULL, thread_function, argument)
        pthread_create(&ptid, NULL, handleClient, pclient);
    }

    close(clientSocket);
}

int main(int argc, char* argv[]){

    loadFilePath();
    pthread_t ptid;
	char* SERVER_IP_PORT_FOR_CLIENT = argv[1];
	pthread_create(&ptid, NULL, setupServer, SERVER_IP_PORT_FOR_CLIENT);

    string trackerData(argv[2]);
    string ipPort[2], tempIPPORT;
    fstream info(trackerData.c_str());

    for(int i=0; i<2; i++){
        info >> tempIPPORT;
        ipPort[i] = tempIPPORT;
    } 
	int client_Socket, ret;
	struct sockaddr_in serverAddr;
	char buffer[1024];

	client_Socket = socket(AF_INET, SOCK_STREAM, 0);
	errorString(client_Socket, "[-][CLIENT]Error in connection.\n");
	// printf("[+][CLIENT]Client Socket is created.\n");

    

    for(int i=0; i<2; i++){        
        string trackerIPPORT(ipPort[i]);
        int found = trackerIPPORT.find_last_of(':');
        string TRACKER_IP = trackerIPPORT.substr(0, found);
        int TRACKER_PORT = stoi(trackerIPPORT.substr(found+1, trackerIPPORT.size()-found));

        memset(&serverAddr, '\0', sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(TRACKER_PORT);
        serverAddr.sin_addr.s_addr = inet_addr(TRACKER_IP.c_str());

        ret = connect(client_Socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if(ret == 0){
            printf("[+][CLIENT]Client Socket is created and Connected to Server.\n");
            break;
        }
    }

	while(1){
		printf("Client: \t");
        string commandForTracker;
        getline(cin, commandForTracker, '\n');

        if(strcmp(commandForTracker.substr(0,5).c_str(), ":exit") == 0){
			close(client_Socket);
			printf("[-][CLIENT]Disconnected from server.\n");
			exit(1);
		}
        addInCommand(commandForTracker);
		send(client_Socket, commandForTracker.c_str(), commandForTracker.size(), 0);

        bzero(buffer, sizeof(buffer));
        read(client_Socket, buffer, 1024);
        string reply(buffer);
		
        if(reply.size() < 0)
			printf("[-][CLIENT]Error in receiving data.\n");
		else if(commandForTracker.substr(0, 5) == "login" && reply=="User logged in Successfully"){
            logged = true;
            vector<string> args = getArguments(commandForTracker);
            userId = args[1];
        }
		else if(commandForTracker.substr(0, 6) == "logout" && reply=="User logged Out Successfully"){
            logged = false;
            vector<string> args = getArguments(commandForTracker);
            userId = "";
        }
        else if(strcmp(commandForTracker.substr(0,13).c_str(), "download_file") == 0){
            vector<string> sendArgs = getArguments(commandForTracker);
            vector<string> downloadArgs = getArguments(reply);
            if(downloadArgs[0] != "Failed!"){
                downloadFromSeeder(downloadArgs);
                reply = "Download Complete";
                
                // // adding this user to seeder list
                // vector<string> fileDetails = getArguments2(downloadArgs[0]);
                // string fileName = getFileName(fileDetails[0]);
                
                // char resPath[300];
                // realpath(downloadArgs[1].c_str(), resPath);
                // filePathMap[fileName] = resPath;
                // fileChunkMap[fileName].push_back(-1);
                // // addSeeder grpId fileName userId
                // commandForTracker = "addSeeder " + sendArgs[1] + " " + fileName + " " + userId;

                // send(client_Socket, commandForTracker.c_str(), commandForTracker.size(), 0);
                
                // bzero(buffer, sizeof(buffer));
                // read(client_Socket, buffer, 1024);
                // string reply(buffer);
		
                bzero(buffer, sizeof(buffer));
                strcpy(buffer, reply.c_str());
            }
        }
        else if(strcmp(commandForTracker.substr(0,14).c_str(), "show_downloads") == 0){
            for (auto mp: downloadsMap)
                reply += mp.first + "\t" + mp.second + "\n";

            bzero(buffer, sizeof(buffer));
            strcpy(buffer, reply.c_str());
        }
        
        printf("Server: \t%s\n", buffer);
		
	}
	return 0;
}