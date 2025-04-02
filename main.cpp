#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <fstream>
#include <string>
std::string getIpAddress(const char *domain)
{
    struct addrinfo hints, *res;
    int err;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4 address
    hints.ai_socktype = SOCK_STREAM;

    // Get the address information
    err = getaddrinfo(domain, NULL, &hints, &res);
    if (err != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(err));
        return nullptr;
    }

    struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));

    // Free the address information
    freeaddrinfo(res);

    return ip;
}

class Data
{
public:
    Data(const std::string &str)
    {
        std::istringstream stream(str);
        std::string word;
        int i = 0;
        while (stream >> word)
        {

            switch (i)
            {
            case 0:
                mountPoint = word;
                break;
            case 1:
                shareName = word;
                break;
            case 2:
                if (word != "-")
                    ipLocal = word;
                break;
            case 3:
                if (word != "-")
                    ipWeb = word;
                break;
            case 4:
                if (word != "-")
                    domain = word;
                break;
            case 5:
                creds = word;
                break;
            default:
                options.push_back(word);
            }
            i++;
        }
        if (!(domain.empty()) && (ipWeb.empty()))
        {
            ipWeb = getIpAddress(domain.c_str());
        }
    }

public:
    std::string mountPoint;
    std::string shareName;
    std::string ipLocal;
    std::string ipWeb;
    std::string domain;
    std::string creds;
    std::vector<std::string> options;
};


bool isDeviceReachable(const std::string &ipAddr)
{
    std::string command = "ping -c 1 -W 1 " + ipAddr;
    int result = system(command.c_str());
    return result == 0;
}

    bool mountSmbShare(const std::string &ipServer, const std::string &shareName, const std::string &mntPoint, const std::string &creds){

    // TODO redo credentials
    // TODO change system()
    std::ostringstream command;
    command << "sudo mount -t cifs //"
            << ipServer << "/" << shareName << " "
            << mntPoint << " "
            << "-o credentials=" << creds;

    int result = system(command.str().c_str());
    if(result != 0){
        std::cerr << "Failed to  mount share. Error code: "  << result  <<  std::endl;
        return false;
    }
    return result == 0;
}
void printData(const Data &data)
{
    std::cout << "Mount Point: " << data.mountPoint << std::endl;
    std::cout << "Share Name: " << data.shareName << std::endl;
    std::cout << "IP Local: " << data.ipLocal << std::endl;
    std::cout << "IP Web: " << data.ipWeb << std::endl;
    std::cout << "Domain: " << data.domain << std::endl;
    std::cout << "Credentials: " << data.creds << std::endl;
    std::cout << "Options: ";

    if (!data.options.empty())
    {
        for (const auto &option : data.options)
        {
            std::cout << option << " ";
        }
    }
    else
    {
        std::cout << "No options available.";
    }

    std::cout << std::endl;
}
int main(int argc, char *argv[])
{

    for (int i = 0; i < argc; i++)
    {
        std::cout << i << argv[i] << "\n";
    }
    std::ifstream file(argv[1]);

    if (!file)
    {
        std::cerr << "Could not open file." << std::endl;
        return 1;
    }
    std::string line;
    std::vector<Data> input;

    while (std::getline(file, line))
    {
        input.push_back(Data(line));
    }

    file.close();

    printData(input[0]);
    for (Data data : input)
    {
        std::cout << "Trying the host on the local network...\n";
        if (isDeviceReachable(data.ipLocal))
        {
            std::cout << "\nConnecting on local network...\n";
            if (mountSmbShare(data.ipLocal, data.shareName, data.mountPoint, data.creds))
            {
                std::cout << "Successfully connected to " << data.ipLocal << "/" << data.shareName << ". Mounted to " << data.mountPoint << std::endl;
                return 0;
            }
            else
            {
                std::cout << "Failed to connect to " << data.ipLocal << "/" << data.shareName << std::endl;
            }
        }
        else
        {
            std::cout << "Host at " << data.ipLocal << " is not reachable.\n";
            std::cout << "Trying the host over the internet...\n";
            if (isDeviceReachable(data.ipWeb))
            {
                std::cout << "\nConnecting to host over the internet...\n";
                if (mountSmbShare(data.ipWeb, data.shareName, data.mountPoint, data.creds))
                {
                    std::cout << "Successfully connected to " << data.ipWeb << "/" << data.shareName << ". Mounted to " << data.mountPoint << std::endl;
                    return 1;
                }
                else
                {
                    std::cout << "Failed to connect to " << data.ipWeb << "/" << data.shareName << std::endl;
                }

                std::cout << "Host is not reachable.\n";

                return 2;
            }
        }
    }


}
