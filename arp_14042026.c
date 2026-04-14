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

struct ip_datagram {
    unsigned char version_ihl; // Version and Internet Header Length
    unsigned char tos; // Type of Service
    unsigned short total_length; // Total Length of the datagram (header + payload)
    unsigned short id; // Identification
    unsigned short flags_fragment_offset; // Flags and Fragment Offset
    unsigned char ttl; // Time To Live
    unsigned char protocol; // Protocol
    unsigned short header_checksum; 
    unsigned char src_ip[4]; // Source IP address
    unsigned char dst_ip[4]; // Destination IP address
};

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
    unsigned char hlen; // Hardware address length
    unsigned char plen; // Protocol address length
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

void forge_arp_request (struct arp_packet * arp, unsigned char * dst_ip) {
    arp->htype = htons(1); // Hardware type: Ethernet
    arp->ptype = htons(0x0800); // Protocol type: IPv4
    arp->hlen = 6; // Hardware address length
    arp->plen = 4; // Protocol address length
    arp->op = htons(1); // Opcode: ARP request

    // Set the sender MAC and IP addresses
    for (int i = 0; i < 6; i++) 
        arp->src_mac[i] = my_mac[i];

    // Set the sender IP address
    for (int i = 0; i < 4; i++) 
        arp->src_ip[i] = my_ip[i];

    // Set the target MAC address and IP address
    for (int i = 0; i < 6; i++) 
        arp->dst_mac[i] = 0; // ARP request has the target MAC address set to 00:00:00:00:00:00

    // Set the target IP address
    for (int i = 0; i < 4; i++) 
        arp->dst_ip[i] = dst_ip[i];
}

void forge_ip (struct ip_datagram * ip, unsigned char * dst_ip) {
    //ip->version_ihl = (4 << 4) | 5; // Version: IPv4, IHL: 5 (20 bytes)
    ip->version_ihl = 0x45; // Version: IPv4, IHL: 5 (20 bytes)
    ip->tos = 0; // Type of Service
    ip->total_length = htons(20); // Total Length (header + payload), here we have only the header
    ip->id = htons(0); // Identification
    ip->flags_fragment_offset = htons(0); // Flags and Fragment Offset
    ip->ttl = 64; // Time To Live
    ip->protocol = 0; // Protocol (e.g., TCP, UDP, ICMP)
    ip->header_checksum = htons(0); // Header checksum (to be calculated later)

    // Set the source IP address
    for (int i = 0; i < 4; i++) 
        ip->src_ip[i] = my_ip[i];

    // Set the destination IP address
    for (int i = 0; i < 4; i++) 
        ip->dst_ip[i] = dst_ip[i];
}

struct sockaddr_ll sll; // Socket address structure for AF_PACKET

int s; // Socket file descriptor

int resolve_ip (unsigned char * target_ip, unsigned char * target_mac); 

int main() {

    unsigned char target_ip[4] = {139, 162, 197, 70}; // IP address to resolve
    unsigned char target_mac[6]; // Buffer to store the resolved MAC address

    s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    // Check the creation of the socket
    if (s == -1) {
        perror("\nSocket Failed"); // Normal user has not the permission - now 08/04 - he will give us the permission
        return -1;
    }

    resolve_ip(target_ip, target_mac);

    printf("Resolved MAC address:");
    for (int i = 0; i < 6; i++)
        printf("%02.X (%3.d)", target_mac[i], target_mac[i]);

}

int resolve_ip (unsigned char * target_ip, unsigned char * target_mac) {

    int size; 

    int structlen = sizeof(struct sockaddr_ll); 

    unsigned char buf[1500];

    struct eth_frame * eth = (struct eth_frame *) buf;;
    struct arp_packet * arp = (struct arp_packet *) eth->payload; // or (*eth).payload

    // Forge the Ethernet frame - 0x0806 is the value of ULP field for ARP
    forge_ethernet(eth, broadcast, 0x0806); // Broadcast address

    // Forge the ARP request
    forge_arp_request(arp, target_ip);

    printf("Sending ARP request for IP address:");
    for (int i = 0; i < 4; i++) {
        printf("%02.X (%3.d)", buf[i], buf[i]);
    }

    // Clean the structure
    for (int i = 0; i < structlen; i++) {
        ((char *)&sll)[i] = 0; // We reset all the bytes to zero
    }

    // Populate the sockaddr_ll structure
    sll.sll_family = AF_PACKET; // Address family
    // If ifcofig return command not found use /sbin/ifconfig or /usr/sbin/ifconfig
    sll.sll_ifindex = if_nametoindex("eth0"); // We have to know the number of the intrface to send the frame, we can obtain it with ifconfig or ip addr show
    
    /*
    man 2 sendto
    #include <sys/socket.h>
    ssize_t sendto(
        int socket, 
        const void *buffer, 
        size_t length, 
        int flags, 
        const struct sockaddr *dest_addr, 
        socklen_t dest_len
    );
    */
    size = sendto(s, buf, 14 + sizeof(struct arp_packet), 0, (struct sockaddr *) &sll, structlen);

    /*
    man 2 recvfrom
    #include <sys/types.h>
    #include <sys/socket.h>
    ssize_t recvfrom(
        int sockfd,
        void *buf, ---> An array passed without [] is a pointer
        size_t len, ---> Size of the buffer
        int flags, ---> Forget about it
        struct sockaddr *src_addr, ---> Generic struct sockaddr
        socklen_t *addrlen ---> Lenght of the socket passed as a pointer, it could be possible that the size of the structure may be changed by the function, some fields (we don't use) can be modified from the function
    );
    */
    for (int i = 0; i < size; i++) {
        size = recvfrom(s, buf, 1500, 0, (struct sockaddr *)&sll, &structlen);
        if (size == -1) {
            perror("Recvfrom Failed");
            return -1;
        }
        if (eth->type == htons(0x0806)) {
            // When we are inside this if body we are the arp layer and we are receving only arp packets
            if (arp->op == htons(0x0002)) {
                // When we are inside this if body we are receiving only ARP reply
                printf("ARP response received... \n");
                for (int i = 0; i < 4; i++)
                    printf("%02.X (%3.d)", buf[i], buf[i]);
                for (int i = 0; i < 6; i++)
                    target_mac[i] = arp->src_mac[i]; // We copy the source MAC address
                return 1; 
            }
        }
        
    }
    return 0; 
}