/*
README:
    This program is used to resolve the MAC address of a target IP address using ARP protocol. 
    It sends an ARP request to the broadcast address and waits for the ARP reply from the target IP address. 
    The program uses raw sockets to send and receive Ethernet frames.

To compile the code, use the following command:
    gcc arp.c -o arp
*/

#include <sys/types.h>
#include <errno.h> // man 3 perror
#include <arpa/inet.h> //used for htons (from Big Endian to Little Endian)
#include <stdio.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <net/if.h>


/* man 7 packet
Must include:
    <sys/socket.h>, 
    <linux/if_packet.h>
    <net/ethernet.h> // the L2 protocols

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

// Configuration of the node
unsigned char my_mac[6] = {0x22, 0x00, 0xb6, 0xec, 0x6a, 0x3c}; // ifconfig to obtain the real mac
    /* 
    If ifcofig return command not found use /sbin/ifconfig or /usr/sbin/ifconfig
    eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        ...
        ether 22:00:b6:ec:6a:3c  txqueuelen 1000  (Ethernet) <------- Mac address of the node
        ...
    */
unsigned char broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Default
unsigned char my_ip[4] = {139, 162, 197, 68};

/*
Destination MAC address: 6 bytes
Source MAC address: 6 bytes
EtherType: 2 bytes
Payload: 46-1482 bytes
*/
struct eth_frame {
    unsigned char dst[6]; 
    unsigned char src[6];
    unsigned short type;
    unsigned char payload[1]; // We are interested only on the first byte
};

void forge_ethernet (struct eth_frame * eth, unsigned char * dest_addr, unsigned short type) {
    for (int i = 0; i < 6; i++) {
        eth->src[i] = my_mac[i]; // Set the source MAC address of the frame with our MAC address
        eth->dst[i] = dest_addr[i]; // Set the destination MAC address of the frame with the destination address passed as parameter
    }
    eth->type = htons(type); // We must swap the bytes, protocol of the payload, for example 0x0800 for IP, 0x0806 for ARP
}

/* https://www.rfc-editor.org/rfc/rfc826.html - Packet format:

To communicate mappings from <protocol, address> pairs to 48.bit
Ethernet addresses, a packet format that embodies the Address
Resolution protocol is needed.  The format of the packet follows.

----------------------- NOT IN THE ARP PACKET -----------------------
    Ethernet transmission layer (not necessarily accessible to
         the user):
        48.bit: Ethernet address of destination (6 bytes)
        48.bit: Ethernet address of sender (6 bytes)
        16.bit: Protocol type = ether_type$ADDRESS_RESOLUTION 
---------------------------------------------------------------------
    Ethernet packet data:
        16.bit: (ar$hrd) Hardware address space (e.g., Ethernet,
                         Packet Radio Net.) 
        16.bit: (ar$pro) Protocol address space.  For Ethernet
                         hardware, this is from the set of type
                         fields ether_typ$<protocol>.
         8.bit: (ar$hln) byte length of each hardware address
         8.bit: (ar$pln) byte length of each protocol address
        16.bit: (ar$op)  opcode (ares_op$REQUEST | ares_op$REPLY)
        nbytes: (ar$sha) Hardware address of sender of this
                         packet, n from the ar$hln field.
        mbytes: (ar$spa) Protocol address of sender of this
                         packet, m from the ar$pln field.
        nbytes: (ar$tha) Hardware address of target of this
                         packet (if known).
        mbytes: (ar$tpa) Protocol address of target.
*/
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

/* 
The checksum algorithm is:

    The checksum field is the 16 bit one's complement of the one's
    complement sum of all 16 bit words in the header.  For purposes of
    computing the checksum, the value of the checksum field is zero.
*/
unsigned short checksum(unsigned char * b, int len) { // Buffer and length of the buffer
    unsigned short *p = (unsigned short *) b;
    unsigned short tot = 0;
    unsigned short prev = 0;
    for (int i = 0; i < len/2; i++) {
        tot += ntohs(p[i]); // We swap the bytes, header is in big endian, but we are in little endian
        if (prev > tot) // If the sum is less than the previous value, we have an overflow, we must add 1 to the sum
            tot ++;
        prev = tot; // We update the previous value
    }
    return (0xFFFF - tot); // We return the one's complement of the sum, we must swap the bytes again, because the checksum is in big endian (we do it in the function that call the checksum)
}


/* https://datatracker.ietf.org/doc/html/rfc791#ref-9 - Internet Header Format
Version:  4 bits

    The Version field indicates the format of the internet header.  This
    document describes version 4. - In hex : 0x4

IHL:  4 bits

    Internet Header Length is the length of the internet header in 32
    bit words, and thus points to the beginning of the data.  Note that
    the minimum value for a correct header is 5. - In hex : 0x5

Type of Service:  8 bits

    The Type of Service provides an indication of the abstract
    parameters of the quality of service desired, used to guide 
    the selection of the actual service parameters when transmitting a 
    datagram through a particular network. 

      Bits 0-2:  Precedence.
      Bit    3:  0 = Normal Delay,      1 = Low Delay.
      Bits   4:  0 = Normal Throughput, 1 = High Throughput.
      Bits   5:  0 = Normal Relibility, 1 = High Relibility.
      Bit  6-7:  Reserved for Future Use.

         0     1     2     3     4     5     6     7
      +-----+-----+-----+-----+-----+-----+-----+-----+
      |                 |     |     |     |     |     |
      |   PRECEDENCE    |  D  |  T  |  R  |  0  |  0  |
      |                 |     |     |     |     |     |
      +-----+-----+-----+-----+-----+-----+-----+-----+

        Precedence:
            111 - Network Control
            110 - Internetwork Control
            101 - CRITIC/ECP
            100 - Flash Override
            011 - Flash
            010 - Immediate
            001 - Priority
            000 - Routine

Total Length:  16 bits

    Length of the datagram including internet header and data.

Identification:  16 bits

    An identifying value assigned by the sender to aid in assembling the
    fragments of a datagram.

Flags:  3 bits

    Various Control Flags.

      Bit 0: reserved, must be zero
      Bit 1: (DF) 0 = May Fragment,  1 = Don't Fragment.
      Bit 2: (MF) 0 = Last Fragment, 1 = More Fragments.

          0   1   2
        +---+---+---+
        |   | D | M |
        | 0 | F | F |
        +---+---+---+

Fragment Offset:  13 bits

    This field indicates where in the datagram this fragment belongs.
    The first fragment has offset zero.

Time to Live:  8 bits

    This field indicates the maximum time the datagram is allowed to
    remain in the internet system.  If this field contains the value
    zero, then the datagram must be destroyed.  This field is modified
    in internet header processing.  The time is measured in units of
    seconds, but since every module that processes a datagram must
    decrease the TTL by at least one even if it process the datagram in
    less than a second, the TTL must be thought of only as an upper
    bound on the time a datagram may exist.  The intention is to cause
    undeliverable datagrams to be discarded, and to bound the maximum
    datagram lifetime.

Protocol:  8 bits

    This field indicates the next level protocol used in the data
    portion of the internet datagram.  The values for various protocols
    are specified in "Assigned Numbers".
    https://datatracker.ietf.org/doc/html/rfc790

Header Checksum:  16 bits

    Since some header fields change (e.g., time to live), this is recomputed 
    and verified at each point that the internet header is processed.

    The checksum algorithm is:

      The checksum field is the 16 bit one's complement of the one's
      complement sum of all 16 bit words in the header.  For purposes of
      computing the checksum, the value of the checksum field is zero.

    Since it is a 16 bit sum we use unsigned short to store the sum

Source Address:  32 bits

Destination Address:  32 bits
*/
struct ip_datagram {
    unsigned char ver_ihl; // Version and Internet Header Length - 4bits + 4 bits -> char
    unsigned char tos; // Type of Service  
    unsigned short totlen; // Total Length of the datagram (header + payload)
    unsigned short id; // Identification
    unsigned short flags_offs; // Flags and Fragment Offset - 3bits + 13bits -> short
    unsigned char ttl; // Time To Live
    unsigned char proto; //Upper layer protocol
    unsigned short checksum; // Header checksum
    unsigned int srcip; // Source IP address
    unsigned int dstip; // Destination IP address
    unsigned char payload[1]; // We are interested only on the first byte, we can put data inside the payload
};

void forge_ip (struct ip_datagram * ip, char * target_ip, char protocol, unsigned short payload_len) {
    ip->ver_ihl = 0x45; // Version: IPv4, IHL: 5 (20 bytes) we can also write it as (4 << 4) | 5, but it's more clear to write it in hex
    ip->tos = 0;
    ip->totlen = htons(20 + payload_len); // 20 bytes: minimum header length, payload_len: length of the payload
    ip->id = htons(0x1234); // Identification, it can be random
    ip->flags_offs = htons(0); // Now it is 0x0000, May fragment, last fragment, offset 0
    ip->ttl = 128; // Time to live - normal value 128
    ip->proto = protocol; // Protocol (e.g. TCP, UDP, ICMP, ecc...) upper layer protocol we use ICMP : 1

    // The checksum is part of the packet
    // The value of the checksum is zero, before computing it
    ip->checksum = htons(0); // Must be calculated

    // Set the source IP address - my_ip
    ip->srcip = *(unsigned int *)my_ip;

    // Set the destination IP address
    ip->dstip = *(unsigned int*)target_ip;

    // We overwrite the checksum as the rfc says
    ip->checksum = htons(checksum((unsigned char *)ip, 20));
}

// Socket address structure for AF_PACKET
struct sockaddr_ll sll;

// Socket file descriptor
int s; 

// Counter for printing the bytes of the packet
int count = 0; 

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

    printf("\nResolved MAC is...\n");
    for (int i = 0; i < 6; i++)
        printf("%.2X (%.3d) ", target_mac[i], target_mac[i]);
    printf("\n");
}

int resolve_ip (unsigned char * target_ip, unsigned char * target_mac) {
    printf("\nResolving IP address...\n");

    // Size of the packet sent or received
    int size; 

    // Length of the sockaddr_ll structure
    int structlen = sizeof(struct sockaddr_ll); 

    // Buffer for the packet
    unsigned char buf[1500];

    struct eth_frame * eth = (struct eth_frame *) buf;
    struct arp_packet * arp = (struct arp_packet *) eth->payload; // or (*eth).payload

    // Forge the Ethernet frame - 0x0806 is the value of ULP field for ARP
    forge_ethernet(eth, broadcast, 0x0806); // Broadcast address

    // Forge the ARP request
    forge_arp_request(arp, target_ip);

    printf("\nSending ARP request for IP address...\n");
    count = 0; 
    for (int i = 0; i < 14 + sizeof(struct arp_packet); i++) {
        printf("%.2X (%.3d) ", buf[i], buf[i]);
        count = (count == 8) ? 1 : count + 1;
        if (count == 8)
            printf("\n");
    }
    printf("\n");

    // Clean the structure
    for (int i = 0; i < structlen; i++) {
        ((char *)&sll)[i] = 0; // We reset all the bytes to zero
    }

    // Populate the sockaddr_ll structure
    sll.sll_family = AF_PACKET; // Address family
    // If ifcofig return command not found use /sbin/ifconfig or /usr/sbin/ifconfig
    sll.sll_ifindex = if_nametoindex("eth0"); // We have to know the number of the intrface to send the frame, we can obtain it with ifconfig or ip addr show
    
    // Socket creation
    s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    // Check the creation of the socket
    if (s == -1) {
        perror("Socket Failed");
        return -1;
    }

    /* man 2 sendto
    Must include 
        <sys/socket.h>

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

    if (size == -1) {
        perror("Sendto Failed");
        return -1;
    }

    printf("\nARP request (size %d)...\n", size);

    // Clean the structure
    for (int i = 0; i < structlen; i++) {
        ((char *)&sll)[i] = 0; // We reset all the bytes to zero
    }

    // Populate the structure
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = if_nametoindex("eth0");

    /* man 2 recvfrom
    Must include: 
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
                printf("\nARP response received (size = %d)...\n", size);
                count = 0;
                for (int i = 0; i < size; i++) {
                    printf("%.2X (%.3d) ", buf[i], buf[i]);
                    count = (count == 8) ? 1 : count + 1;
                    if (count == 8)
                        printf("\n");
                }
                printf("\n");
                for (int i = 0; i < 6; i++)
                    target_mac[i] = arp->src_mac[i]; // We copy the source MAC address
                return 1; 
            }
        }
        
    }
    return 0; 
}
/*
OUTPUT:
Resolving IP address...

Sending ARP request for IP address...
FF (255) FF (255) FF (255) FF (255) FF (255) FF (255) 22 (034) 00 (000) 
B6 (182) EC (236) 6A (106) 3C (060) 08 (008) 06 (006) 00 (000) 01 (001) 
08 (008) 00 (000) 06 (006) 04 (004) 00 (000) 01 (001) 22 (034) 00 (000) 
B6 (182) EC (236) 6A (106) 3C (060) 8B (139) A2 (162) C5 (197) 44 (068) 
00 (000) 00 (000) 00 (000) 00 (000) 00 (000) 00 (000) 8B (139) A2 (162) 
C5 (197) 46 (070) 

ARP request (size 42)...

ARP response received (size = 42)...
22 (034) 00 (000) B6 (182) EC (236) 6A (106) 3C (060) FE (254) FF (255) 
FF (255) FF (255) FF (255) FF (255) 08 (008) 06 (006) 00 (000) 01 (001) 
08 (008) 00 (000) 06 (006) 04 (004) 00 (000) 02 (002) FE (254) FF (255) 
FF (255) FF (255) FF (255) FF (255) 8B (139) A2 (162) C5 (197) 46 (070) 
22 (034) 00 (000) B6 (182) EC (236) 6A (106) 3C (060) 8B (139) A2 (162) 
C5 (197) 44 (068) 

Resolved MAC is...
FE (254) FF (255) FF (255) FF (255) FF (255) FF (255) 
*/