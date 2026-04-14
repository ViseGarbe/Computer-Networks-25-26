#include <sys/types.h>
#include <errno.h> // man 3 perror
#include <arpa/inet.h> //used for htons (from Big Endian to Little Endian
#include <stdio.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <net/if.h>


/* man 7 packet
// Must include <sys/socket.h>, <linux/if_packet.h>, #include <net/ethernet.h> // the L2 protocols
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

//protocol is in general 0, but we can specify a particular protocol if needed

unsigned char rxbuf[1500];

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

    // man recvfrom
    // #include <sys/types.h>
    // #include <sys/socket.h>
    // ssize_t recvfrom(
    //      int sockfd, ---> The socket created
    //      void *buf, ---> An array passed without [] is a pointer, it is the buffer where the data will be stored
    //      size_t len, ---> Size of the buffer, in our case 1500, the maximum size of an Ethernet frame
    //      int flags, ---> Forget about it, we can set it to zero
    //      struct sockaddr *src_addr, ---> Generic struct sockaddr, we have to cast our struct sockaddr_ll to struct sockaddr
    //      socklen_t *addrlen ---> Lenght of the socket passed as a pointer, it could be possible that the size of the structure may be changed by the function, some fields (we don't use) can be modified from the function 
    // );
    int size = recvfrom(s, rxbuf, 1500, 0, (struct sockaddr *)&sll, &structlen);

    if (size == -1) {
        perror("Recvfrom Failed");
        return -1;
    }

    for (int i = 0; i < size; i++) {
        printf("%2.X(%.3d) ", rxbuf[i], rxbuf[i]);
    }
}