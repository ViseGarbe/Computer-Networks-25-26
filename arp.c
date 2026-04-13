#include <sys/types.h>
#include <errno.h> // man 3 perror
#include <arpa/inet.h> //used for htons (from Big Endian to Little Endian)
#include <stdio.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <net/if.h>


/* man 7 packet
// Must include <sys/socket.h>, <linux/if_packet.h>, #include <net/ethernet.h> // the L2 protocols
struct sockaddr_ll {
    unsigned short sll_family;       <-----------
    unsigned short sll_protocol;     Not used
    int sll_ifindex;                 <-----------
    unsigned short sll_hatype;       Not used
    unsigned char  sll_pkttype;      Not used
    unsigned char  sll_halen;        Not used
    unsigned char  sll_addr[8];      Not used
};
*/

//protocol is in general 0, but we can specify a particular protocol if needed

// Configuration of the node
unsigned char my_mac[6] = {22, 0x00, 0xb6, 0xec, 0x6a, 0x3c}; // ifconfig to obtain the real mac
/* 
If ifcofig return command not found use /sbin/ifconfig or /usr/sbin/ifconfig
eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
    ...
    ether 22:00:b6:ec:6a:3c  txqueuelen 1000  (Ethernet) <------- Mac address of the node
    ...
*/
unsigned char broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Default
unsigned char my_ip[4] = {139, 162, 197, 68};

unsigned char buf[1500];

struct eth_frame {
    unsigned char dst[6];
    unsigned char src[6];
    unsigned short type;
    unsigned char payload[1]; // We are interested only on the first byte
};

// man 7 ARP
struct arp_packet {
    unsigned short htype; // Hardware type
    unsigned short ptype; // Protocol type
    unsigned char hlen;
    unsigned char plen;
    unsigned short op; // Opcode
    unsigned char src_mac[6]; // Sender MAC address
    unsigned char src_ip[4]; // Sender IP address
    unsigned char dst_mac[6]; // Target MAC address, ARP request is 00:00:00:00:00:00, ARP reply real MAC address of the target
    unsigned char dst_ip[4]; // Target IP address
};

void forge_ethernet (struct eth_frame * eth, unsigned char * dest_addr, unsigned char type) {
    for (int i = 0; i < 6; i++) {
            eth->src[i] = my_mac[i]; // Set the source MAC address of the frame with our MAC address
            eth->dst[i] = dest_addr[i]; // Set the destination MAC address of the frame with the destination address passed as parameter
    }
    eth->type = htons(type); // We must swap the bytes
}

int main() {

    struct sockaddr_ll sll;

    struct eth_frame * eth;
    struct arp_packet * arp;

    eth = (struct eth_frame *) buf;
    arp = (struct arp_packet *) eth->payload; // or (*eth).payload

    int s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    // Check the creation of the socket

    if (s == -1) {
        perror("\nSocket Failed"); // Normal user has not the permission - now 08/04 - he will give us the permission
        return -1;
    }

    int structlen = sizeof(struct sockaddr_ll);

    // Clean the structure
    for (int i = 0; i < structlen; i++) {
        ((char *)&sll)[i] = 0; // We reset all the bytes to zero
    }

    // Populate the struct
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = if_nametoindex("eth0");// We have to know the number of the interface, use command ifconfig

    // #include <sys/types.h>
    // #include <sys/socket.h>
    // ssize_t recvfrom(
    //      int sockfd,
    //      void *buf, ---> An array passed without [] is a pointer
    //      size_t len, ---> Size of the buffer
    //      int flags, ---> Forget about it
    //      struct sockaddr *src_addr, ---> Generic struct sockaddr
    //      socklen_t *addrlen ---> Lenght of the socket passed as a pointer, it could be possible that the size of the structure may be changed by the function, some fields (we don't use) can be modified from the function
    // );
    int size = recvfrom(s, buf, 1500, 0, (struct sockaddr *)&sll, &structlen);

    if (size == -1) {
        perror("Recvfrom Failed");
        return -1;
    }

    for (int i = 0; i < size; i++) {
        printf("%2.X(%3d) ", buf[i], buf[i]);
    }
}