#include <sys/types.h>
#include <errno.h> // man 3 perror
#include <arpa/inet.h> //used for htons (from Big Endian to Little Endian
#include <stdio.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <net/if.h>


/* man 7 packet
Must include: 
    <sys/socket.h>
    <linux/if_packet.h> 
    <net/ethernet.h> // the L2 protocols

struct sockaddr_ll {
    unsigned short sll_family;      <-----------
    unsigned short sll_protocol;    Not used
    int sll_ifindex;                <-----------       
    unsigned short sll_hatype;      Not used
    unsigned char  sll_pkttype;     Not used
    unsigned char  sll_halen;       Not used
    unsigned char  sll_addr[8];     Not used
};
*/

// Buffer for receiving packets
unsigned char rxbuf[1500];

int count = 0; // We will use it to print 8 bytes per line

int main() {

    struct sockaddr_ll sll;

    int s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    // Check the creation of the socket 

    if (s == -1) {
        perror("\nSocket Failed"); // Normal user has not the permission - now 08/04 - he will give us the permission 
        return -1;
    }

    int structlen = sizeof(struct sockaddr_ll);

    // Clean the structure 
    for (int i = 0; i < structlen; i++) {
        ((char *)&sll) [i] = 0; // We reset all the bytes to zero
    }

    // Populate the struct
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = if_nametoindex("eth0");// We have to know the number of the interface, use command ifconfig

    /* man recvfrom
    Must include: 
        <sys/types.h>
        <sys/socket.h>

    ssize_t recvfrom(
        int sockfd, ---> The socket created
        void *buf, ---> An array passed without [] is a pointer, it is the buffer where the data will be stored
        size_t len, ---> Size of the buffer, in our case 1500, the maximum size of an Ethernet frame
        int flags, ---> Forget about it, we can set it to zero
        struct sockaddr *src_addr, ---> Generic struct sockaddr, we have to cast our struct sockaddr_ll to struct sockaddr
        socklen_t *addrlen ---> Lenght of the socket passed as a pointer, it could be possible that the size of the structure may be changed by the function, some fields (we don't use) can be modified from the function 
    );
    */
    int size = recvfrom(s, rxbuf, 1500, 0, (struct sockaddr *)&sll, &structlen);

    if (size == -1) {
        perror("Recvfrom Failed");
        return -1;
    }

    for (int i = 0; i < size; i++) {
        printf("%.2X(%.3d) ", rxbuf[i], rxbuf[i]);
        count = (count == 8) ? 1 : count + 1;
        if (count == 8)
            printf("\n");
    }
    printf("\n");
}
/*
OUTPUT:
22(034) 00(000) B6(182) EC(236) 6A(106) 3C(060) FE(254) FF(255) 
FF(255) FF(255) FF(255) FF(255) 08(008) 00(000) 45(069) 40(064) 
00(000) 34(052) 00(000) 00(000) 40(064) 00(000) 2D(045) 06(006) 
D1(209) 66(102) BC(188) DB(219) 6E(110) 5B(091) 8B(139) A2(162) 
C5(197) 44(068) FA(250) 25(037) 00(000) 16(022) 05(005) 79(121) 
3C(060) 59(089) A7(167) 28(040) 0F(015) 3E(062) 80(128) 10(016) 
07(007) FF(255) 80(128) DF(223) 00(000) 00(000) 01(001) 01(001) 
08(008) 0A(010) 30(048) 0A(010) BA(186) 0E(014) BF(191) 70(112) 
D5(213) C2(194) 
*/