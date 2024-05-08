#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>

// To read the contents of a file - page.html
std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main()
{
    int s_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (s_sock == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(s_sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Error binding socket");
        close(s_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(s_sock, 12) < 0) {
        perror("Error listening on socket");
        close(s_sock);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is up (8080): " << s_sock << std::endl;

    fd_set active_set, read_set;
    FD_ZERO(&active_set);
    FD_SET(s_sock, &active_set);

    std::string htmlContent = readFile("page.html");
    const char* httpResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n\r\n";

    while (true)
    {
        read_set = active_set;
        if (select(FD_SETSIZE, &read_set, NULL, NULL, NULL) < 0) {
            perror("Server error: SELECT");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < FD_SETSIZE; ++i)
        {
            if (FD_ISSET(i, &read_set))
            {
                if (i == s_sock)
                {
                    int c_sock = accept(s_sock, NULL, NULL);
                    if (c_sock != -1) {
                        FD_SET(c_sock, &active_set);
                    }
                }
                else
                {
                    send(i, httpResponse, strlen(httpResponse), 0);
                    send(i, htmlContent.c_str(), htmlContent.length(), 0);
                    close(i);
                    FD_CLR(i, &active_set);
                }
            }
        }
    }

    close(s_sock);
    return 0;
}