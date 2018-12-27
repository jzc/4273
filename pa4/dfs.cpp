#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include <iostream>
#include <unordered_map>
#include <sstream>
#include <vector>
#include <fstream>
#include <thread>


#include "helper.h"


void handle_client(int client);
void create_logins();

std::unordered_map<std::string, std::string> logins;
std::string root;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <root dir> <port>" << std::endl;
        return 0;
    }

    root = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in servaddr;

    // Setup server address
    memset(&servaddr, 0, sizeof servaddr);
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(port);

    create_logins();
    mkdir(root.c_str(), 0777);

    auto servsocket = Socket(AF_INET, SOCK_STREAM, 0);
    auto option = 1;
    setsockopt(servsocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    Bind(servsocket, (struct sockaddr *) &servaddr, sizeof servaddr);
    Listen(servsocket, 10);

    for (;;) {
        auto clisocket = Accept(servsocket, NULL, 0);

        std::cout << "Connection accepted" << std::endl;
        std::thread t(handle_client, clisocket);
        t.detach();
    }
}

void create_logins() {
    std::ifstream infile("dfs.conf");
    std::string line;
    if (infile.is_open()) {
        while (getline(infile, line)) {
            std::stringstream ss(line);
            std::string user, pass;
            getline(ss, user, ' ');
            getline(ss, pass);
            std::cout << user << std::endl;
            std::cout << pass << std::endl;
            logins[user] = pass;
        }
    }
    infile.close();
}

void handle_client(int client) {
    char buf[BUFSIZE];
    recv(client, buf, BUFSIZE, 0);

    std::stringstream ss(buf);

    std::string  command;
    getline(ss, command, ' ');

    if (command == "login") {
        std::string user, pass;
        getline(ss, user, ' '); getline(ss, pass, ' ');

        auto query = logins.find(user);
        std::string response;
        if (query != logins.end() && query->second == pass) {
            response = "1";
        } else {
            response = "0";
        }
        std::cout << response << std::endl;
        send(client, response.c_str(), response.length(), 0);
    } else if (command == "get") {
        std::string filename, username;
        getline(ss, filename, ' '); getline(ss, username, ' ');
        
        std::vector<std::string> filelist;
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir((root + "/" + username + "/").c_str())) != NULL) {
            while((ent = readdir(dir)) != NULL) {
                filelist.push_back(ent->d_name);
            }
            closedir(dir);

            std::vector<std::string> partlist;
            for (auto f : filelist) { 
                if (f.substr(1, filename.size()) == filename) {
                    partlist.push_back(f);
                }
            }

            if (partlist.size() == 0) { 
                std::cout << filename << " not found" << std::endl;
            } else {
                // part 1
                std::ifstream part1stream(root + "/" + username + "/" + partlist[0], std::ios::binary | std::ios::ate);
                auto part1size = part1stream.tellg();
                part1stream.seekg(0, std::ios::beg);
                std::vector<char> part1data(part1size);
                part1stream.read(part1data.data(), part1size);
                part1stream.close();
                auto part1no = partlist[0][partlist[0].size() - 1];

                // part 2
                std::ifstream part2stream(root + "/" + username + "/" + partlist[1], std::ios::binary | std::ios::ate);
                auto part2size = part2stream.tellg();
                part2stream.seekg(0, std::ios::beg);
                std::vector<char> part2data(part2size);
                part2stream.read(part2data.data(), part2size);
                part2stream.close();
                auto part2no = partlist[1][partlist[1].size() - 1];
                
                std::stringstream res_ss;
                res_ss << part1no << " " << part2no << " " << part1size << " " << part2size << " " << "\r\n\r\n";
                auto response = res_ss.str();

                send(client, response.data(), response.size(), 0);
                send(client, part1data.data(), part1data.size(), 0);
                send(client, part2data.data(), part2data.size(), 0);
            }
        } 
    } else if (command == "put") {
        std::string filename, username, part1no, part2no, size1, size2;
        getline(ss, filename, ' '); getline(ss, username, ' ');
        getline(ss, part1no, ' '); getline(ss, part2no, ' ');
        getline(ss, size1, ' '); getline(ss, size2, ' ');
        int isize1 = stoi(size1), isize2 = stoi(size2);

        std::vector<char> part1data, part2data;
        int header_len = ss.tellg();
        if (header_len + isize1 + isize2 < BUFSIZE) {
            // All of message was received in first recv
            auto x = buf + header_len + 4;
            auto y = x + isize1;
            auto z = y + isize2;
            part1data.insert(part1data.end(), x, y);
            part2data.insert(part2data.end(), y, z);
            
        } else {
            // Message was not fully received, get rest of message
            int m = (header_len + 4 + isize1 + isize2) / BUFSIZE + 1;
            std::vector<char> temp(BUFSIZE*m), part1temp;
            recv(client, temp.data(), BUFSIZE*m, 0);
            part1temp.insert(part1temp.end(), buf+header_len+4, buf+BUFSIZE);
            part1temp.insert(part1temp.end(), temp.begin(), temp.end());

            auto x = part1temp.begin();
            auto y = x + isize1;
            auto z = y + isize2;
            part1data.insert(part1data.end(), x, y);
            part2data.insert(part2data.end(), y, z);
        }

        auto diruser = root + "/" + username;
        mkdir(diruser.c_str(), 0777);
        std::ofstream part1stream(diruser + "/." + filename + "." + part1no);
        if (part1stream.is_open()) {
            part1stream.write(part1data.data(), part1data.size());
        }
        part1stream.close();

        std::ofstream part2stream(diruser + "/." + filename + "." + part2no);
        if (part2stream.is_open()) {
            part2stream.write(part2data.data(), part2data.size());
        }
        part2stream.close();

    } else if (command == "list") {
        std::string username;
        getline(ss, username, ' ');

        std::vector<std::string> files;
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir((root + "/" + username).c_str())) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                files.push_back(ent->d_name);
            }
            closedir(dir);

            if (files.size() != 0) {
                std::stringstream rss;
                for (auto f: files) {
                    if (f != ".." && f != ".") {
                        rss << f << " ";
                    }
                }

                std::string response = rss.str();
                send(client, response.c_str(), response.length()+1, 0);
            }
        }
    } else {

    }
    close(client);
    std::cout << "closed connection" << std::endl;
}
