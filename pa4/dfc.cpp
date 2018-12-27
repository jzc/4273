#include <string.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>

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

bool logged_in = true;
struct sockaddr_in server_addrs[NSERVER];

std::vector<char> send_message(std::vector<char> message, int server_index);

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
    infile.close();

    std::stringstream login_ss;
    login_ss << "login" << " " << user << " " << password << " " << "\r\n\r\n";
    std::string login_message = login_ss.str();
    for (int i = 0; i < NSERVER; i++) {
        std::vector<char> request(login_message.begin(), login_message.end());
        auto response = send_message(request, i);
        if (response.size() != 0 && response[0] == '0') {
            logged_in = false;
        }
    }

    for (;;) {
        std::cout << "> ";
        std::string line;
        if(!getline(std::cin, line)) {
            std::cout << std::endl;
            break;
        }

        if (!logged_in) {
            std::cout << "Invalid Username/Password. please try again" << std::endl;
            continue;
        }

        std::stringstream ss(line);
        std::string command;
        getline(ss, command, ' ');

        if (command == "put") {
            std::string filename;
            getline(ss, filename, ' ');

            // get file size
            std::ifstream infile(filename, std::ios::binary | std::ios::ate);
            if (!infile.is_open()) {
                std::cout << "File not found" << std::endl;
                continue;
            }

            // calculate hash
            FILE *fp = popen(("md5sum " + filename).c_str(), "r");
            char buf[BUFSIZE];
            fgets(buf, BUFSIZE, fp);
            std::stringstream ss2(buf);
            std::string md5;
            getline(ss2, md5, ' ');
            int hash = strtol(&md5[md5.size()-1], NULL, 16) & 0b11;\
            pclose(fp);


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

            // send parts 
            for (int i = 0; i < NSERVER; i++) { 
                auto partno1 = destinations[hash][i][0];
                auto partno2 = destinations[hash][i][1];
                auto part1 = parts[partno1-1];
                auto part2 = parts[partno2-1];
                auto size1 = part1.size();
                auto size2 = part2.size();

                std::stringstream msg_ss;
                msg_ss << "put" << " " << filename << " " 
                       << user << " " << partno1 << " " << partno2 << " " 
                       << size1 << " " << size2 << " " << "\r\n\r\n";
                auto message_str = msg_ss.str();

                std::vector<char> message_vec;
                message_vec.insert(message_vec.end(), message_str.begin(), message_str.end());
                message_vec.insert(message_vec.end(), part1.begin(), part1.end());
                message_vec.insert(message_vec.end(), part2.begin(), part2.end());
                auto response = send_message(message_vec, i);
            }
        } else if (command == "get") {
            std::string file;
            getline(ss, file, ' ');

            std::vector<std::vector<char>> responses;
            for (int i = 0; i < NSERVER; i++) {
                std::stringstream msg_ss;
                msg_ss << "get" << " " << file << " " << user << " " << "\r\n\r\n";
                auto message_str = msg_ss.str();

                std::vector<char> message_vec(message_str.begin(), message_str.end());
                auto response = send_message(message_vec, i);
                if (response.size() != 0) {
                    responses.push_back(response);
                }
            }

            if (responses.size() <= 2) {
                std::cout << "File incomplete or not found" << std::endl;
                continue;
            }

            std::vector<char> parts[4];
            for (auto r : responses) {
                std::string response(r.data());
                int delim = response.find("\r\n\r\n");
                auto header = response.substr(0, delim);

                std::stringstream hss(header);
                std::string part1no, part2no, part1size, part2size;
                getline(hss, part1no, ' '); getline(hss, part2no, ' ');
                getline(hss, part1size, ' '); getline(hss, part2size, ' ');

                auto x = r.begin() + delim + 4;
                auto y = x + stoi(part1size);
                auto z = y + stoi(part2size);
                std::vector<char> part1data(x, y);
                std::vector<char> part2data(y, z);

                parts[stoi(part1no)-1] = part1data;
                parts[stoi(part2no)-1] = part2data;
            }

            std::ofstream outfile(file);
            if (outfile.is_open()) {
                for (auto p : parts) {
                    outfile.write(p.data(), p.size());
                }
            }
            outfile.close();
        } else if (command == "list") { 
            std::vector<std::vector<char>> responses;
            for (int i = 0; i < NSERVER; i++) { 
                std::stringstream msg_ss;
                msg_ss << "list" << " " << user << " " << "\r\n\r\n";
                auto message_str = msg_ss.str();
                std::vector<char> message_vec(message_str.begin(), message_str.end());
                auto response = send_message(message_vec, i);
                if (response.size() != 0) {
                    responses.push_back(response);
                }
            }

            std::vector<std::string> filenames;
            for (auto r : responses) {
                std::stringstream hss(r.data());
                std::string filename;
                while (getline(hss, filename, ' ')) {
                    filenames.push_back(filename);
                }
            }

            std::unordered_map<std::string, bool[4]> counts;
            for (auto f: filenames) { 
                std::string found = f.substr(1, f.length()-3);
                int partno = f[f.length()-1]-'0';
                counts[found][partno-1] = true;
            }

            for (auto p : counts) {
                bool complete = p.second[0] && p.second[1] && p.second[2] && p.second[3];
                if (complete) {
                    std::cout << p.first << " [complete]" << std::endl;
                } else {
                    std::cout << p.first << " [incomplete]" << std::endl;
                }
            }
        } else {

        }
    }
}

std::vector<char> recv_all(int socket) {
    char buf[BUFSIZE];
    std::vector<char> response;
    for (;;) {
        memset(buf, 0, BUFSIZE);
        int n = recv(socket, buf, BUFSIZE, 0);
        // std::cout << "buf: " << std::endl;
        if (n < 0) {
            perror("error in receiving response");
            exit(1);
        } 
        if (n == 0) {
            break;
        }
        response.insert(response.end(), buf, buf+n);
    }
    return response;
}

std::vector<char> send_message(std::vector<char> message, int server_index) { 
    auto servsocket = Socket(AF_INET, SOCK_STREAM, 0);
    auto res = connect(servsocket, (struct sockaddr *) &server_addrs[server_index], sizeof server_addrs[server_index]);
    if (res < 0) {
        // Server not up
        std::cout << "Server " << server_index+1 << " not up" << std::endl;
        close(servsocket);
        return std::vector<char>();
    } else {
        // Server is up
        // Send inital message to server
        // No extra bytes on the end!!!
        int n = send(servsocket, message.data(), message.size(), 0);
        if (n < (int) message.size()) {
            std::cout << "partial send" << std::endl;
            return std::vector<char>();
        } 

        // Receive whole response from server
        auto response = recv_all(servsocket);

        close(servsocket);
        return response;
    }
}
