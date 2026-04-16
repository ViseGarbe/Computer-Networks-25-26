#include <sys/types.h>
#include <errno.h> // man 3 perror
#include <arpa/inet.h> //used for htons (from Big Endian to Little Endian)
#include <stdio.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <net/if.h>

/*
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

//protocol is in general 0, but we can specify a particular protocol if needed

// Configuration of the node
unsigned char my_mac[6] = {22,0x00, 0xb6, 0xec, 0x6a, 0x3c}; // ifconfig to obtain the real mac eth0
unsigned char broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Default
unsigned char my_ip[4] = {139, 162, 197, 68};
unsigned char netmask[4] = {255, 255, 255, 0}; // ifconfig to obtain the netmask, /sbin/ifconfig
unsigned char gateway[4] = {139, 162, 197, 1};

/*
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

void forge_icmp(struct icmp_message * icmp) {
    icmp->type = 8; // Echo request message
    icmp->code = 0; // No need for icmp
    icmp->checksum = 0; // Overwrite it later;
    icmp->id = htons(0xABCD); // Identifier, it can be random, but it is common to use 0 if code is 0
    icmp->seq = htons(1); // Sequence number, it can be random, but it is common to use 0 if code is 0

    // Why we should put some data inside a ping request? Which data we have to put inside?
    // In general: if we want to test the nwtwork we must try different size of the packet, test round trip time, fragmentation ecc...
    // The possibility to send data that we can fully describe
    for (int i = 0; i < 20; i++)
        icmp->payload[i] = i;

    icmp->checksum = htons(checksum((unsigned char *)icmp, 8+20));
}

struct sockaddr_ll sll;

int s;

int resolve_ip (unsigned char * target_ip, unsigned char * target_mac);

int main() {
    unsigned char target_ip[4] = {139, 162, 197, 70};
    unsigned char target_mac[6];
    unsigned char buffer[1500];

    // Bisogna spostare la creazione della socket qui
    s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    // Check the creation of the socket
    if (s == -1) {
        perror("Socket Failed");
        return -1;
    }

    // --->
    struct eth_frame * eth = (struct eth_frame *) buffer;
    struct ip_datagram * ip = (struct ip_datagram *) eth->payload;
    struct icmp_message * icmp = (struct icmp_message *) ip->payload;

    forge_icmp(icmp);
    /*
        Protocol = 1 = ICMP, 
        payload_length = 8 + 20
            8 bytes for the ICMP header
            20 bytes for the ICMP payload
    */
    forge_ip(ip, target_ip, 1, 8 + 20); 

    // Compare my_ip masked with the target_ip masked and if they are equal we are in the same network
    if (((*(unsigned int *) target_ip) & (*(unsigned int *) netmask)) == ((*(unsigned int *) my_ip) & (*(unsigned int *) netmask))) {
        resolve_ip(target_ip, target_mac);
    } else {
        resolve_ip(gateway, target_mac);
    }

    printf("Resolved MAC is...\n");

    for (int i = 0; i < 6; i++)
        printf("%2.X (%3.d)", target_mac[i], target_mac[i]);

    forge_ethernet(eth, target_mac, 0x0800); // type = 0x0800 for the IP packet

    /*
        14 bytes for the Ethernet header
            6 bytes for the destination MAC address
            6 bytes for the source MAC address
            2 bytes for the EtherType field
        20 bytes for the IP header
            The IPV4 minimum header length is 20 bytes (0x45)
        8 bytes for the ICMP header
            1 byte for the type
            1 byte for the code
            2 bytes for the checksum
            2 bytes for the identifier
            2 bytes for the sequence number
        20 bytes for the ICMP payload
            20 bytes for the ICMP payload - We choose to put 20 bytes of data inside the ICMP payload, but we can choose any size for the payload
    */
    printf("Echo Request over IP over Ethernet...\n");
    for (int i = 0; i < 14+20+8+20; i++)
        printf("%2.X (%3.d)", buffer[i], buffer[i]);
}

int resolve_ip(unsigned char * target_ip, unsigned char * target_mac) {

    int size;

    int structlen;

    unsigned char buf[1500];

    struct eth_frame * eth;
    struct arp_packet * arp;

    eth = (struct eth_frame *) buf;
    arp = (struct arp_packet *) eth->payload; // or (*eth).payload

    // 0x0806 value of ULP for arp packet
    forge_ethernet(eth, broadcast, 0x0806);

    forge_arp_request(arp, target_ip);

    printf("ARP request prepared...\n");
    for (int i = 0; i < sizeof(struct arp_packet); i++) {
        printf("%2.X (%3.d)", buf[i], buf[i]);
    }

    structlen = sizeof(struct sockaddr_ll);

    // Clean the structure
    for (int i = 0; i < structlen; i++) {
        ((char *)&sll)[i] = 0; // We reset all the bytes to zero
    }

    // Populate the structure
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = if_nametoindex("eth0");

    // ssize_t sendto(
    //          int sockfd,
    //          const void *buf,
    //          size_t len,
    //          int flags,
    //          const struct sockaddr *dest_addr,
    //          socklen_t addrlen
    //                );
    size = sendto(s, buf, 14 + sizeof(struct arp_packet), 0, (struct sockaddr *) &sll, structlen);

    // ssize_t recvfrom(
    //          int sockfd,
    //          void *buf, ---> An array passed without [] is a pointer
    //          size_t len, ---> Size of the buffer
    //          int flags, ---> Forget about it
    //          struct sockaddr *src_addr, ---> Generic struct sockaddr
    //          socklen_t *addrlen ---> Length of the socket passed as a pointer, it could be possible that the size of the structure may be changed by the function, some fields (we don't use) can be modified from the function
    //                );
    //size = recvfrom(s, buf, 1500, 0, (struct sockaddr *)&sll, &structlen);

    for (int j = 0; j < 1000; j++) {
        size = recvfrom (s, buf, 1500, 0, (struct sockaddr *)&sll, &structlen);
        if (size == -1) {
            perror("Recvfrom Failed");
            return -1;
        }
        if (eth->type == htons(0x0806)) {
            // When we are inside this if body we are the arp layer, and we are receiving only arp packet
            if (arp->op == htons(0x0002)) {
                printf("ARP response received...\n");
                for (int i = 0; i < size; i++) {
                    printf("%2.X (%3.d)", buf[i], buf[i]);
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