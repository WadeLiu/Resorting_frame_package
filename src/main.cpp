#pragma pack(1)

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <algorithm>
#include <vector>

constexpr int MAX_BUFFER_SIZE = 10 * 1024 * 1024;

struct HEADER
{
    int number;
    short len;
};

struct DATA_IDX
{
    int number;
    uintptr_t address;
    int len;
};

int main (int argc, char* argv[])
{
    if(argc != 2)
    {
        printf("\n Usage: %s <ip of server> \n",argv[0]);
        exit(1);
    }

    struct sockaddr_in destAddress = {0};

    struct hostent* hostInfo = gethostbyname(argv[1]);
    if (hostInfo == nullptr)
    {
        exit(1);
    }

    char *ip = inet_ntoa(*(struct in_addr*)*hostInfo->h_addr_list);

    destAddress.sin_family = AF_INET;
    destAddress.sin_port = htons(49152);
    memcpy((char *) &destAddress.sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);

    int sockfd = 0;
    if ((sockfd = socket(destAddress.sin_family, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&destAddress, sizeof(destAddress)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       exit(1);
    }

    char* buffer = new char[MAX_BUFFER_SIZE];
    memset(buffer, 0x00, MAX_BUFFER_SIZE);

    size_t size = 0;
    std::vector<DATA_IDX> vecData;

    int len = 0;
    int readPos = 0;
    int writePos = 0;

    int idx = 0;
    while ((len = read(sockfd, buffer + writePos, MAX_BUFFER_SIZE)) > 0)
    {
        writePos += len;

        if ((writePos - readPos) < sizeof(HEADER))
        {
            continue;
        }

        HEADER* header = (HEADER*)(buffer + readPos);
        int packageNumber   = __bswap_32(header->number);   // be32toh(header->number)
        int packageLen      = __bswap_16(header->len);      // be16toh(header->len)

        if ((writePos - readPos) < (sizeof(HEADER) + packageLen))
        {
            continue;
        }

        // get payload
        char* payload = buffer + (readPos + sizeof(HEADER));
        vecData.push_back({packageNumber, (uintptr_t)payload, packageLen});

        readPos += (sizeof(HEADER) + packageLen);
    }

    if(len < 0)
    {
        printf("\n Read error \n");
    }

    std::sort(vecData.begin(), vecData.end(), [](const DATA_IDX lhs, const DATA_IDX rhs)
    {
        return lhs.number < rhs.number;
    });

    FILE* pFile = fopen("ffd8_ffd9", "wb");

    for (auto& data : vecData)
    {
        printf("seq = %d\n", data.number);
        fwrite((const void*)data.address, 1, data.len, pFile);
    }

    fclose(pFile);

    printf("Total count = %ld, total size(%d)\n", vecData.size(), writePos);

    close(sockfd);

    if (buffer != nullptr)
    {
        delete[] buffer;
    }

    return 0;
}
