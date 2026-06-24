/*
READEME:
    Modify the ping.c program to implement a "traceroute" program. 
    The program sends many ip packets destined to 147.162.2.100 having an increasing TTL value from 1 to 30 
    and reports the list of the IP addresses of the gateways encountered in the network path. 

To compile the code, use the following command:
    gcc traceroute.c -o np
*/

#include <sys/time.h> // Include the sys/time.h header for timer-related functions and structures
#include <sys/types.h>
#include <errno.h> // man 3 perror
#include <arpa/inet.h> //used for htons (from Big Endian to Little Endian)
#include <stdio.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <net/if.h>

#define PAYLOAD_ICMP 20

/*man 7 packet
Must include:
    <sys/socket.h>, 
    <linux/if_packet.h>
    <net/ethernet.h> // the L2 protocols

struct sockaddr_ll {
    unsigned short sll_family;      <-----------
    unsigned short sll_protocol;    Not used
    int sll_ifindex;                <-----------
    unsigned short sll_hatype;      Not used
    unsigned char sll_pkttype;      Not used
    unsigned char sll_halen;        Not used
    unsigned char sll_addr[8];      Not used
};
*/

// Configuration of the node
unsigned char my_mac[6] = {0x22, 0x00, 0xb6, 0xec, 0x6a, 0x3c};// ifconfig to obtain the real mac
    /* 
    If ifcofig return command not found use /sbin/ifconfig or /usr/sbin/ifconfig
    eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        ...
        ether 22:00:b6:ec:6a:3c  txqueuelen 1000  (Ethernet) <------- Mac address of the node
        ...
    */
unsigned char broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Default
unsigned char my_ip[4] = {139, 162, 197, 68};
unsigned char netmask[4] = {255, 255, 255, 0}; // ifconfig to obtain the netmask, /sbin/ifconfig
unsigned char gateway[4] = {139, 162, 197, 1};

/*
Minimum size: 6 + 6 + 2 = 14 bytes for the Ethernet header

Destination MAC address: 6 bytes
Source MAC address: 6 bytes
EtherType: 2 bytes
Payload: 46-1482 bytes
*/
struct eth_frame {
    unsigned char dst[6]; // Destination MAC address
    unsigned char src[6]; // Source MAC address
    unsigned short type; // EtherType field to indicate the protocol of the payload (e.g., 0x0800 for IP, 0x0806 for ARP) - aka ULP field
    unsigned char payload[1]; // We are interested only on the first byte
};

void forge_ethernet (struct eth_frame * eth, unsigned char * dest_addr, unsigned short type) {
    for (int i = 0; i < 6; i++) {
        eth->src[i] = my_mac[i];
        eth->dst[i] = dest_addr[i]; 
    }
    eth->type = htons(type); // We must swap the bytes
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
    unsigned char dst_mac[6]; // Target MAC address
    unsigned char dst_ip[4]; // Target IP address
};

void forge_arp_request (struct arp_packet * arp, unsigned char * dest_ip ) {
    arp->htype = htons(1); // Hardware type: Ethernet = 1, we must swap the bytes because the field is in big endian, but we are in little endian
    arp->ptype = htons(0x0800); // Protocol type: IP = 0x0800, ARP = 0x0806
    arp->hlen = 6; // Hardware address length: 6 bytes for Ethernet
    arp->plen = 4; // Protocol address length: 4 bytes for IP
    arp->op = htons(1); // Opcode: REQUEST = 1, REPLY = 2

    for (int i = 0; i < 6; i++) {
        arp->src_mac[i] = my_mac[i]; // Set the sender MAC address of the ARP request with our MAC address
    }
    for (int i = 0; i < 4; i++) {
        arp->src_ip[i] = my_ip[i]; // Set the sender IP address of the ARP request with our IP address
    }
    for (int i = 0; i < 6; i++) {
        arp->dst_mac[i] = 0; // Set the target MAC address of the ARP request to 00:00:00:00:00:00, because we don't know it yet
    }
    for (int i = 0; i < 4; i++) {
        arp->dst_ip[i] = dest_ip[i]; // Set the target IP address of the ARP request with the destination IP address passed as parameter
    }
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

A summary of the contents of the internet header follows:

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |Version|  IHL  |Type of Service|          Total Length         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         Identification        |Flags|      Fragment Offset    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Time to Live |    Protocol   |         Header Checksum       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       Source Address                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Destination Address                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Options                    |    Padding    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

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

// Exercise 8.6 - "[...] having an increasing TTL value from 1 to 30  [...]"
void forge_ip (struct ip_datagram * ip, char * target_ip, char protocol, unsigned short payload_len, unsigned char ttl, unsigned short id) {
    ip->ver_ihl = 0x45; // Version: IPv4, IHL: 5 (20 bytes) we can also write it as (4 << 4) | 5, but it's more clear to write it in hex
    ip->tos = 0;
    ip->totlen = htons(20 + payload_len); // 20 bytes: minimum header length, payload_len: length of the payload
    //ip->id = htons(0); // Identification, it can be random

    // --> Modified for Exercise 8.6
    ip->id = htons(id); // Identification, it can be random

    ip->flags_offs = htons(0); // Now it is 0x0000, May fragment, last fragment, offset 0
    //ip->ttl = 128; // Time to live - normal value 128

    // --> Modified for Exercise 8.6
    ip->ttl = ttl; // Time to live - we set it to the value of the variable ttl, that we will increase in the main function

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

/* https://datatracker.ietf.org/doc/html/rfc792 - ICMP Message Format
Echo or Echo Reply Message

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Type      |     Code      |          Checksum             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Identifier          |        Sequence Number        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Data ...
   +-+-+-+-+-

Type - 1 byte -> char

    8 for echo message;
    0 for echo reply message.

Code - 1 byte -> char

    0 - No Code for echo and echo reply message

Checksum - 2 bytes -> short

    The checksum is the 16-bit ones's complement of the one's
    complement sum of the ICMP message starting with the ICMP Type.
    For computing the checksum , the checksum field should be zero.
    If the total length is odd, the received data is padded with one
    octet of zeros for computing the checksum. This checksum may be
    replaced in the future. - Use the defined checksum function

Identifier - 2 bytes -> short

    If code = 0, an identifier to aid in matching echos and replies,
    may be zero. That means that we can choose any value for the identifier,
    but it is common to use id = 0 if the code is 0

Sequence Number - 2 bytes -> short

    If code = 0, a sequence number to aid in matching echos and replies,
    may be zero. That means that we can choose any value for the sequence number,
    but it is common to use seq = 0 if the code is 0
*/
// The identifier: specific for a given session - ping session
struct icmp_message {
    unsigned char type; // Type of the ICMP message
    unsigned char code; // Code of the ICMP message
    unsigned short checksum; // Checksum of the ICMP message
    unsigned short id; // Identifier of the ICMP message
    unsigned short seq; // Sequence number of the ICMP message
    unsigned char payload[1]; // Payload of the ICMP message
};

void forge_icmp(struct icmp_message * icmp, unsigned short id, unsigned short seq) {
    icmp->type = 8; // Echo request message, for echo reply message we must set it to 0
    icmp->code = 0; // No need for icmp
    icmp->checksum = 0; // Overwrite it later;
    //icmp->id = htons(0xABCD); // Identifier, it can be random, but it is common to use 0 if code is 0
    //icmp->seq = htons(1); // Sequence number, it can be random, but it is common to use 0 if code is 0

    // --> Modified for Exercise 8.6
    icmp->id = htons(id);
    icmp->seq = htons(seq); 

    // Why we should put some data inside a ping request? Which data we have to put inside?
    // In general: if we want to test the nwtwork we must try different size of the packet, test round trip time, fragmentation ecc...
    // The possibility to send data that we can fully describe
    for (int i = 0; i < PAYLOAD_ICMP; i++)
        icmp->payload[i] = i;

    icmp->checksum = htons(checksum((unsigned char *)icmp, 8 + 20));
}

struct time_exceeded_message {
    unsigned char type; 
    unsigned char code; 
    unsigned short checksum;
    unsigned char unused[4]; // Unused field, must be set to zero
    unsigned char payload[1]; 
};

// Socket address structure for the link layer (Ethernet)
struct sockaddr_ll sll; 

// Socket descriptor
int s;

// Counter for printing the bytes of the packet
int count = 0; 

int resolve_ip(unsigned char * target_ip, unsigned char * target_mac) {
    printf("\nResolving IP address...\n");

    // Size of the packet sent or received
    int size;

    // Length of the sockaddr_ll structure
    int structlen;

    // Buffer for the packet
    unsigned char buf[1500];

    struct eth_frame * eth = (struct eth_frame *) buf;
    struct arp_packet * arp = (struct arp_packet *) eth->payload; // or (*eth).payload

    // 0x0806 value of ULP for arp packet
    forge_ethernet(eth, broadcast, 0x0806);

    // Forge the ARP request
    forge_arp_request(arp, target_ip);

    printf("\nARP request prepared...\n");
    count = 0;
    // Why 14? Because the Ethernet header is 14 bytes, and the arp packet is after the Ethernet header, 
    // so we must start to print from the first byte of the Ethernet header, and we must print all the bytes 
    // of the Ethernet header and all the bytes of the arp packet, so we must print 14 + sizeof(struct arp_packet) bytes
    for (int i = 0; i < 14 + sizeof(struct arp_packet); i++) {
        printf("%.2X (%.3d) ", buf[i], buf[i]);
        count = (count == 8) ? 1 : count + 1;
        if (count == 8)
            printf("\n");
    }
    printf("\n");

    structlen = sizeof(struct sockaddr_ll);

    // Clean the structure
    for (int i = 0; i < structlen; i++) {
        ((char *)&sll)[i] = 0; // We reset all the bytes to zero
    }

    // Populate the structure
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = if_nametoindex("eth0");

    // Socket creation
    s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    // Check the creation of the socket
    if (s == -1) {
        perror("Socket Failed");
        return -1;
    }

    /* man 2 sendto - Invio della ARP request
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

    printf("Sending ARP request (size %d)...\n", size);

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

    for (int i = 0; i < 1000; i++) {
        size = recvfrom (s, buf, 1500, 0, (struct sockaddr *)&sll, &structlen);
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
                    target_mac[i] = arp->src_mac[i];
                return 1;
            }
        }
    }
    return 0;
}

int main() {
    unsigned char target_ip[4] = {147, 162, 2, 100};
    unsigned char target_mac[6];
    unsigned char buffer[1500];

    int size;
    int structlen;

    unsigned char ttl;
    unsigned short session_id = 0xABCD; // Defined session id
    unsigned short ip_id = 0;
    unsigned short seq;

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    struct eth_frame *eth = (struct eth_frame *)buffer;
    struct ip_datagram *ip = (struct ip_datagram *)eth->payload;
    struct icmp_message *icmp = (struct icmp_message *)ip->payload;

    if (((*(unsigned int *)target_ip) & (*(unsigned int *)netmask)) == ((*(unsigned int *)my_ip) & (*(unsigned int *)netmask))) {
        printf("\nThe target IP address is in the same subnet of the node, we can resolve the MAC address of the target IP address...\n");
        resolve_ip(target_ip, target_mac);
    } else {
        printf("\nThe target IP address is not in the same subnet of the node, we must resolve the MAC address of the gateway...\n");
        resolve_ip(gateway, target_mac);
    }

    printf("\nTraceroute to %d.%d.%d.%d\n", target_ip[0], target_ip[1], target_ip[2], target_ip[3]);

    structlen = sizeof(struct sockaddr_ll);

    for (int i = 0; i < structlen; i++)
        ((char *)&sll)[i] = 0;

    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = if_nametoindex("eth0");

    s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (s == -1) {
        perror("Socket Failed");
        return -1;
    }

    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        perror("setsockopt failed");
        return -1;
    }

    for (ttl = 1; ttl <= 30; ttl++) {
        /*
        hop_found = 0 means that we haven't received a reply from the target IP address
        hop_found = 1 means that we have received a reply
        */

        int hop_found = 0;

        // We set the sequence number of the ICMP message to the value of the global variable ttl
        seq = ttl;
        // We increase the IP identification field for each packet, to be able to distinguish the packets in the reply messages
        ip_id++;

        forge_icmp(icmp, session_id, seq);
        forge_ip(ip, target_ip, 1, 8 + 20, ttl, ip_id);
        forge_ethernet(eth, target_mac, 0x0800);

        // Ethernet header: 14
        // IP header: 20
        // Header ICMP: 8
        // Payload ICMP: PAYLOAD_ICMP
        size = sendto(s, buffer, 14 + 20 + 8 + PAYLOAD_ICMP, 0, (struct sockaddr *)&sll, structlen);

        if (size == -1) {
            perror("Sendto Failed");
            return -1;
        }
        
        while (!hop_found) {
            // eth_r is the Ethernet frame of the received packet
            // outer_ip is the IP header of the received packet 
            // outer_icmp is the ICMP message of the received packet 
            // outer_ihl is the Internet Header Length of the received packet
            struct eth_frame *eth_r;
            struct ip_datagram *outer_ip;
            struct icmp_message *outer_icmp;
            unsigned char outer_ihl;

            size = recvfrom(s, buffer, 1500, 0, NULL, NULL);

            if (size == -1) {
                // Dato che è stato impostato un timeout di ricezione e il timeout è scaduto prima che i dati venissero ricevuti. 
                // La Socket restituisce EAGAIN oppure EWOULDBLOCK che indicano che non ci sono dati da leggere.
                // In tutti gli altri casi di errore stampiamo l'errore ed unsciamo dal programma.
                // man 3 errno
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    printf("%2d\t*\t\t\tThe error is EAGAIN or EWOULDBLOCK \n", ttl);
                    break;
                }
                perror("Recvfrom Failed");
                return -1;
            }

            /*
            Road map of the received packet:

            Ethernet
            └── outer IP
                └── outer ICMP
                    ├── se type 11: contiene inner IP + inner ICMP originale
                    └── se type 0: e' direttamente Echo Reply
            */

            /*
            Check:
                14 byte = header Ethernet
                20 byte = header IP minimo
                8 byte  = header ICMP minimo
            */
            if (size >= 14 + 20 + 8) {

                // buffer contiene l'intero pacchetto ricevuto, quindi contiene:
                //      - l'Ethernet header
                //      - l'IP header
                //      - l'ICMP header
                //      - il payload dell'ICMP message che può anche essere "vuoto" 
                eth_r = (struct eth_frame *)buffer;

                // 0x0800 value for IP packet
                if (eth_r->type == htons(0x0800)) {
                    // We are receiving an IP packet
                    
                    // Eth
                    // Interpretiamo il payload dell'Ethernet frame come un IP datagram
                    // Otteniamo il puntatore all'IP header del pacchetto ricevuto
                    outer_ip = (struct ip_datagram *)eth_r->payload;

                    // proto == 1 means that the upper layer protocol is ICMP
                    /*
                    Protocol field: 
                        1 = ICMP
                        6 = TCP
                        17 = UDP
                    */
                    if (outer_ip->proto == 1) {
                        // We are receiving an ICMP packet

                        // Size of the recieved packet
                        /*
                        Version:  4 bits

                            The Version field indicates the format of the internet header.  This
                            document describes version 4.

                        IHL:  4 bits

                            Internet Header Length is the length of the internet header in 32
                            bit words, and thus points to the beginning of the data.  Note that
                            the minimum value for a correct header is 5.
                        
                        We must remove the first 4 bits of the ver_ihl field to obtain the IHL value
                        since the IHL value is in the last 4 bits.
                        We can do a bitwise AND with 0x0F to obtain only the last 4 bits
                        then we must multiply it by 4 to obtain the length in bytes, 
                        because the IHL value is in 32 bit words (4 bytes) 
                        */
                        outer_ihl = (outer_ip->ver_ihl & 0x0F) * 4;

                        /*
                        Check:
                            14 byte = header Ethernet
                            outer_ihl byte = header outer_ip calcolato
                            8 byte  = header ICMP minimo
                        */
                        if (size >= 14 + outer_ihl + 8) {

                            /*
                            Assegno il puntatore all'ICMP message del pacchetto ricevuto, che si trova dopo l'IP header, 
                            quindi dopo 14 byte (già considerati assegnando outer_ip = ... eth_r->payload;) di Ethernet 
                            header e outer_ihl byte di IP header
                            */
                            outer_icmp = (struct icmp_message *)((unsigned char *)outer_ip + outer_ihl);

                            // type == 11 means that the ICMP message is a Time Exceeded Message
                            // type == 0 means that the ICMP message is an Echo Reply Message
                            // code == 0 means that there is no code for Echo and Echo Reply Messages
                            if (outer_icmp->type == 11 && outer_icmp->code == 0) {

                                /*
                                Un messaggio Time Exceeded contiene:

                                ICMP Time Exceeded header
                                └── IP originale che avevo inviato
                                    └── primi 8 byte del payload originale
                                */

                                struct time_exceeded_message *tem;
                                struct ip_datagram *inner_ip; 
                                struct icmp_message *inner_icmp;
                                unsigned char inner_ihl;

                                /*
                                Check:
                                    14 byte = header Ethernet
                                    outer_ihl byte = header outer_ip calcolato
                                    8 byte  = header ICMP minimo
                                    20 byte = header IP minimo (generico) 
                                        -> potrebbe essere un header IP di qualsiasi pacchetto, 
                                           non necessariamente il nostro ping request, ma se è più piccolo di 20 byte non possiamo processarlo
                                */
                                if (size >= 14 + outer_ihl + 8 + 20) {
                                    // The payload of the Time Exceeded message contains the IP header and the first 8 bytes 
                                    // of the payload of the original packet that caused the Time Exceeded message to be generated, 
                                    // in this case our ping request, so we can check if the Time Exceeded message is a reply to 
                                    // our ping request by checking the identifier and the sequence number of the inner ICMP message
                                    tem = (struct time_exceeded_message *)outer_icmp; 
                                    inner_ip = (struct ip_datagram *)tem->payload;

                                    /*
                                    We remove the first 4 bits of the ver_ihl field, obtain the IHL value (bitwise AND with 0x0F to obtain only the last 4 bits)
                                    then we multiply it by 4 to obtain the length in bytes since the IHL value is in 32 bit words (4 bytes) 
                                    */
                                    inner_ihl = (inner_ip->ver_ihl & 0x0F) * 4;

                                    /*
                                    Check:
                                        14 byte = header Ethernet
                                        outer_ihl byte = header outer_ip calcolato
                                        8 byte  = header ICMP minimo
                                        inner_ihl byte = header inner_ip calcolato
                                        8 byte  = header ICMP minimo (inner)
                                    */
                                    if (size >= 14 + outer_ihl + 8 + inner_ihl + 8) {

                                        /*
                                        Assegno il puntatore all'ICMP message del pacchetto ricevuto, che si trova dopo l'IP header, 
                                        quindi dopo 14 byte (già considerati assegnando outer_ip = ... eth_r->payload;) di Ethernet 
                                        header e outer_ihl byte di IP header
                                        */
                                        inner_icmp = (struct icmp_message *)((unsigned char *)inner_ip + inner_ihl);
                                        /*
                                        eth_r = (struct eth_frame *)buffer;
                                        outer_ip = (struct ip_datagram *)eth_r->payload;
                                        outer_icmp = (struct icmp_message *)((unsigned char *)outer_ip + outer_ihl);
                                            outer_ihl = lunghezza dell'IP header del pacchetto ricevuto

                                        tem = (struct time_exceeded_message *)outer_icmp;
                                        inner_ip = (struct ip_datagram *)tem->payload;
                                        inner_icmp = (struct icmp_message *)((unsigned char *)inner_ip + inner_ihl);
                                            inner_ihl = lunghezza dell'IP header del pacchetto originale che ha causato il Time Exceeded message
                                        */

                                        // We check the identifier and the sequence number of the inner ICMP message, to be sure that it is a reply to our ping request
                                        if (inner_icmp->id == htons(session_id) && inner_icmp->seq == htons(seq)) {
                                            // Se l'id e il seq corrispondono a quelli inviati significa che il Time Exceeded message è una risposta al nostro ping request, 
                                            // quindi possiamo stampare l'indirizzo IP del nodo che ha generato il Time Exceeded message, 
                                            // che si trova nel campo srcip dell'IP header del pacchetto ricevuto (outer_ip->srcip), 
                                            // e il TTL value (ttl) che abbiamo usato per inviare il ping request
                                            
                                            unsigned char *src_ip = (unsigned char *)&outer_ip->srcip;
                                            printf("%2d\t%d.%d.%d.%d\t\tICMP Time Exceeded\n", ttl, src_ip[0], src_ip[1], src_ip[2], src_ip[3]);
                                            hop_found = 1; // We set the hop_found variable to 1 to exit the while loop and increase the TTL value for the next ping request
                                        }
                                    }
                                }
                            } else if (outer_icmp->type == 0) { // If the type of the ICMP message is 0, it means that it is an Echo Reply message, so we check if it is a reply to our ping request by checking the identifier and the sequence number of the outer ICMP message
                                if (outer_icmp->id == htons(session_id) && outer_icmp->seq == htons(seq)) {
                                    unsigned char *src_ip = (unsigned char *)&outer_ip->srcip;
                                    printf("%2d\t%d.%d.%d.%d\t\tEcho Reply\n", ttl, src_ip[0], src_ip[1], src_ip[2], src_ip[3]);
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
