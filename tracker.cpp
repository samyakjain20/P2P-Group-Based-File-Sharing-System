#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
using namespace std;
// std::thread t1(callable)

#define CHUNK 1024
char buffer[1024];

void errorString(int err, string s){
    if (err < 0){ // error occured
        if (s != "")
            cout << s;
        exit(1);
    }
}

struct args{
    int socket, port;
    string ip;
};

struct User{
    char id[50], pass[50];
};

struct Group{
    char grpId[50], userId[50];
};

struct FileData{
    char fileName[200], grpId[50], fileSize[20];
};
struct FileWithPeers{
    char grpId[50], fileName[200], fileSHA[50],  fileSize[20], userId[50];
};

map<string, User> userMap;
// userId - password
map<string, string> userLogin;
// userId - ipPort
map<string, string> userSocket;
// grpId - userIds
map<string, vector<string>> grpMap;
// grpId - Owner
map<string, string> grpOwner;
// grpId - userIds (joinreq)
map<string, vector<string>> listRqs;

// "grpId" - fileData(with fileName unique),
map<string, vector<string>> grpFiles;

// <grpId, fileName> - <fileSHA, fileSize>
map< pair<string, string>, pair<string, string> > grpFileSize;

// <grpId , fileName> - <filesize, userIds>
map< pair<string, string>, pair<string, vector<string>> > peersHavingFile;

void loadGrpMemberData();
void loadGrpOwnerData();
void loadListReqData();
// void loadGrpFilesData();
void loadFileWithPeersData();

void loadUserData(){
    FILE *userFile;
    userFile = fopen("./data/user.txt", "r");
    User u;
    if (userFile == NULL){
        fprintf(stderr, "\nError opening user.txt file\n");
        return;
    }
    while (fread(&u, sizeof(struct User), 1, userFile)){
        userMap[u.id] = u;
        cout << u.id << " " << u.pass << endl;
    }
    fclose(userFile);

    loadGrpMemberData();
    loadGrpOwnerData();
    loadListReqData();
    // loadFileWithPeersData();
}

void loadGrpMemberData(){
    FILE *grpMember;
    grpMember = fopen("./data/grpMember.txt", "r");
    cout << "grp member: \n";
    if (grpMember == NULL){
        fprintf(stderr, "\nError opening grpMember.txt file\n");
        return;
    }
    Group g;
    while (fread(&g, sizeof(struct Group), 1, grpMember)){
        grpMap[g.grpId].push_back(g.userId);
        cout << g.grpId << " " << g.userId << endl;
    }
    fclose(grpMember);
}
void loadGrpOwnerData(){

    FILE *ownerFile;
    ownerFile = fopen("./data/owner.txt", "r");
    cout << "grp owner: \n";
    if (ownerFile == NULL){
        fprintf(stderr, "\nError opening owner.txt file\n");
        return;
    }
    Group g;
    while (fread(&g, sizeof(struct Group), 1, ownerFile)){
        grpOwner[g.grpId] = g.userId;
        cout << g.grpId << " " << g.userId << endl;
    }
    fclose(ownerFile);
}

void loadListReqData(){
    FILE *listReq;
    listReq = fopen("./data/listReq.txt", "r");
    cout << "join req: \n";
    if (listReq == NULL){
        fprintf(stderr, "\nError opening listReq.txt file\n");
        return;
    }
    Group g;
    while (fread(&g, sizeof(struct Group), 1, listReq)){
        listRqs[g.grpId].push_back(g.userId);
        cout << g.grpId << " " << g.userId << endl;
    }
    fclose(listReq);
}

// void loadGrpFilesData(){
//     FILE *filesUploaded;
//     filesUploaded = fopen("./grpFiles.txt", "r");
//     cout << "join req: \n";
//     if (filesUploaded == NULL){
//         fprintf(stderr, "\nError opening grpFiles.txt file\n");
//         return;
//     }
//     FileData f;
//     while (fread(&f, sizeof(struct FileData), 1, filesUploaded)){
//         grpFiles[f.grpId].push_back(f.fileName);
//         grpFileSize[{f.grpId, f.fileName}] = f.fileSize;
//         cout << f.grpId << " " << f.fileName << " " << f.fileSize << endl;
//     }
//     fclose(filesUploaded);
// }

void loadFileWithPeersData(){
    FILE *filePeers;
    filePeers = fopen("./data/filePeers.txt", "r");
    cout << "join req: \n";
    if (filePeers == NULL){
        fprintf(stderr, "\nError opening filePeers.txt file\n");
        return;
    }
    FileWithPeers fp;
    cout<<"Loading data from filePeers.txt\n";
    while (fread(&fp, sizeof(struct FileWithPeers), 1, filePeers)){
        if(peersHavingFile.find({fp.grpId, fp.fileName}) == peersHavingFile.end()){
            cout<<fp.grpId<<" "<<fp.fileName<<" "<<fp.fileSHA<<" "<<fp.fileSize<<" "<<fp.userId<<endl;
            vector<string> peers;
            peers.push_back(fp.userId);
            grpFiles[fp.grpId].push_back(fp.fileName);
            grpFileSize[{fp.grpId, fp.fileName}] = {fp.fileSHA, fp.fileSize};
            peersHavingFile[{fp.grpId, fp.fileName}] = {fp.fileSize, peers};
        }
        else{
            vector<string> peers = peersHavingFile[{fp.grpId, fp.fileName}].second;
            peers.push_back(fp.fileName);
            peersHavingFile[{fp.grpId, fp.fileName}] = {fp.fileSize, peers};
        }
    }
    fclose(filePeers);
}



void updateGroupData(){
    FILE *grpMember;
    grpMember = fopen("./data/grpMember.txt", "w+");

    cout << "updating grpMember data: ";
    for (auto mp : grpMap){
        vector<string> mem = mp.second;
        for (auto it : mem){
            Group g;
            strcpy(g.grpId, mp.first.c_str());
            strcpy(g.userId, it.c_str());
            fwrite(&g, sizeof(struct Group), 1, grpMember);
        }
    }
    fclose(grpMember);
}
void updateOwnerData(){
    FILE *ownerFile;
    ownerFile = fopen("./data/owner.txt", "w+");

    cout << "updating ownerFile data: ";
    for (auto mp : grpOwner){
        Group g;
        strcpy(g.grpId, mp.first.c_str());
        strcpy(g.userId, mp.second.c_str());
        fwrite(&g, sizeof(struct Group), 1, ownerFile);
    }

    fclose(ownerFile);
}

void updateFileReq(){
    FILE *listReq;
    listReq = fopen("./data/listReq.txt", "w+");

    cout << "updating fileReq data: ";
    for (auto mp : listRqs){
        vector<string> mem = mp.second;
        for (auto it : mem){
            Group g; // list req data
            strcpy(g.grpId, mp.first.c_str());
            strcpy(g.userId, it.c_str());
            fwrite(&g, sizeof(struct Group), 1, listReq);
        }
    }
    fclose(listReq);
}

void updateFileWithPeersData(){
    FILE *filePeers;
    filePeers = fopen("./data/filePeers.txt", "w+");
    cout << "updating uploaded files : ";
    for (auto grp : grpMap){
        vector<string> files = grpFiles[grp.first];
        for(auto file : files){
            string fileSize = peersHavingFile[{grp.first, file}].first;
            vector<string> peers = peersHavingFile[{grp.first, file}].second;
            for(auto peer: peers){
                FileWithPeers fp;
                strcpy(fp.grpId, grp.first.c_str());
                strcpy(fp.fileName, file.c_str());
                strcpy(fp.fileSHA, grpFileSize[{grp.first, file}].first.c_str());
                strcpy(fp.fileSize, fileSize.c_str());
                strcpy(fp.userId, peer.c_str());
                fwrite(&fp, sizeof(struct FileWithPeers), 1, filePeers);
            }
        }
    }        
    fclose(filePeers);
}


string createUser(string id, string pass){
    if (userMap.find(id) != userMap.end())
        return "User already exists.";
    // struct person input1 = {1, "rohan", "sharma"};
    struct User u;
    strcpy(u.id, id.c_str());
    strcpy(u.pass, pass.c_str());
    userMap[id] = u;

    FILE *userFile;
    userFile = fopen("./data/user.txt", "a+");
    cout << "writing user in file: ";
    fwrite(&u, sizeof(struct User), 1, userFile);
    fclose(userFile);

    return "User created Successfully";
}

string loginUser(string id, string pass, string IP, string PORT){
    User u = userMap[id];
    if (userMap.find(id) == userMap.end())
        return "User does not exists.";
    else if (strcmp(pass.c_str(), u.pass) == 0){
        userSocket[id] = IP + " " + PORT;
        cout << userSocket[id] << endl;
        return "User logged in Successfully"; // do not change this message ever
    }
    return "Incorrect login password.";
}
string logoutUser(string userId){
    if (userSocket.find(userId) == userSocket.end())
        return "User already not logged in.";

    userSocket.erase(userId);
    return "User logged Out Successfully"; // do not change this message ever
}

string createGroup(string grpId, string userId){

    if (grpMap.find(grpId) != grpMap.end())
        return "Group already existed.";
    else{
        grpOwner[grpId] = userId;
        grpMap[grpId].push_back(userId); // grp users map

        struct Group g;
        strcpy(g.grpId, grpId.c_str());
        strcpy(g.userId, userId.c_str());
        FILE *ownerFile, *grpMember;
        ownerFile = fopen("./data/owner.txt", "a+");
        grpMember = fopen("./data/grpMember.txt", "a+");
        cout << "writing in owner/grpMember file: ";
        fwrite(&g, sizeof(struct Group), 1, ownerFile);
        fwrite(&g, sizeof(struct Group), 1, grpMember);
        fclose(ownerFile);
        fclose(grpMember);

        return "Group created Successfully";
    }
}

string joinGroup(string grpId, string userId){
    // joining request implementation remaining
    if (grpMap.find(grpId) == grpMap.end())
        return "Group does not exists.";
    else{
        vector<string> list = listRqs[grpId];
        auto it = find(list.begin(), list.end(), userId);
        if (it != list.end())
            return "Join group request alredy pending.";
        list = grpMap[grpId];
        it = find(list.begin(), list.end(), userId);
        if (it != list.end())
            return "Already a member of the group.";

        listRqs[grpId].push_back(userId);

        struct Group g;
        strcpy(g.grpId, grpId.c_str());
        strcpy(g.userId, userId.c_str());
        FILE *listReq;
        listReq = fopen("./data/listReq.txt", "a+");
        cout << "writing listReq file: ";
        fwrite(&g, sizeof(struct Group), 1, listReq);
        fclose(listReq);

        return "Join group request added.";
    }
}

string listGroups(string userId){
    // joining request implementation remaining
    string reply = "List of groups in network:\n";
    for (auto mp : grpMap)
        reply += mp.first + "\n";
    return reply;
}
string listRequest(string grpId, string userId){
    // joining request implementation remaining
    if (grpMap.find(grpId) == grpMap.end())
        return "Group does not exists.";
    else if (grpOwner[grpId] != userId)
        return "Owner access required.";
    else{
        vector<string> list = grpMap[grpId];
        auto it = find(list.begin(), list.end(), userId);
        if (it == list.end())
            return "Not a member of the group";

        list = listRqs[grpId];
        string reply = "List of pending request:\n";
        for (int i = 0; i < list.size(); i++)
            reply += list[i] + "\n";
        return reply;
    }
}

string acceptRequest(string grpId, string userId, string ownerId){
    // joining request implementation remaining
    if (grpMap.find(grpId) == grpMap.end())
        return "Group does not exists.";
    else if (grpOwner[grpId] != ownerId)
        return "Owner can access required.";
    else{
        vector<string> list = listRqs[grpId];
        auto it = find(list.begin(), list.end(), userId);
        if (it == list.end())
            return "Request not found";
        list.erase(it);
        listRqs[grpId] = list;
        grpMap[grpId].push_back(userId);

        struct Group g;
        strcpy(g.grpId, grpId.c_str());
        strcpy(g.userId, userId.c_str());
        FILE *grpMember;
        grpMember = fopen("./data/grpMember.txt", "a+");
        cout << "writing in grpMember file: ";
        fwrite(&g, sizeof(struct Group), 1, grpMember);
        fclose(grpMember);

        updateFileReq(); // user is added request need to be removed

        return "Request accepted";
    }
}

string leaveGroup(string grpId, string userId){
    if (grpMap.find(grpId) == grpMap.end())
        return "Group does not exists.";
    else{
        vector<string> members = grpMap[grpId];
        auto it = find(members.begin(), members.end(), userId);
        if (it == members.end())
            return "Already not a member of the group";

        members.erase(it);
        if (members.size() == 0){
            grpMap.erase(grpId);
            grpOwner.erase(grpId);
        }
        updateGroupData(); // updating data of the grpMember file

        grpMap[grpId] = members; // grp users map
        if (grpOwner[grpId] == userId){ // if owner is only leaving the group
            grpOwner[grpId] = members[0];
            updateOwnerData(); // updating data of the owner file
        }

        bool needsUpdation = false;
        vector<string> files = grpFiles[grpId];
        for (auto file : files){
            vector<string> peers = peersHavingFile[{grpId, file}].second;
            auto it = find(peers.begin(), peers.end(), userId);
            if(it != peers.end()){
                peers.erase(it);
                peersHavingFile[{grpId, file}] = {grpFileSize[{grpId, file}].second, peers};
                needsUpdation = true;
            }
        }        
        if(needsUpdation)
            updateFileWithPeersData();

        return "Removed from group Successfully";
    }
}

string getFileName(string filePath){
    int n = filePath.size() , k = n - 1;
    while(k>=0 && filePath[k--] != '/');
    if(k>0)
        return filePath.substr(k + 2, n - k - 2);
    return filePath;
}

string stopShare(string grpId, string fileName, string userId){
    fileName = getFileName(fileName);
    vector<string> peers = peersHavingFile[{grpId, fileName}].second;
    auto it = find(peers.begin(), peers.end(), userId);
    if(it != peers.end()){
        peers.erase(it);
        peersHavingFile[{grpId, fileName}] = {grpFileSize[{grpId, fileName}].second, peers};
        updateFileWithPeersData();
    }

    return "Stoped sharing "+ fileName;
}

string uploadFile(string fileName, string grpId, string fileUserPath, string fileSHA, string fileSize, string userId){
    fileName = getFileName(fileName);
    // joining request implementation remaining
    if (grpMap.find(grpId) == grpMap.end())
        return "Group does not exists.";
    else{
        vector<string> list = grpMap[grpId];
        auto it = find(list.begin(), list.end(), userId);
        if (it == list.end())
            return "Not a member of the group";

        // cout<<grpId<<" "<<fileName<<"asdc\n";
        vector<string> files = grpFiles[grpId];
        if(find(files.begin(), files.end(), fileName) != files.end()){
            vector<string> peers = peersHavingFile[{grpId, fileName}].second;
            if(find(peers.begin(), peers.end(), userId) == peers.end()){
                peers.push_back(userId);
                peersHavingFile[{grpId, fileName}] = {fileSize, peers};
            }
        }
        else{
            grpFiles[grpId].push_back(fileName);
            grpFileSize[{grpId, fileName}] = {fileSHA, fileSize};
            vector<string> peers;
            peers.push_back(userId);
            peersHavingFile[{grpId, fileName}] = {fileSize, peers};
        }

        // bahot kaam bacha hai
        // FileData f;
        // strcpy(f.fileName, fileName.c_str());
        // strcpy(f.grpId, grpId.c_str());
        // strcpy(f.fileSize, fileSize.c_str());
        // FILE *filesUploaded;
        // filesUploaded = fopen("./grpFiles.txt", "a+");
        // fwrite(&f, sizeof(struct FileData), 1, filesUploaded);
        // fclose(filesUploaded);

        FileWithPeers fp;
        strcpy(fp.grpId, grpId.c_str());
        strcpy(fp.fileName, fileName.c_str());
        strcpy(fp.fileSHA, fileSHA.c_str());
        strcpy(fp.fileSize, fileSize.c_str());
        strcpy(fp.userId, userId.c_str());
        FILE *filePeers;
        filePeers = fopen("./data/filePeers.txt", "a+");
        fwrite(&fp, sizeof(struct FileWithPeers), 1, filePeers);
        fclose(filePeers);

        return "File upload done";
    }
}
string list_files(string grpId, string userId){
    // joining request implementation remaining
    if (grpMap.find(grpId) == grpMap.end())
        return "Group does not exists.";
    else{
        vector<string> list = grpMap[grpId];
        auto it = find(list.begin(), list.end(), userId);
        if (it == list.end())
            return "Not a member of the group";

        vector<string> files = grpFiles[grpId];
        string reply = "List of sharable files:\n";
        for (auto file : files){
            vector<string> peers = peersHavingFile[{grpId, file}].second;
            if(peers.size() > 0)
                reply += file + "\n";
        }

        return reply;
    }
}

string downloadFile(string grpId, string fileName, string dest, string userId){
    if (grpMap.find(grpId) == grpMap.end()){
        cout << "Failed! Group does not exists.";
        return "Failed! Group does not exists.";
    }
    else{
        vector<string> list = grpMap[grpId];
        auto it = find(list.begin(), list.end(), userId);
        if (it == list.end()){
            cout << "Failed! Not a member of the group";
            return "Failed! Not a member of the group";
        }

        // vector<string> files = grpFiles[grpId];
        // auto it = find(files.begin(), files.end(), userId);
        // if(it == files.end()){
        //     cout<<"Failed! File has not been shared in the group";
        //     return "Failed! File has not been shared in the group";
        // }

        auto itr = peersHavingFile[{grpId, fileName}];
        vector<string> peers = itr.second;
        if (peers.size() > 0){
            string reply = fileName + ":" + grpFileSize[{grpId, fileName}].first + ":" + itr.first + " ";
            for (int i = 0; i < peers.size(); i++)
                reply += userSocket[peers[i]] + " ";
            reply += dest;
            cout<<reply<<endl;

            // adding to seeder list
            peers.push_back(userId);
            peersHavingFile[{grpId, fileName}] = {grpFileSize[{grpId, fileName}].second, peers};

            FileWithPeers fp;
            strcpy(fp.grpId, grpId.c_str());
            strcpy(fp.fileName, fileName.c_str());
            strcpy(fp.fileSHA, grpFileSize[{grpId, fileName}].first.c_str());
            strcpy(fp.fileSize,  grpFileSize[{grpId, fileName}].first.c_str());
            strcpy(fp.userId, userId.c_str());
            FILE *filePeers;
            filePeers = fopen("./data/filePeers.txt", "a+");
            fwrite(&fp, sizeof(struct FileWithPeers), 1, filePeers);
            fclose(filePeers);

            return reply;
        }

        return "Failed! File not found in sharables.";
    }
}

string addSeeder(string grpId, string fileName, string userId){

    vector<string> peers = peersHavingFile[{grpId, fileName}].second;
    peers.push_back(userId);
    peersHavingFile[{grpId, fileName}] = {grpFileSize[{grpId, fileName}].second, peers};

    FileWithPeers fp;
    strcpy(fp.grpId, grpId.c_str());
    strcpy(fp.fileName, fileName.c_str());
    strcpy(fp.fileSHA, grpFileSize[{grpId, fileName}].first.c_str());
    strcpy(fp.fileSize, grpFileSize[{grpId, fileName}].second.c_str());
    strcpy(fp.userId, userId.c_str());
    FILE *filePeers;
    filePeers = fopen("./data/filePeers.txt", "a+");
    fwrite(&fp, sizeof(struct FileWithPeers), 1, filePeers);
    fclose(filePeers);

    return "Downloaded file: " + fileName;
}

vector<string> getArguments(string inputArg){
    vector<string> args;
    int i = 0, j = 0, n = inputArg.size();
    while (i < n){
        if (i == j && inputArg[i] == ' ')
            j++;
        else if (inputArg[i] == ' '){
            args.push_back(inputArg.substr(j, i - j));
            j = i + 1;
        }
        i++;
    }
    args.push_back(inputArg.substr(j, i - j));
    return args;
}

string processCommand(char *temp){
    string command(temp);
    vector<string> cmnd = getArguments(temp);
    if (cmnd[0] == "NOT_LOGGED")
        return "User Login required";
    else if (cmnd[0] == "create_user" && cmnd.size() == 3)
        return createUser(cmnd[1], cmnd[2]);
    else if (cmnd[0] == "login" && cmnd.size() == 5)
        return loginUser(cmnd[1], cmnd[2], cmnd[3], cmnd[4]);
    else if (cmnd[0] == "logout" && cmnd.size() == 2)
        return logoutUser(cmnd[1]);

    else if (cmnd[0] == "create_group" && cmnd.size() == 3)
        return createGroup(cmnd[1], cmnd[2]);
    else if (cmnd[0] == "join_group" && cmnd.size() == 3)
        return joinGroup(cmnd[1], cmnd[2]);
    else if (cmnd[0] == "leave_group" && cmnd.size() == 3)
        return leaveGroup(cmnd[1], cmnd[2]);

    else if (cmnd[0] == "list_groups" && cmnd.size() == 2)
        return listGroups(cmnd[1]);
    else if (cmnd[0] == "list_requests" && cmnd.size() == 3)
        return listRequest(cmnd[1], cmnd[2]);
    else if (cmnd[0] == "accept_request" && cmnd.size() == 4)
        return acceptRequest(cmnd[1], cmnd[2], cmnd[3]);

    else if (cmnd[0] == "upload_file" && cmnd.size() == 7)
        return uploadFile(cmnd[1], cmnd[2], cmnd[3], cmnd[4], cmnd[5], cmnd[6]);
    else if (cmnd[0] == "list_files" && cmnd.size() == 3)
        return list_files(cmnd[1], cmnd[2]);
    else if (cmnd[0] == "download_file" && cmnd.size() == 5)
        return downloadFile(cmnd[1], cmnd[2], cmnd[3], cmnd[4]);
    else if (cmnd[0] == "show_downloads")
        return "Downloading Status:\n";
    else if (cmnd[0] == "addSeeder" && cmnd.size() == 4)
        return addSeeder(cmnd[1], cmnd[2], cmnd[3]);
    else if (cmnd[0] == "stop_share" && cmnd.size() == 4)
        return stopShare(cmnd[1], cmnd[2], cmnd[3]);
    else
        return "invalid command.";
}

void *handleConnection(void *pclient_clientSocket){
    // struct args threadArgs = *((struct args *)pclient_clientSocket);
    // int clientSocket = threadArgs.socket;
    // cout<<"hello\n";
    int clientSocket = *((int *)pclient_clientSocket);
    free(pclient_clientSocket);
    int zero = 0;
    while (1){
        int a = recv(clientSocket, buffer, 1024, 0);
        // cout<<a<<endl;
        if(a == 0)
            zero++;
        else
            zero = 0;
        if(zero == 2)
            return NULL;
        if (strcmp(buffer, ":exit") == 0){
            bzero(buffer, sizeof(buffer));
            printf("Disconnected from Server\n");
            break;
            // printf("Disconnected from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
        }
        else{
            printf("Client: %s\n", buffer);
            string clientInput(buffer);
            string reply;
            // if(clientInput.substr(0, 5) == "login")

            reply = processCommand(buffer);
            // cout<<reply<<endl;
            write(clientSocket, reply.c_str(), reply.size());
            bzero(buffer, sizeof(buffer));
        }
    }
    return NULL;
}

// int main(){
int main(int argc, char *argv[]){

    loadUserData();
    int trackerNo = stoi(argv[2]);
    string trackerData(argv[1]);
    string ipPort(argv[1]);
    fstream info(trackerData.c_str());

    for(int i=0; i<trackerNo; i++)
        info >> ipPort; 

    int found = ipPort.find_last_of(':');
    string IP = ipPort.substr(0, found);
    int SERVERPORT = stoi(ipPort.substr(found + 1, ipPort.size() - found));

    int sockfd, ret;
    struct sockaddr_in serverAddr;

    int clientSocket;
    struct sockaddr_in newAddr;

    socklen_t addr_size;

    char buffer[1024];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    errorString(sockfd, "[-]Error in connection.\n");
    printf("[+]Server Socket is created.\n");

    memset(&serverAddr, '\0', sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVERPORT);
    serverAddr.sin_addr.s_addr = inet_addr(IP.c_str());

    ret = bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    errorString(ret, "[-]Error in binding.\n");
    printf("[+]Bind to port %d\n", SERVERPORT);
    errorString(listen(sockfd, 10), "[-]Error in binding.\n");
    printf("[+]Listening....\n");

    while (1){
        clientSocket = accept(sockfd, (struct sockaddr *)&newAddr, &addr_size);
        errorString(clientSocket, "[-]Error in acceoting connection req");
        printf("[+]Connection accepted from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));

        pthread_t ptid;
        int *pclient = (int *)malloc(sizeof(int));
        *pclient = clientSocket;
        // struct args *pclient = (struct args *)malloc(sizeof (struct args));
        // pclient->socket = clientSocket;
        // pclient->ip = inet_ntoa(newAddr.sin_addr);
        // pclient->port = (newAddr.sin_port);

        // pthread_create(&t, NULL, thread_function, argument)
        pthread_create(&ptid, NULL, handleConnection, pclient);
    }

    close(clientSocket);
    return 0;
}