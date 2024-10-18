#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <bits/stdc++.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <thread>
#include <openssl/sha.h>
#include <time.h>

using namespace std;

string peerServerIpAddress;
string peerServerPort;
pair<string, string> tracker1, tracker2;
#define PIECESIZE 524288
#define BUFFER_SIZE 65536

class peerConnectionDetails
{
public:
    string ipAddress;
    string port;
    peerConnectionDetails()
    {
    }
    peerConnectionDetails(string ip, string por)
    {
        ipAddress = ip;
        port = por;
    }
};

class fileDetails
{
public:
    string fileName;
    string filePath;
    string groupName;
    string pieceWiseHashPath;
    vector<bool> piecesAvailable;
    long long fileSize;
    int noOfPieces;
    fileDetails()
    {
    }
    fileDetails(string fName, string fPath, string gName, string fSize, string pieceWiseHashPath, bool available)
    {
        fileName = fName;
        filePath = fPath;
        groupName = gName;
        fileSize = stoll(fSize);
        noOfPieces = ceil((float)fileSize / PIECESIZE);
        piecesAvailable.resize(noOfPieces, available);
    }
    fileDetails(string fName, string fPath, string gName, string pAvailable, string fSize, string piecesLen)
    {
        fileName = fName;
        filePath = fPath;
        groupName = gName;
        fileSize = stoll(fSize);
        noOfPieces = ceil((float)fileSize / PIECESIZE);
        for (auto each : pAvailable)
        {
            if (each == '1')
                piecesAvailable.push_back(true);
            else
                piecesAvailable.push_back(false);
        }
    }
};

map<string, fileDetails> files;

map<pair<string, string>, bool> fileStatus;

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

void loadFileDetails(string userName)
{
    string folder = "userData";
    string path = folder + "/" + userName;
    ifstream input_file(path);
    if (!input_file.is_open())
    {
        cerr << "Could not open the file - '" << endl;
        return;
    }
    string fileName, filePath, groupName, piecesAvailable;
    string fileSize;
    string noOfPieces;
    string data, key;
    int i = 0;
    while (getline(input_file, data))
    {
        vector<string> splittedString = splitString(data);
        if (i == 0)
            key = splittedString[0];
        else if (i == 1)
            fileName = splittedString[0];
        else if (i == 2)
            filePath = splittedString[0];
        else if (i == 3)
            groupName = splittedString[0];
        else if (i == 4)
        {
            piecesAvailable = splittedString[0];
        }
        else if (i == 5)
            fileSize = splittedString[0];
        else if (i == 6)
            noOfPieces = splittedString[0];
        if (i == 6)
        {
            files[key] = fileDetails(fileName, filePath, groupName, piecesAvailable, fileSize, noOfPieces);
        }
        i++;
    }
}

void writeFileDetails(string userName)
{
    string folder = "userData";
    string path = folder + "/" + userName;
    ofstream outFile;
    outFile.open(path.c_str(), ios::out);
    // cout << "fileDetails" << endl;
    // cout << path << endl;
    // cout << files.size() << endl;
    for (auto each : files)
    {
        string key = each.first;
        // cout << key << endl;
        outFile << key;
        outFile << "\n";
        fileDetails file = each.second;
        outFile << file.fileName;
        outFile << "\n";
        outFile << file.filePath;
        outFile << "\n";
        outFile << file.groupName;
        outFile << "\n";
        string chunksAvailable;
        for (auto each : file.piecesAvailable)
        {
            if (each == true)
                chunksAvailable += "1";
            else
                chunksAvailable += "0";
        }
        outFile << chunksAvailable;
        outFile << "\n";
        outFile << to_string(file.fileSize);
        outFile << "\n";
        outFile << to_string(file.noOfPieces);
        outFile << "\n";
    }
    files.clear();
    fileStatus.clear();
    // string fileName, fileSize, groupName, noOfPieces, pieceWiseHashPath;
}

string getFileNameFromPath(string path)
{
    int n = path.size();
    string name = "";
    for (int i = n - 1; i > -1; i--)
    {
        if (path[i] == '/')
            break;
        else
            name = path[i] + name;
    }
    // cout << "fileName " << name << endl;
    return name;
}

string getFileSize(string filePath)
{
    ifstream in_file(filePath.c_str(), ios::binary);
    in_file.seekg(0, ios::end);
    long long int file_size = in_file.tellg();
    return to_string(file_size);
}

string makeSHA1raw(string data)
{
    // SHA_DIGEST_LENGTH is 160bits or 20 bytes which is standard for SHA1 hash
    unsigned char hash[SHA_DIGEST_LENGTH];
    // SHA1 returns a raw hash of size 160bits
    SHA1(reinterpret_cast<const unsigned char *>(data.c_str()), data.size(), hash);
    char sha1string[SHA_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
    {
        // converts into raw hash into readable hash by converting it into hexadecimal
        sprintf(&sha1string[i * 2], "%02x", (unsigned int)hash[i]);
    }
    // returns 40 digited hash
    string hashedString(sha1string);
    return hashedString;
}

string findPieceWiseHash(string filePath)
{
    string pieceWiseHash = "";
    int fd1 = open(filePath.c_str(), O_RDONLY);
    if (fd1 == -1)
    {
        cout << "error in opening file to calculate piece wise hash" << endl;
        return "#";
    }
    char buffer[PIECESIZE];
    int fileSize = 0;
    int n;
    while ((n = read(fd1, buffer, PIECESIZE)))
    {
        string data(buffer);
        // find hash of each 512kb and append it
        pieceWiseHash = pieceWiseHash + makeSHA1raw(data);
        bzero(buffer, PIECESIZE);
    }
    return pieceWiseHash;
}

string getFileHash(string filePath)
{

    ostringstream buffer;
    ifstream input(filePath.c_str());
    buffer << input.rdbuf();
    string contents = buffer.str();
    string hashedString;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    if (!SHA256(reinterpret_cast<const unsigned char *>(&contents[0]), contents.length(), hash))
    {
        printf("Error in hashing\n");
    }
    else
    {
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        {
            char buf[3];
            sprintf(buf, "%02x", hash[i] & 0xff);
            hashedString += string(buf);
        }
    }
    return hashedString;
}

int getNoOfChunks(string fSize)
{
    long long fileSize = stoll(fSize);
    int noOfPieces = ceil((float)fileSize / PIECESIZE);
    return noOfPieces;
}

string storeFileDetails(string pieceWiseHash, string fileHash, string fileName, string fileSize, string groupName)
{
    // store pieceWiseHash in every file in a folder
    string destFolder = "pieceWiseHash";
    string path = destFolder + "/" + groupName + "$" + fileName + ".txt";
    ofstream outFile;
    long long size = stoll(fileSize);
    int noOfPieces = ceil((float)size / PIECESIZE);
    string pieces = to_string(noOfPieces);
    cout << path << endl;
    outFile.open(path.c_str(), ios::out);
    outFile << fileName << "\n";
    outFile << groupName << "\n";
    outFile << fileSize << "\n";
    outFile << pieces << "\n";
    outFile << pieceWiseHash << "\n";
    outFile << fileHash << "\n";
    return path;
}

void upload_file(int sock, vector<string> inputData)
{
    string filePath = inputData[1];
    // gets fileName from Path
    string fileName = getFileNameFromPath(filePath);
    string groupName = inputData[2];
    string key = groupName + "$" + fileName;
    if (files.find(key) != files.end())
    {
        cout << "file is already uploaded" << endl;
        return;
    }
    cout << "uploadingFile...." << endl;
    // gets filesize in bytes
    string fileSize = getFileSize(inputData[1]);
    // gets fileHash
    string fileHash = getFileHash(filePath);
    // gets pieceWise hash
    string pieceWiseHash = findPieceWiseHash(filePath);
    if (pieceWiseHash == "#")
    {
        return;
    }
    string destFolder = "pieceWiseHash";
    string pieceWiseHashPath = destFolder + "/" + groupName + "$" + fileName + ".txt";
    string dataToSend = "";
    dataToSend = "upload_file";
    dataToSend = dataToSend + " " + fileName + " " + fileSize + " " + inputData[2] + " " + pieceWiseHashPath;
    int noOfBytes = dataToSend.size();
    if (send(sock, dataToSend.c_str(), noOfBytes, 0) < 0)
    {
        cout << "unable to send data" << endl;
        return;
    }
    char recvBuffer[BUFFER_SIZE];
    int n = recv(sock, recvBuffer, BUFFER_SIZE, 0);
    if (n > 0)
    {
        string reply(recvBuffer);
        cout << "Response from Tracker : ";
        printf("%s\n", recvBuffer);
        if (reply == "File uploaded successfully")
        {
            string key = groupName + "$" + fileName;
            // cout << "infle" << endl;
            storeFileDetails(pieceWiseHash, fileHash, fileName, fileSize, groupName);
            files[key] = fileDetails(fileName, filePath, groupName, fileSize, pieceWiseHashPath, true);
            // cout << files.size() << endl;
        }
        if (reply == "File is already in group and user is a seeder now")
        {
            string key = groupName + "$" + fileName;
            // storeFileDetails(pieceWiseHash,fileHash, fileName, fileSize, groupName);
            files[key] = fileDetails(fileName, filePath, groupName, fileSize, pieceWiseHashPath, true);
        }
    }
    // cout << userLogin << endl;
    bzero(recvBuffer, BUFFER_SIZE);
    return;
}

string readPieceInfo(int sockFd, string dataToSend)
{
    int noOfBytes = dataToSend.size();
    if (send(sockFd, dataToSend.c_str(), noOfBytes, 0) < 0)
    {
        cout << "unable to send data" << endl;
        return "$";
    }
    char recvBuffer[PIECESIZE];
    int n = read(sockFd, recvBuffer, PIECESIZE);
    if (n > 0)
    {
        string reply(recvBuffer);
        bzero(recvBuffer, PIECESIZE);
        return reply;
    }
    bzero(recvBuffer, PIECESIZE);
    return "$";
}

vector<int> corrupted;

string readPieceData(int sockFd, string dataToSend, int destFilePointer, int chunkNo, string fSize, string pieceWiseHash)
{
    int noOfBytes = dataToSend.size();
    if (send(sockFd, dataToSend.c_str(), noOfBytes, 0) < 0)
    {
        cout << "unable to send data" << endl;
        return "$";
    }
    long long offset = chunkNo * PIECESIZE;
    long long max_offset = (chunkNo + 1) * PIECESIZE;
    int i = 0;
    long long fileSize = stoll(fSize);
    string dataRecvd = "";
    while (offset < max_offset and offset < fileSize)
    {
        char recvBuffer[BUFFER_SIZE];
        bzero(recvBuffer, BUFFER_SIZE);
        int n = read(sockFd, recvBuffer, BUFFER_SIZE);

        // if (n <= 0)
        //     break;
        int r = pwrite(destFilePointer, recvBuffer, n, offset);
        offset += n;
        recvBuffer[n] = '\0';
        string s(recvBuffer);
        dataRecvd += s;
        if (r < 0)
        {
            cout << "unable to write data in the destination file" << endl;
            return "$";
        }
    }

    string r = makeSHA1raw(dataRecvd);
    // cout << r << endl;
    // if (pieceWiseHash != r)
    // {
    //     corrupted.push_back(chunkNo);
    // }
    string s = "successfull write of chunkNo " + to_string(chunkNo);
    // cout << "still in loop " << chunkNo << endl;

    return s;
}

string connectToPeer(string command, string dataToSend, string peerServerIpAddress, string peerServerPort, int fp, int chunkNo, string fileSize, string pieceWiseHash)
{
    // cout << "connection to peer" << endl;
    int sock = 0, valread, client_fd;
    struct sockaddr_in serv_addr;
    // char *s = "$";
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return "^";
    }
    serv_addr.sin_family = AF_INET;
    uint16_t peerPort = stoi(peerServerPort);
    serv_addr.sin_port = htons(peerPort);
    // Convert IPv4 and IPv6 addresses from text to binary
    // form
    string reply = "";
    if (inet_pton(AF_INET, peerServerIpAddress.c_str(), &serv_addr.sin_addr) <= 0)
    {
        printf(
            "\nInvalid address/ Address not supported \n");
        reply = "%";
        close(sock);
        return reply;
    }
    // cout << "connection will establish" << endl;
    if ((client_fd = connect(sock, (struct sockaddr *)&serv_addr,
                             sizeof(serv_addr))) < 0)
    {

        printf("\nConnection Failed \n");
        reply = "%";
        close(sock);
        return reply;
    }
    // cout << "connection established" << endl;
    if (command == "sendPieceInfo")
    {
        reply = readPieceInfo(sock, dataToSend);
        close(sock);
        return reply;
    }
    else if (command == "sendPieceData")
    {
        reply = readPieceData(sock, dataToSend, fp, chunkNo, fileSize, pieceWiseHash);
        close(sock);
        return reply;
    }
    reply = "%";
    close(sock);
    return reply;
}

string getPieceInfo(peerConnectionDetails peer, string fileName, string groupName, int noOfChunks)
{
    string ipAddress = peer.ipAddress;
    string port = peer.port;
    string dataToSend = "sendPieceInfo";
    dataToSend += " " + fileName + " " + groupName;
    char recvBuffer[BUFFER_SIZE];
    // cout << "getPieceInfo" << endl;
    string dataFromPeer = connectToPeer("sendPieceInfo", dataToSend, ipAddress, port, -1, -1, "$", "$");
    // cout << "dataFromPeer :" << port << " " << dataFromPeer << endl;
    if (dataFromPeer == "$" or dataFromPeer == "%")
    {
        string chunkInfo = "";
        for (int i = 0; i < noOfChunks; i++)
            chunkInfo += "0";
        dataFromPeer = "chunksAvailable " + chunkInfo;
    }
    vector<string> splitData = splitString(dataFromPeer);
    return splitData[1];
}

void getPieceData(peerConnectionDetails peer, string chunkNo, int filePointer, string fileName, string groupName, string fileSize, string pieceWiseHash)
{
    string ipAddress = peer.ipAddress;
    string port = peer.port;
    string dataToSend = "sendPieceData";
    dataToSend += " " + fileName + " " + groupName + " " + chunkNo;
    char recvBuffer[BUFFER_SIZE];
    string dataFromPeer = connectToPeer("sendPieceData", dataToSend, ipAddress, port, filePointer, stoi(chunkNo), fileSize, pieceWiseHash);
    if (dataFromPeer == "$")
    {
        cout << "data doesn't received properly" << endl;
        return;
    }
    if (dataFromPeer == "unable to send data")
    {
        cout << "data doesn't received properly" << endl;
        return;
    }
    string key = groupName + "$" + fileName;
    int n = stoi(chunkNo);
    files[key].piecesAvailable[n] = true;
    // cout << "chunk no " << chunkNo << " downloaded successfully" << endl;
    return;
}

void pieceWiseAlgo(vector<peerConnectionDetails> peerInfo, vector<string> piecesInfo, string fileName, string groupName, vector<vector<peerConnectionDetails>> chunksAvailableWithPeers, string downloadFilePath, string fileSize, vector<string> piecWiseHash)
{
    srand(time(0));
    int noOfChunks = piecesInfo[0].size();
    vector<thread> getData;
    int c = 0;
    string path = downloadFilePath;
    // cout << "downloading path " << path << endl;
    int filePointer = open(downloadFilePath.c_str(), O_WRONLY | O_CREAT, 0777);
    if (filePointer < 0)
    {
        cout << "error in opening file" << endl;
        return;
    }
    string key = groupName + "$" + fileName;
    fileStatus[{groupName, fileName}] = true;
    for (int i = 0; i < noOfChunks; i++)
    {
        int chunkNo = i;
        int noOfPeers = chunksAvailableWithPeers[i].size();
        // cout<<noOfPeers<<" ";
        int PeerNumber = rand() % noOfPeers;
        // cout << PeerNumber << " ";
        // cerr << fileName << " "
        //      << "peerNumber"
        //      << " " << PeerNumber << " "
        //      << " chunk no " << i << endl;
        string hash = piecWiseHash[i];
        usleep(10000);
        getData.push_back(thread(getPieceData, chunksAvailableWithPeers[i][PeerNumber], to_string(chunkNo), filePointer, fileName, groupName, fileSize, hash));
    }

    // cout << "hello " << c++ << endl;
    int threadsJoined = 0;
    for (auto it = getData.begin(); it != getData.end(); it++)
    {
        if (it->joinable())
        {
            threadsJoined++;
            // cout << threadsJoined << endl;
            it->join();
        }
    }
    // cout << threadsJoined << endl;
    // if (corrupted.size() == 0)
    // {
    //     cout << "file is not corrupted" << endl;
    // }
    // else
    // {
    // for (auto each : corrupted)
    //     cout << each << " ";
    // cout << endl;
    // }
    // cout << "Download completed" << endl;
    return;
}

vector<string> getPeiceWiseHash(string filePath)
{
    vector<string> pieceWiseHash;
    ifstream input_file(filePath.c_str());
    if (!input_file.is_open())
    {
        cerr << "Could not open the file - '"
             << "'" << endl;
        return pieceWiseHash;
    }
    string data;
    string hash;
    string fileHash;
    int i = 0;
    while (getline(input_file, data))
    {
        vector<string> splittedString = splitString(data);
        // cout << data << endl;
        if (i == 4)
        {
            hash = data;
            i++;
        }
        else if (i == 5)
        {
            fileHash = data;
            break;
        }
        else
        {
            i++;
            continue;
        }
    }
    // cout << hash << endl;
    // cout << fileHash << endl;
    for (int offset = 0; offset < hash.size(); offset = offset + 40)
    {
        string t = hash.substr(offset, 40);
        // cout << t << endl;
        pieceWiseHash.push_back(t);
    }
    pieceWiseHash.push_back(fileHash);
    return pieceWiseHash;
}

void startDownload(int sock, string reply, string downloadFilePath)
{
    vector<string> metaData = splitString(reply);
    // cout << reply << endl;
    int n = metaData.size();
    string fileName = metaData[1];
    string groupId = metaData[2];
    string fileSize = metaData[3];
    string pieceWiseHashPath = metaData[4];
    vector<string> pieceWiseHash = getPeiceWiseHash(pieceWiseHashPath);
    int noOfChunks = getNoOfChunks(fileSize);
    vector<peerConnectionDetails> peerInfo;
    for (int i = 5; i < n; i++)
    {
        string socketDetails = metaData[i];
        string ip = "";
        string port = "";
        bool flag = false;
        for (auto each : socketDetails)
        {
            if (each == ':')
            {
                flag = true;
                continue;
            }
            if (flag == false)
                ip += each;
            else
                port += each;
        }
        peerInfo.push_back(peerConnectionDetails(ip, port));
    }
    vector<string> piecesInfo;
    vector<bool> chunksRecieved(noOfChunks, false);

    string key = groupId + "$" + fileName;
    files[key] = fileDetails(fileName, downloadFilePath, groupId, fileSize, pieceWiseHashPath, false);
    // get what are the chunks present in a peer
    vector<vector<peerConnectionDetails>> chunksAvailableWithPeers(noOfChunks);
    for (auto peer : peerInfo)
    {
        // cout << peer.ipAddress << " " << peer.port << endl;
        string chunkInfo = getPieceInfo(peer, fileName, groupId, noOfChunks);
        // cout << chunkInfo << endl;
        for (int i = 0; i < noOfChunks; i++)
        {
            if (chunkInfo[i] == '1')
            {
                chunksAvailableWithPeers[i].push_back(peer);
                chunksRecieved[i] = true;
            }
        }
        piecesInfo.push_back(chunkInfo);
    }
    for (int i = 0; i < noOfChunks; i++)
    {
        if (chunksRecieved[i] == false)
        {
            // cout << i << endl;
            cout << "All the chunks cannot be recieved" << endl;
            cout << "Download cannot be done now" << endl;
            return;
        }
    }

    pieceWiseAlgo(peerInfo, piecesInfo, fileName, groupId, chunksAvailableWithPeers, downloadFilePath, fileSize, pieceWiseHash);
    fileStatus[{groupId, fileName}] = false;
}

void download_file(int sock, vector<string> inputData)
{
    string groupId = inputData[1];
    string fileName = inputData[2];
    string destinationPathFolder = inputData[3];
    string dataToSend = inputData[0] + " " + groupId + " " + fileName + " " + destinationPathFolder;
    int noOfBytes = dataToSend.size();
    string destinationPath = destinationPathFolder + "/" + fileName;
    // string key=groupId+"$"+fileName;
    if (send(sock, dataToSend.c_str(), noOfBytes, 0) < 0)
    {
        cout << "unable to send data" << endl;
        return;
    }
    char recvBuffer[BUFFER_SIZE];
    int n = recv(sock, recvBuffer, BUFFER_SIZE, 0);
    if (n > 0)
    {
        string reply(recvBuffer);
        if (reply == "not a member of the group to download the file" or reply == "File is not present in the group" or reply == "groupId doesn't exists")
        {
            cout << "Response from Tracker : " << reply << endl;
            return;
        }
        cout << "Response from Tracker : File meta Data sent" << endl;
        cout << "-------------File Download started----------" << endl;
        thread t(startDownload, sock, reply, destinationPath);
        t.detach();
        // startDownload(sock, reply, destinationPath);
    }
    // cout << userLogin << endl;
    bzero(recvBuffer, BUFFER_SIZE);
}

bool logout(int sock, vector<string> inputData)
{
    string dataToSend = inputData[0];
    int noOfBytes = dataToSend.size();
    if (send(sock, dataToSend.c_str(), noOfBytes, 0) < 0)
    {
        cout << "unable to send data" << endl;
        return false;
    }
    char recvBuffer[BUFFER_SIZE];
    int n = recv(sock, recvBuffer, BUFFER_SIZE, 0);
    if (n > 0)
    {
        string reply(recvBuffer);
        cout << "Response from Tracker : " << reply << endl;
        bzero(recvBuffer, BUFFER_SIZE);
        if (reply == "logout successfull")
        {
            return true;
        }
    }
    return false;
}

bool login(int sock, bool userLogin, vector<string> inputData)
{
    string dataToSend = inputData[0] + " " + inputData[1] + " " + inputData[2] + " " + peerServerIpAddress + " " + peerServerPort;
    int noOfBytes = dataToSend.size();
    if (send(sock, dataToSend.c_str(), noOfBytes, 0) < 0)
    {
        cout << "unable to send data" << endl;
        return false;
    }
    char recvBuffer[BUFFER_SIZE];
    int n = recv(sock, recvBuffer, BUFFER_SIZE, 0);
    if (n > 0)
    {
        string reply(recvBuffer);
        cout << "Response from Tracker : " << reply << endl;
        bzero(recvBuffer, BUFFER_SIZE);
        if (reply == "user login successfull" or reply == "user already logged in")
        {
            loadFileDetails(inputData[1]);
            return true;
        }
    }
    return false;
}

void show_downloads()
{
    for (auto each : fileStatus)
    {
        if (each.second == true)
        {
            cout << "              [D] [ " << each.first.first << " ]" << each.first.second << endl;
        }
        else
            cout << "              [C] [ " << each.first.first << " ]" << each.first.second << endl;
    }
}

void create_user(int sock, vector<string> inputData)
{
    string dataToSend = inputData[0] + " " + inputData[1] + " " + inputData[2];
    int noOfBytes = dataToSend.size();
    if (send(sock, dataToSend.c_str(), noOfBytes, 0) < 0)
    {
        cout << "unable to send data" << endl;
        return;
    }
    char recvBuffer[BUFFER_SIZE];
    int n = recv(sock, recvBuffer, BUFFER_SIZE, 0);
    if (n > 0)
    {
        string reply(recvBuffer);
        cout << "Response from Tracker : " << reply << endl;
        bzero(recvBuffer, BUFFER_SIZE);
        if (reply == "user created successfully")
        {
            fstream my_file;
            string folder = "userData";
            string path = folder + "/" + inputData[1];
            my_file.open(path.c_str(), ios::out);
            if (!my_file)
            {
                cout << "File not created!";
                return;
            }
            else
            {
                // cout << "File created successfully!";
                // my_file << "Guru99";
                my_file.close();
            }
        }
    }
}

void commands(int sock)
{
    // cout << "hello" << endl;
    bool userLogin = false;
    string userName = "";
    while (true)
    {
        // cout << userLogin << endl;
        string dataToSend;
        string fileName;
        string fileSize;
        string filePath;
        // reads a line of data
        cout << "command $";
        getline(cin, dataToSend);
        // splits string to find the commands
        vector<string> inputData = splitString(dataToSend);
        int noOfBytes = dataToSend.size();
        string command = inputData[0];
        // If the user is not logged in and try to run other commands then he is not permitted to do so
        if (command == "create_user")
        {
            if (userLogin == true)
            {
                cout << "Reply from Tracker : Logout to create another user" << endl;
                continue;
            }
            else
            {
                create_user(sock, inputData);
                continue;
            }
        }
        if (command != "create_user" and command != "login")
        {
            if (userLogin == false)
            {
                cout << "User is not logged in" << endl;
                continue;
            }
        }
        if (command == "login")
        {
            if (userLogin == true)
            {
                cout << "Reply from tracker : user already logged in" << endl;
                userLogin = true;
                continue;
            }
            // send peer server Ip and port address also to the tracker
            if (login(sock, userLogin, inputData))
            {
                userName = inputData[1];
                userLogin = true;
            }
            continue;
        }
        if (command == "upload_file" and inputData.size() == 3)
        {
            upload_file(sock, inputData);
            continue;
            // cout << dataToSend << endl;
        }
        if (command == "download_file" and inputData.size() == 4)
        {
            download_file(sock, inputData);
            // cout << "hello" << endl;
            continue;
        }
        if (command == "show_downloads")
        {
            show_downloads();
            continue;
        }
        if (command == "logout" and inputData.size() == 1)
        {
            if (logout(sock, inputData))
            {
                writeFileDetails(userName);
                userLogin = false;
            }
            continue;
        }
        noOfBytes = dataToSend.size();
        if (send(sock, dataToSend.c_str(), noOfBytes, 0) < 0)
        {
            cout << "unable to send data" << endl;
            continue;
        }
        char recvBuffer[BUFFER_SIZE];
        int n = recv(sock, recvBuffer, BUFFER_SIZE, 0);
        if (n > 0)
        {
            string reply(recvBuffer);
            cout << "Response from Tracker : ";
            printf("%s\n", recvBuffer);
            if (inputData[0] == "login")
            {
                if (reply == "user login successfull" or reply == "user already logged in")
                    userLogin = true;
            }
        }
        bzero(recvBuffer, BUFFER_SIZE);
    }
}

int trackerConnectionThread()
{
    // cout << "inside thread in client" << endl;
    int sock = 0, valread, client_fd;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return 0;
    }
    // cout << sock << " socketfd" << endl;
    serv_addr.sin_family = AF_INET;
    uint16_t PORT = stoi(tracker1.second);
    serv_addr.sin_port = htons(PORT);
    // Convert IPv4 and IPv6 addresses from text to binary
    // form
    if (inet_pton(AF_INET, tracker1.first.c_str(), &serv_addr.sin_addr) <= 0)
    {
        printf(
            "\nInvalid address/ Address not supported \n");
        return 0;
    }
    if ((client_fd = connect(sock, (struct sockaddr *)&serv_addr,
                             sizeof(serv_addr))) < 0)
    {
        printf("\nConnection Failed \n");
        return 0;
    }
    cout << "###################################" << endl;
    cout << endl;
    cout << "connection established with tracker" << endl;
    cout << endl;
    cout << "####################################" << endl;

    commands(sock);

    return 0;
}

void replyMsg(int socketFd, string replyFromTracker)
{
    if (send(socketFd, replyFromTracker.c_str(), replyFromTracker.size(), 0) < 0)
    {
        cout << "unable to send reply to client" << endl;
        return;
    }
}

void handlePeerRequest(int sockFd)
{
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    int n = recv(sockFd, buffer, BUFFER_SIZE, 0);
    // cerr << buffer << endl;
    if (n <= 0)
    {
        return;
    }
    string dataReceived(buffer);
    vector<string> inputData = splitString(dataReceived);
    string responseFromPeerServer = "";
    if (inputData[0] == "sendPieceInfo")
    {
        string fileName = inputData[1];
        string groupName = inputData[2];
        string key = groupName + "$" + fileName;
        if (files.find(key) == files.end())
        {
            responseFromPeerServer = "chunksAvailable $";

            // cout << "in handle peerconnection" << endl;
            cout << responseFromPeerServer << endl;
            replyMsg(sockFd, responseFromPeerServer);
            close(sockFd);
        }
        else
        {
            responseFromPeerServer = "chunksAvailable ";
            for (int i = 0; i < files[key].noOfPieces; i++)
            {
                if (files[key].piecesAvailable[i] == true)
                    responseFromPeerServer += '1';
                else
                    responseFromPeerServer += '0';
            }
            replyMsg(sockFd, responseFromPeerServer);
            close(sockFd);
        }
    }
    if (inputData[0] == "sendPieceData")
    {
        // cout << "inside sendPieceData " << endl;
        string fileName = inputData[1];
        string groupName = inputData[2];
        int chunkNo = stoi(inputData[3]);
        int fd = open(fileName.c_str(), O_RDONLY);
        if (fd == -1)
        {
            responseFromPeerServer = "unable to open file and send data";
            replyMsg(sockFd, responseFromPeerServer);
            return;
        }
        int offset = chunkNo * PIECESIZE;
        int max_offset = (chunkNo + 1) * PIECESIZE;
        int i = 0;
        while (offset < max_offset)
        {
            char data[BUFFER_SIZE];
            bzero(data, BUFFER_SIZE);
            int n = pread(fd, data, BUFFER_SIZE, offset);

            if (n < 0)
            {
                responseFromPeerServer = "unable to read data";
                replyMsg(sockFd, responseFromPeerServer);
                return;
            }
            offset += n;
            int r = send(sockFd, data, n, 0);
            usleep(10000);
            if (r < 0)
            {
                cout << "unable to send reply to client" << endl;
                return;
            }
            bzero(data, BUFFER_SIZE);
            if (n < BUFFER_SIZE)
                break;
        }
        // cout << "chunkNO " << chunkNo << " sent successfully" << endl;
        close(sockFd);
        close(fd);
    }
    bzero(buffer, BUFFER_SIZE);
}

int peerServerThread()
{
    // cerr << "in server thread" << endl;
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
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
    uint16_t PORT = stoi(peerServerPort);
    address.sin_port = htons(PORT);
    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10000) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    // cerr << "waiting for client" << endl;
    vector<thread> peerTopeerConnectionThreads;
    while ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                (socklen_t *)&addrlen)))
    {
        // printf("IP address is: %s\n", inet_ntoa(address.sin_addr));
        // printf("port is: %d\n", (int)ntohs(address.sin_port));
        peerTopeerConnectionThreads.push_back(thread(handlePeerRequest, new_socket));
        // cerr << "handler assigned" << endl;
    }
    if (new_socket < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    for (auto it = peerTopeerConnectionThreads.begin(); it != peerTopeerConnectionThreads.end(); it++)
    {
        if (it->joinable())
        {
            it->join();
        }
    }
    shutdown(server_fd, SHUT_RDWR);
    return 0;
}

void getTrackerandPeerDetails(char const *argv[])
{
    // extract ip-address and port of tracker from tracker_info.txt and peer's server ip address and port
    // contains peer-server's socket details
    string peerServerDetails(argv[1]);
    // tracker_info.txt
    string trackerDetails(argv[2]);
    bool flag = false;
    for (auto each : peerServerDetails)
    {
        if (each == ':')
        {
            flag = true;
            continue;
        }
        if (flag == false)
            peerServerIpAddress += each;
        else
            peerServerPort += each;
    }
    // cout << peerServerIpAddress << " " << peerServerPort << endl;
    ifstream input_file(trackerDetails);
    if (!input_file.is_open())
    {
        cerr << "Could not open tracker details file" << endl;
        return;
    }
    string ip, port;
    while (input_file >> ip and input_file >> port)
    {
        tracker1.first = ip;
        tracker1.second = port;
        break;

        // cout << userName << " " << password << endl;
    }
    // cout << tracker1.first << " " << tracker1.second << endl;
}

int main(int argc, char const *argv[])
{
    // freopen("log3.txt", "w", stderr);
    // cout<<"======================"<<endl;
    getTrackerandPeerDetails(argv);
    // creating a thread in server to connect with server
    thread t2(peerServerThread);
    // t2.join();
    trackerConnectionThread();

    // closing the connected socket
    // close(client_fd);
    return 0;
}