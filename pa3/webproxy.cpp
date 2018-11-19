#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <string>
#include <iostream>
#include <regex>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <utility>
#include <vector>
#include <thread>
#include <unordered_set>
#include <fstream>

#define BUFSIZE 8192


void create_blacklist();
void handle_request(int client);
int dns_lookup(std::string host, struct addrinfo **servinfo);
std::vector<char> get_page(std::string url, std::string host);
std::vector<char> recv_response(int servsocket);
int Socket(int domain, int type, int protocol);
int Bind(int sockfd, struct sockaddr *my_addr, int addrlen);
int Listen(int sockfd, int backlog);
int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int Connect(int sockfd, struct sockaddr *serv_addr, int addrlen);


int port = 80;
int timeout = 60;
const std::regex expr(R"((\w+) https?:\/\/([\w\d\.]+)(\/\S*) )");
const std::regex ip_expr(R"(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})");
// Set of blacklisted IP addresses
std::unordered_set<int> blacklist;
// Mapping from page URL to page content with 
std::unordered_map<std::string, std::pair<time_t, std::vector<char>>> page_cache;
// Mapping from hostnames to addrinfo structs
std::unordered_map<std::string, addrinfo *> dns_cache;
// Locks for respective global structures
std::mutex page_lock;
std::mutex dns_lock;


int main(int argc, char *argv[]) {
    if (argc == 2) {
        port = atoi(argv[1]);
    } else if (argc == 3) {
        port = atoi(argv[1]);
        timeout = atoi(argv[2]);
    } else if (argc >= 4) {
        std::cout << "Usage: " << argv[0] << " <port> <timeout>" << std::endl;
        return 0;
    }

    int servsocket, clisocket;
    struct sockaddr_in servaddr;

    // Setup server address
    memset(&servaddr, 0, sizeof servaddr);
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(port);

    // Create blacklist
    create_blacklist();

    // Create socket, bind, and listen
    servsocket = Socket(AF_INET, SOCK_STREAM, 0);
    auto option = 1;
    setsockopt(servsocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    Bind(servsocket, (struct sockaddr *) &servaddr, sizeof servaddr);
    Listen(servsocket, 10);

    for (;;) {
        clisocket = Accept(servsocket, NULL, 0);
        std::thread t(handle_request, clisocket);
        t.detach();
    }
}

void create_blacklist() {
    std::ifstream infile("blacklist.txt");
    std::string line;
    if (infile.is_open()) {
        while (getline(infile, line)) {
            if (std::regex_match(line, ip_expr)) {
                // Line is an IP 
                int ip;
                inet_pton(AF_INET, line.c_str(), &ip);
                blacklist.insert(ip);
            } else {
                // Line is hostname 
                struct addrinfo *servinfo; 
                auto status = dns_lookup(line, &servinfo);
                if (status == 0) {
                    auto ip = ((struct sockaddr_in *) servinfo->ai_addr)->sin_addr.s_addr;
                    blacklist.insert(ip);
                }
            }
        }
    }
}

void handle_request(int client) {
    char recvbuf[BUFSIZE];
    recv(client, recvbuf, BUFSIZE, 0);

    // Parse request
    std::cmatch match_results;
    if (std::regex_search(recvbuf, match_results, expr)) {
        auto method = match_results.str(1);
        auto url = match_results.str(3);
        auto host = match_results.str(2);

        // Handle get method
        if (method == "GET") {
            auto response = get_page(url, host);
            send(client, &response[0], response.size(), 0);
        } 
    }
    close(client);
}

std::vector<char> get_page(std::string url, std::string host) {
    auto key = host + url;
    std::cout << "GET " << key << std::endl;

    page_lock.lock();
    auto res = page_cache.find(key);
    page_lock.unlock();

    if (res != page_cache.end()) {
        if ((time(nullptr) - res->second.first) < timeout) {
            std::cout << "Page cache hit" << std::endl;
            return res->second.second;
        } else {
            std::cout << "Page cache hit, but cached page is older than " 
            << timeout << " seconds" << std::endl;
        }
    } else {
        std::cout << "Page cache miss" << std::endl;
    }

    struct addrinfo *servinfo; 
    auto status = dns_lookup(host, &servinfo);

    std::vector<char> response;
    if (status == 0) { 
        // Valid host
        auto ip = ((sockaddr_in *)servinfo->ai_addr)->sin_addr.s_addr;
        if (blacklist.find(ip) != blacklist.end()) {
            // IP in blacklist
            std::string response_string = "HTTP/1.1 403 Forbidden\r\n"
                                          "Content-Type: text/plain\r\n"
                                          "\r\n"
                                          "403 Forbidden";
            response = std::vector<char>(response_string.begin(), response_string.end());        
        } else {
            // IP not in blacklist
            struct timeval timeout;
            timeout.tv_sec = 10;
            timeout.tv_usec = 0;

            auto servsocket = Socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
            Connect(servsocket, servinfo->ai_addr, servinfo->ai_addrlen);
            setsockopt(servsocket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof timeout);

            auto request = "GET " + url + " HTTP/1.1\r\nHost: " + host + "\r\nAccept: */*\r\n\r\n";
            send(servsocket, request.c_str(), request.length(), 0);
            response = recv_response(servsocket);
            close(servsocket);
        }
        // freeaddrinfo(servinfo); Don't use with caching
    } else {
        // Invalid host, 404 not found
        std::string response_string = "HTTP/1.1 404 Not Found\r\n"
                                      "Content-Type: text/plain\r\n"
                                      "\r\n"
                                      "404 Not Found";    
        response = std::vector<char>(response_string.begin(), response_string.end());
    }

    page_lock.lock();
    page_cache[key] = std::make_pair(time(nullptr), response);
    page_lock.unlock();

    return response;
}

// Perform a DNS lookup. Returns zero on success, negative on failure.
int dns_lookup(std::string host, struct addrinfo **servinfo) {
    if (dns_cache.find(host) == dns_cache.end()) {
        std::cout << "DNS cache miss" << std::endl;

        struct addrinfo hints;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        auto res = getaddrinfo(host.c_str(), "http", &hints, servinfo);

        // If hostname does not exist, set the cache entry to 
        // null. When you go to look up the host again, the null
        // will signal this function to return a negative number.
        if (res < 0) {
            *servinfo = NULL;
        }
        dns_lock.lock();
        dns_cache[host] = *servinfo;
        dns_lock.unlock();
        
        return res;
    } else {
        std::cout << "DNS cache hit" << std::endl;

        dns_lock.lock();
        *servinfo = dns_cache[host];
        dns_lock.unlock();

        return *servinfo == NULL ? -1 : 0;
    }
}

std::vector<char> recv_response(int servsocket) {
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

int Socket(int domain, int type, int protocol) {
    int ret;
    if ((ret = socket(domain, type, protocol)) < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Bind(int sockfd, struct sockaddr *my_addr, int addrlen) {
    int ret;
    if ((ret = bind(sockfd, my_addr, addrlen)) < 0 ) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Listen(int sockfd, int backlog) {
    int ret;
    if ((ret = listen(sockfd, backlog)) < 0) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int ret;
    if ((ret = accept(sockfd, addr, addrlen)) < 0) {
        perror("accept error");
        exit(EXIT_FAILURE);
    }
    return ret;
}

int Connect(int sockfd, struct sockaddr *serv_addr, int addrlen) {
    int ret;
    if ((ret = connect(sockfd, serv_addr, addrlen)) < 0) {
        perror("connect error");
        exit(EXIT_FAILURE);
    }
    return ret;
}