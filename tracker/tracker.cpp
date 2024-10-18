#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <bits/stdc++.h>
#include <fcntl.h>
#include <thread>
#include <chrono>

using namespace std;
// #define PORT 8080

#define PIECESIZE 524288
#define BUFFER_SIZE 65536

string trackerIpAddress;
string trackerPort;
int trackerNo;
struct userValidationDetails
{
public:
    string userName;
    string password;
    bool isAlive;
    string peerServerIpAddress;
    string peerServerPort;
    int port;
    userValidationDetails()
    {
    }
    userValidationDetails(string name, string pass, bool isA)
    {
        userName = name;
        password = pass;
        isAlive = isA;
    }
    userValidationDetails(string name, string pass, bool isA, int po)
    {
        userName = name;
        password = pass;
        isAlive = isA;
        port = po;
    }
    void updateStatus(bool status, int por, string ipAddress, string serverPort)
    {
        isAlive = status;
        port = por;
        peerServerIpAddress = ipAddress;
        peerServerPort = serverPort;
    }
};

class group
{
public:
    string groupId;
    string groupOwner;
    vector<string> groupMembers;
    vector<string> sharedFiles;
    vector<string> pendingRequests;
    group()
    {
    }
    group(string id, string owner)
    {
        groupId = id;
        groupOwner = owner;
        groupMembers.push_back(owner);
    }
    group(string id, string owner, vector<string> members, vector<string> files, vector<string> requests)
    {
        groupId = id;
        groupOwner = owner;
        groupMembers = members;
        sharedFiles = files;
        pendingRequests = requests;
    }
};

class file
{
public:
    string fileName;
    string fileSize;
    string groupId;
    int noOfPieces;
    string pieceWiseHashPath;
    vector<string> seeders;
    file()
    {
    }
    file(string fName, string fSize, string gId, string upBy, string pwhPath)
    {
        fileName = fName;
        fileSize = fSize;
        groupId = gId;
        long long size = stoll(fSize);
        noOfPieces = noOfPieces = ceil((float)size / PIECESIZE);
        pieceWiseHashPath = pwhPath;
        seeders.push_back(upBy);
    }
    file(string fName, string fSize, string gId, string noOfChunks, string pwhPath, vector<string> uploaders)
    {
        fileName = fName;
        fileSize = fSize;
        groupId = gId;
        noOfPieces = stoi(noOfChunks);
        pieceWiseHashPath = pwhPath;
        for (auto each : uploaders)
        {
            seeders.push_back(each);
        }
    }
};

map<string, userValidationDetails> loginValidation;

map<string, group> groups;

map<string, file> files;

vector<string> splitString(string buffer)
{

    stringstream ss(buffer);
    vector<string> splittedString;
    string word;
    while (ss >> word)
    {
        splittedString.push_back(word);
    }
    return splittedString;
}

bool loadData()
{
    // opening validationData and loading into loginValidation file
    string filename("validationData.txt");
    ifstream input_file(filename);
    if (!input_file.is_open())
    {
        cerr << "Could not open the file - '"
             << filename << "'" << endl;
        return false;
    }
    string userName, password;
    while (input_file >> userName and input_file >> password)
    {
        loginValidation[userName] = userValidationDetails(userName, password, false);
    }
    // open group data and load its content
    string filename1("groupData.txt");
    ifstream input_file1(filename1);
    if (!input_file1.is_open())
    {
        cerr << "Could not open the file - '"
             << filename1 << "'" << endl;
        return false;
    }
    string groupName, groupOwner;
    vector<string> groupMembers;
    vector<string> sharedFiles;
    vector<string> pendingRequests;
    string data;
    int i = 0;
    while (getline(input_file1, data))
    {
        vector<string> splittedString = splitString(data);
        // cout << data << i << endl;
        if (i == 0)
            groupName = splittedString[0];
        else if (i == 1)
            groupOwner = splittedString[0];
        else if (i == 2)
            groupMembers = splittedString;
        else if (i == 3)
        {
            if (data != "###")
                sharedFiles = splittedString;
            else
                sharedFiles.clear();
        }
        else if (i == 4)
        {
            if (data != "###")
                pendingRequests = splittedString;
            else
                pendingRequests.clear();
        }
        i++;
        if (i == 5)
        {
            groups[groupName] = group(groupName, groupOwner, groupMembers, sharedFiles, pendingRequests);
            // cout << groups.size() << endl;
            i = 0;
        }
    }
    // open file data and load its content
    string filename2("fileDetails.txt");
    ifstream input_file2(filename2);
    if (!input_file2.is_open())
    {
        cerr << "Could not open the file - '"
             << filename2 << "'" << endl;
        return false;
    }
    string key, fileName, fileSize, groupId, noOfPieces, pieceWiseHashPath;
    vector<string> seeders;
    string data1;
    i = 0;
    while (getline(input_file2, data1))
    {
        vector<string> splittedString = splitString(data1);
        if (i == 0)
            key = splittedString[0];
        else if (i == 1)
            fileName = splittedString[0];
        else if (i == 2)
            fileSize = splittedString[0];
        else if (i == 3)
        {
            groupId = splittedString[0];
        }
        else if (i == 4)
        {
            noOfPieces = splittedString[0];
        }
        else if (i == 5)
        {
            pieceWiseHashPath = splittedString[0];
        }
        else if (i == 6)
        {
            // cout << data1 << endl;
            if (data != "###")
                seeders = splittedString;
            else
                seeders.clear();
        }
        i++;
        if (i == 7)
        {
            files[key] = file(fileName, fileSize, groupId, noOfPieces, pieceWiseHashPath, seeders);
            i = 0;
        }
    }
    return true;
}

void makeUserDetailsPersistant(string userName, string password)
{
    ofstream outFile;
    outFile.open("validationData.txt", ios::out | ios::app);
    outFile << "\n";
    outFile << userName;
    outFile << " ";
    outFile << password;
    return;
}

void writeGroupDataToFile()
{
    // cout << "in group file" << endl;
    string filename("groupData.txt");
    ofstream outFile;
    outFile.open("groupData.txt", ios::out);
    for (auto each : groups)
    {
        string groupName = each.first;
        outFile << groupName;
        outFile << "\n";
        group groupDetails = each.second;
        string groupOwner = groupDetails.groupOwner;
        outFile << groupOwner;
        outFile << "\n";
        for (auto memb : groupDetails.groupMembers)
        {
            outFile << memb;
            outFile << " ";
        }
        outFile << "\n";
        if (groupDetails.sharedFiles.size() != 0)
        {
            for (auto sharFile : groupDetails.sharedFiles)
            {
                outFile << sharFile;
                outFile << " ";
            }
            outFile << "\n";
        }
        else
        {
            // # means no data
            outFile << "###";
            outFile << "\n";
        }
        if (groupDetails.pendingRequests.size() != 0)
        {
            for (auto pendReq : groupDetails.pendingRequests)
            {
                outFile << pendReq;
                outFile << " ";
            }
            outFile << "\n";
        }
        else
        {
            // # means no data
            outFile << "###";
            outFile << "\n";
        }
    }
}

void writeFileDataToFile()
{
    string filename("fileDetails.txt");
    ofstream outFile;
    outFile.open("fileDetails.txt", ios::out);
    for (auto each : files)
    {
        string key = each.first;
        outFile << key;
        outFile << "\n";
        file fileDetails = each.second;
        string fileName = fileDetails.fileName;
        outFile << fileName;
        outFile << "\n";
        string fileSize = fileDetails.fileSize;
        outFile << fileSize;
        outFile << "\n";
        string groupId = fileDetails.groupId;
        outFile << groupId;
        outFile << "\n";
        string noOfPieces = to_string(fileDetails.noOfPieces);
        outFile << noOfPieces;
        outFile << "\n";
        string pieceWiseHashPath = fileDetails.pieceWiseHashPath;
        outFile << pieceWiseHashPath;
        outFile << "\n";
        if (fileDetails.seeders.size() != 0)
        {
            for (auto seeder : fileDetails.seeders)
            {
                outFile << seeder;
                outFile << " ";
            }
            outFile << "\n";
        }
        else
        {
            // # means no data
            outFile << "###";
            outFile << "\n";
        }
    }
}

void replyMsg(int socketFd, string replyFromTracker)
{
    if (send(socketFd, replyFromTracker.c_str(), replyFromTracker.size(), 0) < 0)
    {
        cout << "unable to send reply to client" << endl;
        return;
    }
}

void *closeServer(void *arg)
{
    // continuisly listening at the input to close the server when quit is entered
    while (true)
    {
        string input;
        cin >> input;
        if (input == "quit")
        {
            writeGroupDataToFile();
            writeFileDataToFile();
            exit(0);
        }
    }
}

void create_user(vector<string> inputData, int socketFd, int clientPort)
{
    string userName = inputData[1];
    string password = inputData[2];
    string replyFromTracker = "";
    if (loginValidation.find(userName) == loginValidation.end())
    {
        loginValidation[userName] = userValidationDetails(userName, password, false, clientPort);
        // add userDetails in a file
        makeUserDetailsPersistant(userName, password);
        // loginValidation[userName] = obj;
        replyFromTracker = "user created successfully";
    }
    else
    {
        replyFromTracker = "user details are already present";
    }

    replyMsg(socketFd, replyFromTracker);
}

bool login(vector<string> inputData, int socketFd, int clientPort)
{
    string userName = inputData[1];
    string password = inputData[2];
    string peerServerIpAddress = inputData[3];
    string peerServerPort = inputData[4];
    string replyFromTracker = "";
    if (loginValidation.find(userName) == loginValidation.end())
    {
        replyFromTracker = "user login failed";
        replyMsg(socketFd, replyFromTracker);
        return false;
    }
    else if (loginValidation[userName].password != password)
    {
        replyFromTracker = "invalid credentials";
        replyMsg(socketFd, replyFromTracker);
        return false;
    }
    // else if (loginValidation[userName].isAlive == true and loginValidation[userName].peerServerIpAddress == peerServerIpAddress and loginValidation[userName].port == stoi(peerServerPort))
    // {

    //     replyFromTracker = "user already logged in";
    //     replyMsg(socketFd, replyFromTracker);
    //     return true;
    // }
    else
    {
        loginValidation[userName].updateStatus(true, clientPort, peerServerIpAddress, peerServerPort);
        replyFromTracker = "user login successfull";
        replyMsg(socketFd, replyFromTracker);
        return true;
    }
    return true;
}

void create_group(vector<string> inputData, int socketFd, string userId)
{
    string groupId = inputData[1];
    string replyFromTracker = "";
    cout << "in create_group" << endl;
    // to check whether the group name is already present
    for (auto gr : groups)
    {
        string groupName = gr.first;
        if (groupName == groupId)
        {
            replyFromTracker = "groupId already exists";
            replyMsg(socketFd, replyFromTracker);
            return;
        }
    }
    groups[groupId] = group(groupId, userId);
    replyFromTracker = "group created successfully";
    replyMsg(socketFd, replyFromTracker);
    return;
}

void join_group(vector<string> inputData, int socketFd, string userId)
{
    string groupId = inputData[1];
    string replyFromTracker = "";
    if (groups.find(groupId) == groups.end())
    {
        replyFromTracker = "groupId doesn't exists";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    for (auto member : groups[groupId].groupMembers)
    {
        if (member == userId)
        {
            replyFromTracker = "member already present in group";
            replyMsg(socketFd, replyFromTracker);
            return;
        }
    }
    groups[groupId].pendingRequests.push_back(userId);
    replyFromTracker = "join request successfull";
    replyMsg(socketFd, replyFromTracker);
    return;
}

void leave_group(vector<string> inputData, int socketFd, string userId)
{
    cout << "leave group" << endl;
    string groupId = inputData[1];
    string replyFromTracker = "";
    if (groups.find(groupId) == groups.end())
    {
        replyFromTracker = "groupId doesn't exists";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    auto it = find(groups[groupId].groupMembers.begin(), groups[groupId].groupMembers.end(), userId);
    if (it == groups[groupId].groupMembers.end())
    {
        replyFromTracker = "not a member of the group";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    else
    {
        groups[groupId].groupMembers.erase(it);
        if (groups[groupId].groupOwner == userId)
        {
            if (groups[groupId].groupMembers.size() != 0)
            {
                groups[groupId].groupOwner = groups[groupId].groupMembers[0];
            }
            else
            {
                // zero members in the group
                groups.erase(groupId);
            }
        }
        replyFromTracker = "removed from the group";
        replyMsg(socketFd, replyFromTracker);
    }
}

void list_requests(vector<string> inputData, int socketFd, string userId)
{
    string groupId = inputData[1];
    string replyFromTracker = "";
    if (groups.find(groupId) == groups.end())
    {
        replyFromTracker = "groupId doesn't exists";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    if (groups[groupId].groupOwner != userId)
    {
        replyFromTracker = "not a owner of the group";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    // no pending requests
    if (groups[groupId].pendingRequests.size() == 0)
    {
        replyFromTracker = "no pending requests";
        replyMsg(socketFd, replyFromTracker);
    }
    for (auto each : groups[groupId].pendingRequests)
    {
        // cout << each << endl;
        replyFromTracker = replyFromTracker + each + " ";
    }

    // cout << "in list requests" << replyFromTracker << endl;
    replyMsg(socketFd, replyFromTracker);
    return;
}

void accept_request(vector<string> inputData, int socketFd, string userId)
{
    string groupId = inputData[1];
    string acceptUserId = inputData[2];
    string replyFromTracker = "";
    if (groups.find(groupId) == groups.end())
    {
        replyFromTracker = "groupId doesn't exists";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    if (groups[groupId].groupOwner != userId)
    {
        replyFromTracker = "not a owner of the group";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    auto it = find(groups[groupId].pendingRequests.begin(), groups[groupId].pendingRequests.end(), acceptUserId);
    if (it == groups[groupId].pendingRequests.end())
    {
        replyFromTracker = "userId is not present in pending request";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    else
    {
        groups[groupId].pendingRequests.erase(it);
        groups[groupId].groupMembers.push_back(acceptUserId);
        replyFromTracker = "user added into group";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    return;
}

void list_groups(int socketFd)
{
    string replyFromTracker = "";
    for (auto group : groups)
    {
        string groupName = group.first;
        replyFromTracker = replyFromTracker + groupName + " ";
    }
    if (groups.size() == 0)
    {
        replyFromTracker = "no groups";
    }
    cout << "list_groups " << replyFromTracker << endl;
    replyMsg(socketFd, replyFromTracker);
    return;
}

void list_files(vector<string> inputData, int socketFd, string userId)
{
    // cout << "inside list files" << endl;
    string groupId = inputData[1];
    string replyFromTracker = "";
    if (groups.find(groupId) == groups.end())
    {
        replyFromTracker = "groupId doesn't exists";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    auto it = find(groups[groupId].groupMembers.begin(), groups[groupId].groupMembers.end(), userId);
    if (it == groups[groupId].groupMembers.end())
    {
        replyFromTracker = "not a member of group";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    if (groups[groupId].sharedFiles.size() == 0)
    {
        replyFromTracker = "no files";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    for (auto eachFile : groups[groupId].sharedFiles)
    {
        replyFromTracker = replyFromTracker + eachFile + " ";
    }
    replyMsg(socketFd, replyFromTracker);
    return;
}

void upload_file(vector<string> inputData, int socketFd, string userId)
{
    string fileName = inputData[1];
    string fileSize = inputData[2];
    string groupId = inputData[3];
    string pieceWiseHashPath = inputData[4];
    string replyFromTracker = "";
    cout << "in uploadFile function" << endl;
    if (groups.find(groupId) == groups.end())
    {
        replyFromTracker = "groupId doesn't exists";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    auto it = find(groups[groupId].groupMembers.begin(), groups[groupId].groupMembers.end(), userId);
    if (it == groups[groupId].groupMembers.end())
    {
        replyFromTracker = "not a member of the group to upload the file";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    // key for a file is groupId+fileName to make sure that duplicate files will be avoided
    string key = groupId + "$" + fileName;
    if (files.find(key) == files.end())
    {
        files[key] = file(fileName, fileSize, groupId, userId, pieceWiseHashPath);
        groups[groupId].sharedFiles.push_back(fileName);
        replyFromTracker = "File uploaded successfully";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    else
    {
        groups[groupId].sharedFiles.push_back(fileName);
        files[key].seeders.push_back(userId);
        replyFromTracker = "File is already in group and user is a seeder now";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    return;
}

void download_file(vector<string> inputData, int socketFd, string userId)
{
    // cout<<"inside download file"<<endl;
    string groupId = inputData[1];
    string fileName = inputData[2];
    string destinationPath = inputData[3];
    string replyFromTracker = "";
    if (groups.find(groupId) == groups.end())
    {
        replyFromTracker = "groupId doesn't exists";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    auto it = find(groups[groupId].groupMembers.begin(), groups[groupId].groupMembers.end(), userId);
    if (it == groups[groupId].groupMembers.end())
    {
        replyFromTracker = "not a member of the group to download the file";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    // key for a file is groupId+fileName to make sure that duplicate files will be avoided
    string key = groupId + "$" + fileName;
    if (files.find(key) == files.end())
    {
        replyFromTracker = "File is not present in the group";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    else
    {
        // cout << "hello" << endl;
        file reqFile = files[key];
        string fileName = reqFile.fileName;
        string groupId = reqFile.groupId;
        string fileSize = reqFile.fileSize;
        int noOfPieces = reqFile.noOfPieces;
        string pieceWiseHashPath = reqFile.pieceWiseHashPath;
        // find seeders ip and port
        string peerDetails = "";
        // cout << reqFile.seeders.size() << endl;
        int count = 0;
        for (auto seeder : reqFile.seeders)
        {
            if (loginValidation.find(seeder) == loginValidation.end())
            {
                continue;
            }
            else
            {
                if (loginValidation[seeder].isAlive == true)
                {
                    string ipAddress = loginValidation[seeder].peerServerIpAddress;
                    string port = loginValidation[seeder].peerServerPort;
                    peerDetails = peerDetails + ipAddress + ":" + port + " ";
                }
            }
        }
        // cout << peerDetails << endl;
        replyFromTracker = "fileDetails " + fileName + " " + groupId + " " + fileSize + " " + pieceWiseHashPath + " " + peerDetails;
        files[key].seeders.push_back(userId);
        cout << replyFromTracker << endl;
        replyMsg(socketFd, replyFromTracker);
    }
}

void logout(int socketFd, string userId)
{
    string replyFromTracker = "";
    if (loginValidation.find(userId) == loginValidation.end())
    {
        replyFromTracker = "not a valid user";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    loginValidation[userId].isAlive = false;
    loginValidation[userId].peerServerIpAddress = "";
    loginValidation[userId].peerServerPort = "";
    replyFromTracker = "logout successfull";
    replyMsg(socketFd, replyFromTracker);
    return;
}

void stop_share(vector<string> inputData, int socketFd, string userId)
{
    string fileName = inputData[2];
    string groupName = inputData[1];
    string key = groupName + "$" + fileName;
    string replyFromTracker = "";
    auto it = find(files[key].seeders.begin(), files[key].seeders.end(), userId);
    if (it == files[key].seeders.end())
    {
        replyFromTracker = "user is not a seeder to remove";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    else
    {
        files[key].seeders.erase(it);
        replyFromTracker = "user is not a seeder of this file now";
        replyMsg(socketFd, replyFromTracker);
        return;
    }
    return;
}

void clientConnection(int socketFd, int clientPort)
{
    string userId;
    while (true)
    {
        // cout << "inside while loop" << endl;
        char buffer[BUFFER_SIZE];
        bzero(buffer, BUFFER_SIZE);
        int n = recv(socketFd, buffer, BUFFER_SIZE, 0);

        if (n <= 0)
        {
            // no data recieved
        }
        // spliting the data
        else
        {
            printf("%s\n", buffer);
            string data(buffer);
            vector<string> inputData = splitString(buffer);
            if (inputData.size() == 0)
            {
                continue;
            }
            if (inputData[0] == "create_user")
            {
                if (inputData.size() != 3)
                {
                    string replyFromTracker = "invalid arguments";
                    if (send(socketFd, replyFromTracker.c_str(), replyFromTracker.size(), 0) < 0)
                    {
                        cout << "unable to send reply to client" << endl;
                        continue;
                    }
                }
                create_user(inputData, socketFd, clientPort);
            }
            else if (inputData[0] == "login")
            {
                if (inputData.size() != 5)
                {
                    string replyFromTracker = "invalid arguments";
                    if (send(socketFd, replyFromTracker.c_str(), replyFromTracker.size(), 0) < 0)
                    {
                        cout << "unable to send reply to client" << endl;
                        continue;
                    }
                }
                if (login(inputData, socketFd, clientPort))
                {
                    userId = inputData[1];
                }
            }
            else if (inputData[0] == "create_group")
            {
                if (inputData.size() != 2)
                {
                    string replyFromTracker = "invalid arguments";
                    replyMsg(socketFd, replyFromTracker);
                    continue;
                }
                create_group(inputData, socketFd, userId);
            }
            else if (inputData[0] == "join_group")
            {
                if (inputData.size() != 2)
                {
                    string replyFromTracker = "invalid arguments";
                    replyMsg(socketFd, replyFromTracker);
                    continue;
                }
                join_group(inputData, socketFd, userId);
            }
            else if (inputData[0] == "leave_group")
            {
                if (inputData.size() != 2)
                {
                    string replyFromTracker = "invalid arguments";
                    replyMsg(socketFd, replyFromTracker);
                    continue;
                }
                leave_group(inputData, socketFd, userId);
            }
            else if (inputData[0] == "list_requests")
            {
                if (inputData.size() != 2)
                {
                    string replyFromTracker = "invalid arguments";
                    replyMsg(socketFd, replyFromTracker);
                    continue;
                }
                list_requests(inputData, socketFd, userId);
            }
            else if (inputData[0] == "accept_request")
            {
                if (inputData.size() != 3)
                {
                    string replyFromTracker = "invalid arguments";
                    replyMsg(socketFd, replyFromTracker);
                    continue;
                }
                accept_request(inputData, socketFd, userId);
            }
            else if (inputData[0] == "list_groups")
            {
                if (inputData.size() != 1)
                {
                    string replyFromTracker = "invalid arguments";
                    replyMsg(socketFd, replyFromTracker);
                    continue;
                }
                list_groups(socketFd);
            }
            else if (inputData[0] == "list_files")
            {
                if (inputData.size() != 2)
                {
                    string replyFromTracker = "invalid arguments";
                    replyMsg(socketFd, replyFromTracker);
                    continue;
                }
                list_files(inputData, socketFd, userId);
            }
            else if (inputData[0] == "upload_file")
            {
                cout << "in uploadFile at tracker" << endl;
                if (inputData.size() != 5)
                {
                    string replyFromTracker = "invalid arguments";
                    replyMsg(socketFd, replyFromTracker);
                    continue;
                }
                upload_file(inputData, socketFd, userId);
            }
            else if (inputData[0] == "download_file")
            {
                if (inputData.size() != 4)
                {
                    string replyFromTracker = "invalid arguments";
                    replyMsg(socketFd, replyFromTracker);
                    continue;
                }
                download_file(inputData, socketFd, userId);
            }
            else if (inputData[0] == "logout")
            {
                if (inputData.size() != 1)
                {
                    string replyFromTracker = "invalid arguments";
                    replyMsg(socketFd, replyFromTracker);
                    continue;
                }
                logout(socketFd, userId);
            }
            else if (inputData[0] == "stop_share")
            {
                if (inputData.size() != 3)
                {
                    string replyFromTracker = "invalid arguments";
                    replyMsg(socketFd, replyFromTracker);
                    continue;
                }
                stop_share(inputData, socketFd, userId);
            }
            else
            {
                string replyFromTracker = "not a valid command";
                replyMsg(socketFd, replyFromTracker);
                continue;
            }
        }
    }
}

void trackerServerThread()
{
    cout << "tracker server thread" << endl;
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    // creating a vector of threads
    vector<thread> peerConnectionThreads;

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    // address.sin_addr.s_addr = trackerIpAddress;
    uint16_t PORT = stoi(trackerPort);
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    // listening for a peer to connect
    while ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                (socklen_t *)&addrlen)))
    {
        int clientPort = (int)ntohs(address.sin_port);

        // adding all the connections
        peerConnectionThreads.push_back(thread(clientConnection, new_socket, clientPort));
    }
    if (new_socket < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    for (auto it = peerConnectionThreads.begin(); it != peerConnectionThreads.end(); it++)
    {
        if (it->joinable())
        {
            it->join();
        }
    }
    shutdown(server_fd, SHUT_RDWR);
}

void getTrackerDetails(char const *argv[])
{
    // cout << "hello" << endl;
    string tNo(argv[2]);
    string trackerDetails(argv[1]);
    trackerNo = stoi(tNo);
    ifstream input_file(trackerDetails);
    if (!input_file.is_open())
    {
        cerr << "Could not open tracker details file" << endl;
        return;
    }
    string ip, port;
    int i = 1;
    while (input_file >> ip and input_file >> port)
    {
        if (i == trackerNo)
        {
            trackerIpAddress = ip;
            trackerPort = port;
            break;
        }
        i++;
        // cout << tracker1.first << " " << tracker1.second << endl;
        // cout << userName << " " << password << endl;
    }
    cout << trackerIpAddress << " " << trackerPort << endl;
}

int main(int argc, char const *argv[])
{
    // loadData();
    // assign ip and port address to tracker
    getTrackerDetails(argv);
    // close the server if quit is entered
    // loadData
    loadData();
    pthread_t closeThread;
    // creating a thread in server to check for closing server
    if (pthread_create(&closeThread, NULL, &closeServer, NULL) < 0)
    {
        perror("could not create thread");
        return 1;
    }

    // Server thread which listens to peers to get connected
    thread t1(trackerServerThread);
    t1.join();
}