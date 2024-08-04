#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <sys/stat.h>
#include <string>
#include <sstream>
#include <string.h>
#include <fstream>
#include<sys/wait.h>
#include <fcntl.h>
#include <iostream>
#include <stdlib.h>
#include <dirent.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#include "cache.cpp"

using namespace std;

/*
To do: cache
*/
//can't do it like this. the caching will be reset with each request cuz it forks
//need to use command line args
bool useCaching = false;
bool useThreading = false;
cache Cache;
string addr;

vector<string> split(const char* str, char c = ' ') {
    vector<string> result;
    do {
        const char* begin = str;

        while (*str != c && *str)
            str++;

        result.push_back(std::string(begin, str));

    } while (0 != *str++);

    return result;
}
vector<pair<string, string>> extractOptions(string req) {
    vector<pair<string, string>> res;
    int startingPoint = req.find("?");
    if (startingPoint == string::npos) {
        cout << "no options\n";
        return res;
    }
    int s = startingPoint + 1, length = 1;
    string temp;
    for (int i = s; i < req.size(); i++) {
        if (req [i] == '&' || i == req.size() - 1) {
            string option = req.substr(s, length);
            if (option.back() == '&') {
                option.pop_back();
            }
            vector<string> nameVal = split(option.data(), '=');
            res.push_back({ nameVal [0], nameVal [1] });
            cout << "name: " << nameVal [0] << "\nval: " << nameVal [1] << "\n";
            length = 1;
            s = i + 1;
        }
        else {
            length++;
        }
    }
    return res;
}
string extractFileName(string req) {
    int index = req.find("?");
    if (index == string::npos) {
        return req;
    }
    string res = req.substr(0, index);
    return res;
}
void sendNotImplemented(int client_fd, string req) {
    // send over the NOT imp html
    if (useCaching) {
        if (Cache.quickSend(client_fd, req)) {
            return;
        }
    }
    string buffer, temp;
    fstream htmlFile("./notImplemented.html", fstream::in | fstream::out);
    while (getline(htmlFile, temp)) {
        buffer += temp;
    }
    buffer = "HTTP/1.1 501 Not Implemented\r\nContent-Type: text/html\r\n\r\n" + buffer;
    if (useCaching) {
        Cache.addToCache(req, buffer);
    }
    send(client_fd, buffer.data(), buffer.size(), 0);
}

void sendNotFound(int client_fd, string req) {
    // send over the NOT FOUND file
    if (useCaching) {
        if (Cache.quickSend(client_fd, req)) {
            return;
        }
    }
    string buffer, temp;
    fstream htmlFile("./notFound.html", fstream::in | fstream::out);
    while (getline(htmlFile, temp)) {
        buffer += temp;
    }
    buffer = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n" + buffer;
    if (useCaching) {
        Cache.addToCache(req, buffer);
    }
    send(client_fd, buffer.data(), buffer.size(), 0);
}
void sendDir(string filename, int client_fd, string req) {
    if (useCaching) {
        if (Cache.quickSend(client_fd, req)) {
            cout << "quicksent\n";
            return;
        }
    }
    DIR* dir = opendir(filename.data());
    string buffer = "<html><body>";
    struct dirent* file;
    if (dir == NULL) {
        printf("error in start open");
        exit(1);
    }
    file = readdir(dir);
    while (file != NULL) {
        if (strcmp(file->d_name, ".") != 0 &&
            strcmp(file->d_name, "..") != 0) {
            buffer += file->d_name;
            buffer += "<br>";
        }
        file = readdir(dir);
    }
    buffer += "</body></html>";
    buffer = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + buffer;
    if (useCaching) {
        Cache.addToCache(req, buffer);
    }
    send(client_fd, buffer.data(), strlen(buffer.data()), 0);
}
void sendHtml(string filename, int client_fd, string req) {
    // send over the file
    if (useCaching && req != "NULL") {
        cout << "sendHTML req: " << req << "\n";
        if (Cache.quickSend(client_fd, req)) {
            return;
        }
    }
    string buffer, temp;
    fstream htmlFile("." + filename, fstream::in | fstream::out);
    while (getline(htmlFile, temp)) {
        buffer += temp;
    }
    buffer = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + buffer;
    if (useCaching && req != "NULL") {
        Cache.addToCache(req, buffer);
    }
    send(client_fd, buffer.data(), buffer.size(), 0);
}
void execCGI(string filepath, int client_fd, char* argvs []) {
    int output = open("./cgiOutput.html", O_RDWR | O_CREAT, 0666);
    cout << filepath.data() << "\n";
    if (fork() == 0) {
        dup2(output, STDOUT_FILENO);
        execv(filepath.data(), argvs);
        perror("could not execute file");
        exit(1);
    }
    wait(NULL);
    sendHtml("/cgiOutput.html", client_fd, "NULL");
    if (remove("./cgiOutput.html") == 0) {
        cout << "deleted successfully\n";
    }
    else {
        perror("error during deletion of temporary cgiOutput.html; try running again");
    }
}
void sendImage(string filename, int client_fd, string type, string req) {
    // send over the file
    string temp;
    fstream imageFile("." + filename, fstream::in | fstream::out | ios::binary);
    if (!imageFile.is_open()) {
        perror("Failed to open image file");
        sendNotFound(client_fd, req);
        return;
    }
    string buffer((istreambuf_iterator<char>(imageFile)), istreambuf_iterator<char>());
    buffer = "HTTP/1.1 200 OK\r\nContent-Type: image/" + type + "\r\n\r\n" + buffer;
    send(client_fd, buffer.data(), buffer.size(), 0);
}
std::string url_decode(const std::string& encoded) {
    int output_length;
    const auto decoded_value = curl_easy_unescape(nullptr, encoded.c_str(), static_cast<int>(encoded.length()), &output_length);
    std::string result(decoded_value, output_length);
    curl_free(decoded_value);
    return result;
}
string getYoutubeLink(vector<pair<string, string>>& options) {
    for (auto p : options) {
        if (p.first == "URL") {
            //convert the encoding
            cout << "found url: " << p.second << "\n";
            string res = url_decode(p.second);
            cout << "decoded url: " << res << "\n";
            return res;
        }
    }
    return "";
}
bool parseOptions(vector<pair<string, string>>& options) {
    for (auto p : options) {
        string name = p.first;
        if (name == "URL") {
            continue;
        }
        else if (name == "") {
            continue;
        }
        else if (name == "dir") {
            continue;
        }
        else {
            return false;
        }
    }
    return true;
}
void sendRequestedFile(vector<vector<string>>& requests, int client_fd) {
    //extract the filename from the request
    if (useCaching) {
        if (Cache.quickSend(client_fd, requests [0][1])) {
            cout << "cache used";
            return;
        }
    }
    string filename = extractFileName(requests [0][1]);
    cout << "req: " << requests [0][1] << "\n";
    string filepath = "." + filename;
    vector<pair<string, string>> options = extractOptions(requests [0][1]);
    if (!parseOptions(options)) {
        sendNotImplemented(client_fd, requests [0][1]);
        return;
    }
    if (filename == "/youtubeLink") {
        //parse the youtubeLink and then return
        string youtubeLink = getYoutubeLink(options);
        cout << "youtube link: " << youtubeLink << "\n";
        if (youtubeLink == "") {
            printf("url was not found in options\n");
        }
        else {
            char* argvs [] = { filename.data(), youtubeLink.data(), NULL };
            //might need to use a different function cuz of outputting of execCGI
            execCGI("./youtubeProcessing.py", client_fd, argvs);
            return;
        }
        return;
    }
    else if (filename == "/playVideo") {
        //execute the program that sends the subtitile to the arduino
        //so create a socket to connect to the arduino
        cout << "playvideo button was pressed\n";
        if (fork() == 0) {
            char* argv [] = { filename.data(), NULL };
            execv("./subtitle_display", argv);
            perror("exec of subtitlePlayer failed");
            exit(1);
        }
        return;
    }
    struct stat s;
    if (stat(filepath.data(), &s) == 0) {
        if (s.st_mode & S_IFDIR) {
            sendDir(filepath, client_fd, requests [0][1]);
        }
        else {
            // if html send html
            if (filename.find(".html") != string::npos) {
                sendHtml(filename, client_fd, requests [0][1]);
                return;
            }
            else if (filename.find(".gif") != string::npos) {
                sendImage(filename, client_fd, "gif", requests [0][1]);
                return;
            }
            else if (filename.find(".jpeg") != string::npos) {
                sendImage(filename, client_fd, "jpeg", requests [0][1]);
                return;
            }
            else if (filename.find(".jpg") != string::npos) {
                sendImage(filename, client_fd, "jpg", requests [0][1]);
                return;
            }
            else if (filename.find(".cgi") != string::npos) {
                char* argvs [] = { filepath.data(), NULL, NULL };
                if (filename.find("my-histogram.cgi") != string::npos) {
                    string cgiInput;
                    for (auto p : options) {
                        if (p.first == "dir") {
                            cgiInput = p.second;
                        }
                    }
                    if (cgiInput.size() == 0) {
                        printf("missing dir option from client\n");
                        return;
                    }
                    argvs [1] = cgiInput.data();
                }
                execCGI(filepath, client_fd, argvs);
                return;
            }
            else {
                sendHtml(filename, client_fd, requests [0][1]);
                return;
            }
        }
    }
    else {
        // Error occurred
        perror("Failed to stat");
        sendNotFound(client_fd, requests [0][1]);
    }
}
void servConn(int port) {
    // the address of the socket
    struct sockaddr_in sock_address, client_address;
    socklen_t client_len;
    // create a socket and store it's file descriptor
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);
    // if -1, then error so exit
    if (socketFD < 0) {
        printf("error in creating socket");
        exit(1);
    }
    // initialize the sock_address
    sock_address.sin_family = AF_INET;
    sock_address.sin_port = htons(port); // host to network conversion for the short integer
    sock_address.sin_addr.s_addr = inet_addr(addr.data());
    //my laptop at MCM: 172.30.85.89

    // for setsockopt
    int sock_opt_val = 1;
    // set socket options
    // SO_REUSEADDR and SO_REUSEPORT to be able to reuse the address and the port
    if (setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &sock_opt_val,
        sizeof(sock_opt_val)) == -1) {
        perror("(servConn): Failed to set SO_REUSEADDR on INET socket");
        exit(-1);
    }

    // bind socketFD to the IP address and port
    if (bind(socketFD, (struct sockaddr*) &sock_address, sizeof(sock_address)) == -1) {
        perror("failure in binding");
        exit(1);
    }

    // listen; only 5 incoming connections can queue at a time
    if (listen(socketFD, 5) == -1) {
        perror("failure in listening");
        exit(1);
    }

    // accepting a connection
    // client_length
    client_len = sizeof(client_address);
    // continuously accept connections
    while (1) {
        // accept a new connection
        int client_fd = accept(socketFD, (struct sockaddr*) &client_address, &client_len);
        if (client_fd == -1) {
            perror("failure in accepting");
            exit(1);
        }
        if (fork() == 0) {
            if (useCaching) {
                cout << "cache curr size: " << Cache.curr_size << "\n";
                /*if ((Cache.storedRequestsData = (char*) shmat(Cache.shmid, NULL, 0)) == (void*) -1) {
                    cerr << "Error in attaching shared memory!" << endl;
                    exit(1);
                }*/
            }
            //sharedmemory for cache:
            // strings for sending the file
            string buffer, temp;
            // string for storing the requests
            string req;
            vector<vector<string>> requests;
            req.resize(100000);
            // changing the client fd into a FILE object to use fgets
            FILE* client = fdopen(client_fd, "r");
            // while loop to receive the entire http request from client
            int linecount = 1;
            while (fgets(req.data(), 100000, client) != NULL) {

                requests.push_back(split(req.data()));
                linecount++;
                if (req.find("Accept") != string::npos) {
                    break;
                }
            }
            // sending file requested function here
            sendRequestedFile(requests, client_fd);
            // keep the connection open
            exit(0);
        }

    }

    // shut the socket down and close
    shutdown(socketFD, SHUT_RDWR);
    close(socketFD);
}
bool parseCLOptions(int argc, char const* argv []) {
    int option;
    long cacheSize = 0;
    while ((option = getopt(argc, (char* const*) argv, "c:tp:")) != -1) {
        cout << char(option) << "\n";
        if (char(option) == 'c') {
            useCaching = true;
            cout << "caching set\n";
            cacheSize = stol(optarg);
            if (cacheSize > 2000000 || cacheSize < 4000) {
                cerr << "invalid cache size\n";
                return false;
            }
            cache temp(cacheSize);
            Cache.mutex = temp.mutex;
            Cache.end_index = temp.end_index;
            Cache.shmid = temp.shmid;
            Cache.storedRequestsData = temp.storedRequestsData;
            Cache.max_size = temp.max_size;
            Cache.curr_size = temp.curr_size;
            cout << "cache size: " << cacheSize << "\n";
        }
        else if (char(option) == 't') {
            useThreading = true;
        }
        else if (char(option) == 'p') {
            addr = string(optarg);
        }
        else {

            cout << "unknown option used\nPlease try again\n";
            return false;
        }

    }
    return true;
}
int main(int argc, char const* argv []) {
    int port;
    //default address is loopback
    addr = "127.0.0.1";
    if (argv [1] == nullptr || atoi(argv [1]) == 0) {
        cerr << "you must specify the port number\n";
        exit(1);
    }
    else {
        port = atoi(argv [1]);
        printf("port: %d\n", port);
    }
    if (!parseCLOptions(argc, argv)) {
        exit(1);
    }
    servConn(port);
    return 0;
}
