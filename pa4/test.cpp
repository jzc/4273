#include <string.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "helper.h"

#define NSERVER 4

std::string user;
std::string password;

int destinations[NSERVER][NSERVER][2] = {
    {{1, 2}, {2, 3}, {3, 4}, {4, 1}},
    {{4, 1}, {1, 2}, {2, 3}, {3, 4}},
    {{3, 4}, {4, 1}, {1, 2}, {2, 3}},
    {{2, 3}, {3, 4}, {4, 1}, {1, 2}},
};

struct sockaddr_in server_addrs[NSERVER];

int send_part(std::string filename, std::vector<char> part, int partno, struct sockaddr_in addr) {

    auto servsocket = Socket(AF_INET, SOCK_STREAM, 0);
    auto res = connect(servsocket, (struct sockaddr *) &addr, sizeof addr);
    if (res < 0) return 1;

    std::string response;
    response += user;
    response += " ";
    response += password;
    response += " put ";
    response += filename;
    response += " ";
    response += std::to_string(partno);
    response += " ";
    response += std::to_string(part.size());
    response += " ";
    response += "\r\n\r\n";
    
    std::cout << "Sending part " << partno << std::endl;
    std::cout << (char *)&part[0] << std::endl;
    send(servsocket, response.c_str(), response.length(), 0);
    send(servsocket, &part[0], part.size(), 0);

    char buf[BUFSIZE];
    recv(servsocket, buf, BUFSIZE, 0);

    close(servsocket);
    return std::string(buf) != INVALID_USER_PASS;
}

std::vector<char> recv_all(int servsocket) {
    int n;
    char buf[BUFSIZE];
    std::vector<char> response;
    for (;;) {
        memset(buf, 0, BUFSIZE); 
        n = recv(servsocket, buf, BUFSIZE, 0);
        if (n < 0) {
            if (errno == EWOULDBLOCK) {
                std::cout << "timeout hit" << std::endl;
                break;
            }
            perror("recv error");
            exit(EXIT_FAILURE);
        }
        if (n == 0) break;
        response.insert(response.end(), buf, buf+n);
    }
    return response;
}

std::vector<std::pair<int, std::vector<char>>> recv_part(std::string filename, int i) {
    auto servsocket = Socket(AF_INET, SOCK_STREAM, 0);
    auto res = connect(servsocket, (struct sockaddr *) &server_addrs[i], sizeof server_addrs[i]);
    if (res < 0) return std::vector<std::pair<int, std::vector<char>>> (); 

    std::string request;
    request += user;
    request += " ";
    request += password;
    request += " get ";
    request += filename;
    request += "\r\n\r\n";

    send(servsocket, request.data(), request.size(), 0);
    std::vector<char> original = recv_all(servsocket);
    std::string response(original.data());
    auto delim = response.find("\r\n\r\n");
    std::string header = response.substr(0, delim);
    std::vector<char> rest(original.begin()+delim+4, original.end());

    std::stringstream ss(header);
    std::string auth, filefound, partno1, partno2, size1, size2;
    getline(ss, auth, ' ');
    getline(ss, filefound, ' ');
    getline(ss, partno1, ' ');
    getline(ss, partno2, ' ');
    getline(ss, size1, ' ');
    getline(ss, size2, ' ');
    if (auth == "1" && filefound == "1") {
        auto mid = rest.begin()+stoi(size1);
        std::vector<std::pair<int, std::vector<char>>> res { 
            std::make_pair(stoi(partno1), std::vector<char>(rest.begin(), mid)),
            std::make_pair(stoi(partno2), std::vector<char>(mid, mid+stoi(size2))),
        };
        return res;
    } else {
        return std::vector<std::pair<int, std::vector<char>>> ();
    }
}



int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <conf>" << std::endl;
        return 0;
    }

    std::ifstream infile(argv[1]);
    if (!infile.is_open()) {
        std::cout << "Unable to read configuration file" << std::endl;
        return 0;
    }

    for (int i = 0; i < NSERVER; i++ ) { 
        std::string line, ipstr, port;
        getline(infile, line);
        std::stringstream ss(line);
        getline(ss, ipstr, ' ');
        getline(ss, port, ' ');
        int ip;
        inet_pton(AF_INET, ipstr.c_str(), &ip);

        memset(&server_addrs[i], 0, sizeof server_addrs[i]);
        server_addrs[i].sin_family = AF_INET;
        server_addrs[i].sin_addr.s_addr = ip; 
        server_addrs[i].sin_port = htons(stoi(port));
    }

    getline(infile, user);
    getline(infile, password);
    std::cout << "username pass" << std::endl;
    std::cout << user << std::endl;
    std::cout << password << std::endl;

    infile.close();

    for (;;) {
        std::cout << "> ";
        
        std::string line;
        if(!getline(std::cin, line)) {
            std::cout << std::endl;
            break;
        }

        std::stringstream ss(line);
        std::string command;
        getline(ss, command, ' ');

        if (command == "put") {
            std::string filename;
            getline(ss, filename, ' ');

            // calculate hash
            FILE *fp = popen(("md5sum " + filename).c_str(), "r");
            char buf[BUFSIZE];
            fgets(buf, BUFSIZE, fp);
            std::stringstream ss2(buf);
            std::string md5;
            getline(ss2, md5, ' ');
            std::cout << md5 << std::endl;
            int hash = strtol(&md5[md5.size()-1], NULL, 16) & 0b11;
            std::cout << hash << std::endl;

            // get file size
            std::ifstream infile(filename, std::ios::binary | std::ios::ate);
            auto size = infile.tellg();
            infile.seekg(0, std::ios::beg);

            // read file
            std::vector<char> filedata(size);
            infile.read(filedata.data(), size);
            infile.close();

            // split file
            int quarter = size/4;
            std::vector<char> part1(filedata.begin(), filedata.begin()+quarter);
            std::vector<char> part2(filedata.begin()+quarter, filedata.begin()+2*quarter);
            std::vector<char> part3(filedata.begin()+2*quarter, filedata.begin()+3*quarter);
            std::vector<char> part4(filedata.begin()+3*quarter, filedata.end());
            std::vector<std::vector<char>> parts { part1, part2, part3, part4 };

            std::cout << "chukn sizes" << std::endl;
            std::cout << part1.size() << std::endl;
            std::cout << part2.size() << std::endl;
            std::cout << part3.size() << std::endl;
            std::cout << part4.size() << std::endl;

            // send parts 
            int res = 1;
            for (int i = 0; i < NSERVER; i++) { 
                int partno1 = destinations[hash][i][0];
                int partno2 = destinations[hash][i][1];
                res &= send_part(filename, parts[partno1-1], partno1, server_addrs[i]);
                res &= send_part(filename, parts[partno2-1], partno2, server_addrs[i]);
            }
            if (!res) {
                std::cout << "Invalid Username/Password. Please try again" << std::endl;
            }
        } else if (command == "get") {
            std::string filename;
            getline(ss, filename, ' ');
            
            std::vector<std::pair<int, std::vector<char>>> parts;
            for (int i = 0; i < NSERVER; i++) {
                auto res = recv_part(filename, i);
                parts.insert(parts.end(), res.begin(), res.end());
            }

            std::vector<char> *arr[4];
            memset(arr, 0, sizeof arr);
            for (auto p : parts) {
                arr[p.first-1] = &p.second;
            }

            bool anynull = false;
            for (auto p : arr) {
                if (p == NULL) {
                    anynull = true;
                    break;
                }
            }
            if (!anynull) {
                std::ofstream outfile(filename);
                if (outfile.is_open()) {
                    for (auto p : arr) { 
                        outfile.write(p->data(), p->size());
                    }
                }
                outfile.close();
            } else {

            }
        }
    }
}

