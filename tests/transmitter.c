#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <netinet/in.h>

#define MULTICAST_IP "239.255.0.1"
#define MULTICAST_PORT 12345

struct packet {
    char msg[5];
    uint32_t ipv4;    
} __attribute__((packed));

int main() {
    int sockfd;
    struct sockaddr_in addr;

    struct packet p;
    p.msg[0] = 'H';
    p.msg[1] = 'E';
    p.msg[2] = 'L';
    p.msg[3] = 'L';
    p.msg[4] = 'O';

    inet_pton(AF_INET, "127.0.0.1", &p.ipv4); // this is already network-byte order

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(MULTICAST_PORT);
    inet_pton(AF_INET, MULTICAST_IP, &addr.sin_addr);

    while (1) {
        sendto(sockfd, &p, sizeof(p), 0,
               (struct sockaddr *)&addr, sizeof(addr));
        printf("Multicast message sent\n");
        sleep(2);
    }

    close(sockfd);
    return 0;
}

