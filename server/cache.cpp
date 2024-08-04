#include <deque>
#include <string>
#include <sys/socket.h>
#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fstream>
#include <stdio.h>
#include <fcntl.h>
using namespace std;
class cache {
    //don't cache dynamically created assets
    pthread_mutexattr_t attr;
public:
    long max_size;
    long curr_size;
    pthread_mutex_t mutex;
    string storedRequestsData;
    int shmid;
    long end_index;

    cache() {

    }
    cache(long size) {
        max_size = size;
        curr_size = 0;
        end_index = 0;
        storedRequestsData.resize(max_size);
        //initialize the mutex
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&mutex, &attr);
        // clear the storage
        for (int i = 0; i <= max_size; i++) {
            storedRequestsData [i] = '\0';
        }
    }
    void updateStorage() {
        std::string filename = "cache.txt";
        storedRequestsData.clear();
        storedRequestsData.resize(max_size);
        int fd = open(filename.data(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        FILE* cacheFile = fdopen(fd, "w+");
        // Check if the file is open
        if (cacheFile == NULL) {
            perror("Error opening file");
            return;
        }
        char ch;
        int index = 0;
        while ((ch = fgetc(cacheFile)) != EOF) {
            storedRequestsData [index++] = ch;
        }
        curr_size = index;
        end_index = index;
    }
    void updateFile() {
        std::string filename = "cache.txt";

        int fd = open(filename.data(), O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        FILE* cacheFile = fdopen(fd, "w+");
        // Check if the file is open
        if (cacheFile == NULL) {
            perror("Error opening file");
            return;
        }
        char ch;
        for (int i = 0; i < storedRequestsData.size(); i++) {
            fputc(storedRequestsData [i], cacheFile);
        }

    }
    //remove the front req/data pair and then 
    void makeSpace() {
        string buffer(storedRequestsData);
        int first = buffer.find('^');
        if (first == -1) {
            cout << "nothing in buffer";
            return;
        }
        int second = buffer.find('^', first + 1);
        //if there's only one '^'
        if (second == -1) {
            for (int i = 0; i < max_size; i++) {
                storedRequestsData [i] = '\0';
            }
            curr_size = 0;
            end_index = 0;
            //cout << "buffer after erase: " << string(storedRequestsData) << "\n";
            //cout << "end_index after erase: " << end_index << "\n";
            return;
        }
        buffer.erase(buffer.begin(), buffer.begin() + second + 1);
        curr_size -= second + 1; //not sure
        end_index -= second + 1;  // not sure
        for (int i = 0; i < buffer.size(); i++) {
            storedRequestsData [i] = buffer [i];
        }
        for (int i = buffer.size(); i <= max_size; i++) {
            storedRequestsData [i] = '\0';
        }
        //cout << "buffer after erase: " << string(storedRequestsData) << "\n";
        //cout << "end_index after erase: " << end_index << "\n";

    }
    void addToCache(string request, string data) {
        pthread_mutex_lock(&mutex);
        updateStorage();
        cout << "adding to cache\n";
        cout << "cache curr size: " << curr_size;
        //adding at front; don't need / at front
        if (end_index == 0) {
            //cout << "it was zero\n";
            if (request.size() + data.size() + 1 > max_size) {
                pthread_mutex_unlock(&mutex);
                return;
            }
            while (curr_size + request.size() + data.size() + 1 > max_size) {
                makeSpace();
            }
            curr_size += request.size() + 1 + data.size();
        }
        //need a slash at the front of the pairing
        else {
            if (request.size() + data.size() + 2 > max_size) {
                pthread_mutex_unlock(&mutex);
                return;
            }
            while (curr_size + request.size() + data.size() + 2 > max_size) {
                makeSpace();
            }
            storedRequestsData [end_index++] = '^';
            curr_size += request.size() + 2 + data.size();
        }
        for (int i = 0; i < request.size(); i++) {
            storedRequestsData [end_index++] = request [i];
        }
        storedRequestsData [end_index++] = '^';
        for (int i = 0; i < data.size(); i++) {
            storedRequestsData [end_index++] = data [i];
        }
        if (storedRequestsData [0] == '^') {
            for (int i = 0; i < max_size; i++) {
                storedRequestsData [i] = storedRequestsData [i + 1];
            }
            curr_size--;
            end_index--;
        }
        cout << "current state of the storage after adding: " << string(storedRequestsData) << "\n";
        cout << "end_index after adding: " << end_index << "\n";
        updateFile();
        pthread_mutex_unlock(&mutex);
        return;
    }
    string getData(string request) {
        updateStorage();
        //gets the data associated with a request
        //if there is no data that is associated if that request, returns ""
        pthread_mutex_lock(&mutex);
        string str(storedRequestsData);
        int indexLoc = str.find(request + "^");
        if (indexLoc == -1) {
            //cout << "got here\n";
            pthread_mutex_unlock(&mutex);
            return "";
        }
        string data;
        for (int i = indexLoc + request.size() + 1; i <= end_index; i++) {
            if (str [i] == '^' || str [i] == '\0') {
                break;
            }
            cout << str [i];
            data += str [i];
        }
        pthread_mutex_unlock(&mutex);
        return data;

    }
    bool quickSend(int clientFd, string request) {
        string data = getData(request);
        cout << "data: " << data;
        if (data == "") {
            cout << "datavalue: " << data << "\n";
            cout << "can't send cache\n";
            return false;
        }
        cout << "can send cached\n";
        send(clientFd, data.data(), data.size(), 0);
        cout << "used cached data!" << "\n";
        return true;
    }

};

/*int main() {
    cache Cache(6000);
    Cache.addToCache("help", "isgfdgfdjkgnflkgnjflgnjdfklgnfjklfnldgdghere");
    Cache.addToCache("I", "amhegfsilgkfjgnlngsjkgnsfklkfdnsgkdfjlgre");
    Cache.addToCache("Hello", "World");
    string data = Cache.getData("i");
    cout << "help data: " << data << "\n";

}*/
