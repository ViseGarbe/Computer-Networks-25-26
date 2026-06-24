/*

The IP datagram built by the program ping.c developed in class includes a header with the minimum possible length (20 characters), i.e., with no optional fields. The IP protocol allows extending the IP header, adding some optional fields that allow requesting or sending additional information to the network nodes through which the packet passes. We now intend to explore this possibility.

Modify it, in such a way that it constructs the IP datagram by adding the optional "Record Route" field to the IP header (described in RFC 791, see below). This field reserves a free area (called route data) inside the extended IP header, intended to contain the list of IP addresses of the nodes crossed by the packet. Each node crossed, in fact, in the presence of the "Record Route" option, should append its IP address in the area route data to the IP addresses list already saved by the previous nodes. The fields length and pointer allow you to manage the memory area during the packet trip, as reported in the text taken from RFC 791:

Record Route

        + -------- + -------- + -------- + --------- // -------- +
        | 00000111 | length | pointer | route data |
        + -------- + -------- + -------- + --------- // -------- +
          Type = 7

        The record route option provides a means to record the route of
        an internet datagram.

        The option begins with the option type code. The second octet
        is the option length which includes the option type code and the
        length octet, the pointer octet, and length - 3 octets of route
        data. The third octet is the pointer into the route data
        indicating the octet which begins the next area to store a route
        address. The pointer is relative to this option, and the
        smallest legal value for the pointer is 4.

        A recorded route is composed of a series of internet addresses.
        Each internet address is 32 bits or 4 octets. If the pointer is
        greater than the length, the recorded route data area is full.
        The originating host must compose this option with a large
        enough route data area to hold all the address expected. The
        size of the option does not change due to adding addresses. The
        title contents of the route data area must be zero.

        When an internet module routes a datagram it checks to see if
        the record route option is present. If it is, it inserts its
        own internet address as known in the environment into which this
        datagram is being forwarded into the recorded route begining at
        the octet indicated by the pointer, and increments the pointer
        by four.

        If the route data area is already full (the pointer exceeds the
        length) the datagram is forwarded without inserting the address
        into the recorded route. If there is some room but not enough
        room for a full address to be inserted, the original datagram is
        considered to be in error and is discarded. In either case an
        ICMP parameter problem message may be sent to the source host.


The candidate will know that he has well formed the extended IP header,
if he verifies that the echo reply returnfrom the destination will bring
the option "Record Route" in the IP header containing in the "route data"
field the list of IP addresses of the nodes traversed during the echo request process.

 For the purpose of carrying out the task, also keep in mind the following:
Reserve the maximum possible size of bytes in this field, taking into account
all the constraints of the IP standard.
Send the packet to node 147.162.2.100
If the option is not formed correctly, the first intermediate node that recognizes
an error sends to our node an ICMP type 12 message (0xC) described by RFC792 (see below)

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Type      |     Code      |          Checksum             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    Pointer    |                   unused                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      Internet Header + 64 bits of Original Data Datagram      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   IP Fields:

   Destination Address

      The source network and address from the original datagram's data.

   ICMP Fields:

   Type

      12

   Code

      0 = pointer indicates the error.

   Checksum

      The checksum is the 16-bit ones's complement of the one's
      complement sum of the ICMP message starting with the ICMP Type.
      For computing the checksum, the checksum field should be zero.
      This checksum may be replaced in the future.

   Pointer

      If code = 0, identifies the octet where an error was detected.

   Internet Header + 64 bits of Data Datagram

      The internet header plus the first 64 bits of the original
      datagram's data. This data is used by the host to match the
      message to the appropriate process. If a higher level protocol
      uses port numbers, they are assumed to be in the first 64 data
      bits of the original datagram's data.

   Description

      If the gateway or host processing a datagram finds a problem with
      the header parameters such that it cannot complete processing the
      datagram it must discard the datagram. One potential source of
      such a problem is with incorrect arguments in an option. The
      gateway or host may also notify the source host via the parameter
      problem message. This message is only sent if the error caused
      the datagram to be discarded.

      The pointer identifies the octet of the original datagram's header
      where the error was detected (it may be in the middle of an
      option). For example, 1 indicates something is wrong with the
      Type of Service, and (if there are options present) 20 indicates
      something is wrong with the type code of the first option.
*/

#include <sys/types.h>
#include <errno.h>     // man 3 perror
#include <arpa/inet.h> //used for htons (from Big Endian to Little Endian)
#include <stdio.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <net/if.h>

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
unsigned char netmask[4] = {255, 255, 255, 0}; // ifconfig to obtain the netmask, /sbin/ifconfig
unsigned char gateway[4] = {139, 162, 197, 1};

/*
Destination MAC address: 6 bytes
Source MAC address: 6 bytes
EtherType: 2 bytes
Payload: 46-1482 bytes
*/
struct eth_frame
{
    unsigned char dst[6];     // Destination MAC address
    unsigned char src[6];     // Source MAC address
    unsigned short type;      // EtherType field to indicate the protocol of the payload (e.g., 0x0800 for IP, 0x0806 for ARP) - aka ULP field
    unsigned char payload[1]; // We are interested only on the first byte
};

void forge_ethernet(struct eth_frame *eth, unsigned char *dest_addr, unsigned short type)
{
    for (int i = 0; i < 6; i++)
    {
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
struct arp_packet
{
    unsigned short htype;     // Hardware type
    unsigned short ptype;     // Protocol type
    unsigned char hlen;       // Hardware address length
    unsigned char plen;       // Protocol address length
    unsigned short op;        // Opcode
    unsigned char src_mac[6]; // Sender MAC address
    unsigned char src_ip[4];  // Sender IP address
    unsigned char dst_mac[6]; // Target MAC address
    unsigned char dst_ip[4];  // Target IP address
};

void forge_arp_request(struct arp_packet *arp, unsigned char *dest_ip)
{
    arp->htype = htons(1);      // Hardware type: Ethernet = 1, we must swap the bytes because the field is in big endian, but we are in little endian
    arp->ptype = htons(0x0800); // Protocol type: IP = 0x0800, ARP = 0x0806
    arp->hlen = 6;              // Hardware address length: 6 bytes for Ethernet
    arp->plen = 4;              // Protocol address length: 4 bytes for IP
    arp->op = htons(1);         // Opcode: REQUEST = 1, REPLY = 2

    for (int i = 0; i < 6; i++)
    {
        arp->src_mac[i] = my_mac[i]; // Set the sender MAC address of the ARP request with our MAC address
    }
    for (int i = 0; i < 4; i++)
    {
        arp->src_ip[i] = my_ip[i]; // Set the sender IP address of the ARP request with our IP address
    }
    for (int i = 0; i < 6; i++)
    {
        arp->dst_mac[i] = 0; // Set the target MAC address of the ARP request to 00:00:00:00:00:00, because we don't know it yet
    }
    for (int i = 0; i < 4; i++)
    {
        arp->dst_ip[i] = dest_ip[i]; // Set the target IP address of the ARP request with the destination IP address passed as parameter
    }
}

/*
The checksum algorithm is:

    The checksum field is the 16 bit one's complement of the one's
    complement sum of all 16 bit words in the header.  For purposes of
    computing the checksum, the value of the checksum field is zero.
*/
unsigned short checksum(unsigned char *b, int len)
{ // Buffer and length of the buffer
    unsigned short *p = (unsigned short *)b;
    unsigned short tot = 0;
    unsigned short prev = 0;
    for (int i = 0; i < len / 2; i++)
    {
        tot += ntohs(p[i]); // We swap the bytes, header is in big endian, but we are in little endian
        if (prev > tot)     // If the sum is less than the previous value, we have an overflow, we must add 1 to the sum
            tot++;
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
struct ip_datagram
{
    unsigned char ver_ihl;     // Version and Internet Header Length - 4bits + 4 bits -> char
    unsigned char tos;         // Type of Service
    unsigned short totlen;     // Total Length of the datagram (header + payload)
    unsigned short id;         // Identification
    unsigned short flags_offs; // Flags and Fragment Offset - 3bits + 13bits -> short
    unsigned char ttl;         // Time To Live
    unsigned char proto;       // Upper layer protocol
    unsigned short checksum;   // Header checksum
    unsigned int srcip;        // Source IP address
    unsigned int dstip;        // Destination IP address

    // New data
    unsigned char opt;            // Record Route option type
    unsigned char length;         // Record Route option length
    unsigned char pointer;        // Pointer to the next free route-data byte
    unsigned char route_data[36]; // Space for 9 IPv4 addresses
    unsigned char padding;        // Padding to make the header a multiple of 4 bytes

    unsigned char payload[1]; // We are interested only on the first byte, we can put data inside the payload
};

void forge_ip(struct ip_datagram *ip, char *target_ip, char protocol, unsigned short payload_len)
{
    ip->ver_ihl = 0x4F; // Version: IPv4, IHL: 15 (maximum header length: 60 bytes)
    ip->tos = 0;
    ip->totlen = htons(60 + payload_len); // 60 bytes: maximum header length, payload_len: length of the payload
    ip->id = htons(0x1234);               // Identification, it can be random
    ip->flags_offs = htons(0);            // Now it is 0x0000, May fragment, last fragment, offset 0
    ip->ttl = 128;                        // Time to live - normal value 128
    ip->proto = protocol;                 // Protocol (e.g. TCP, UDP, ICMP, ecc...) upper layer protocol we use ICMP : 1

    // The checksum is part of the packet
    // The value of the checksum is zero, before computing it
    ip->checksum = htons(0); // Must be calculated

    // Set the source IP address - my_ip
    ip->srcip = *(unsigned int *)my_ip;

    // Set the destination IP address
    ip->dstip = *(unsigned int *)target_ip;

    ip->opt = 7;
    /*
        1 byte type
        1 byte length
        1 byte pointer
        36 byte for 9 IPV4 address
    */
    ip->length = 39;
    /* Next free space
        1       2       3       4...
        type | length | pointer | route_data
    */
    ip->pointer = 4;
    for (int i = 0; i < 36; i++)
        ip->route_data[i] = 0;
    ip->padding = 0;

    // We overwrite the checksum as the rfc says
    ip->checksum = htons(checksum((unsigned char *)ip, 60));
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
struct icmp_message
{
    unsigned char type;       // Type of the ICMP message
    unsigned char code;       // Code of the ICMP message
    unsigned short checksum;  // Checksum of the ICMP message
    unsigned short id;        // Identifier of the ICMP message
    unsigned short seq;       // Sequence number of the ICMP message
    unsigned char payload[1]; // Payload of the ICMP message
};

void forge_icmp(struct icmp_message *icmp)
{
    icmp->type = 8;           // Echo request message, for echo reply message we must set it to 0
    icmp->code = 0;           // No need for icmp
    icmp->checksum = 0;       // Overwrite it later;
    icmp->id = htons(0xABCD); // Identifier, it can be random, but it is common to use 0 if code is 0
    icmp->seq = htons(1);     // Sequence number, it can be random, but it is common to use 0 if code is 0

    // Why we should put some data inside a ping request? Which data we have to put inside?
    // In general: if we want to test the nwtwork we must try different size of the packet, test round trip time, fragmentation ecc...
    // The possibility to send data that we can fully describe
    for (int i = 0; i < 20; i++)
        icmp->payload[i] = i;

    icmp->checksum = htons(checksum((unsigned char *)icmp, 8 + 20));
}

// Socket address structure for the link layer (Ethernet)
struct sockaddr_ll sll;

// Socket descriptor
int s;

// Counter for printing the bytes of the packet
int count = 0;

int resolve_ip(unsigned char *target_ip, unsigned char *target_mac);

int main()
{
    unsigned char target_ip[4] = {147, 162, 2, 100};
    unsigned char target_mac[6];
    unsigned char buffer[1500];

    // Size of the packet sent or received
    int size;

    // Length of the sockaddr_ll structure
    int structlen;

    struct eth_frame *eth = (struct eth_frame *)buffer;
    struct ip_datagram *ip = (struct ip_datagram *)eth->payload;
    struct icmp_message *icmp = (struct icmp_message *)ip->payload;

    forge_icmp(icmp);
    /*
        Protocol = 1 = ICMP,
        payload_length = 8 + 20
            8 bytes for the ICMP header
            20 bytes for the ICMP payload
    */
    forge_ip(ip, target_ip, 1, 8 + 20);

    // Compare my_ip masked with the target_ip masked and if they are equal we are in the same network
    if (((*(unsigned int *)target_ip) & (*(unsigned int *)netmask)) == ((*(unsigned int *)my_ip) & (*(unsigned int *)netmask)))
    {
        resolve_ip(target_ip, target_mac);
    }
    else
    {
        resolve_ip(gateway, target_mac);
    }

    printf("\n\nResolved MAC is...\n");

    for (int i = 0; i < 6; i++)
        printf("%.2X (%.3d) ", target_mac[i], target_mac[i]);
    printf("\n");

    forge_ethernet(eth, target_mac, 0x0800); // type = 0x0800 for the IP packet

    /*
        14 bytes for the Ethernet header
            6 bytes for the destination MAC address
            6 bytes for the source MAC address
            2 bytes for the EtherType field
        60 bytes for the IP header
            20 bytes for the fixed header
            39 bytes for the Record Route option
            1 byte for EOL/padding
        8 bytes for the ICMP header
            1 byte for the type
            1 byte for the code
            2 bytes for the checksum
            2 bytes for the identifier
            2 bytes for the sequence number
        20 bytes for the ICMP payload
            20 bytes for the ICMP payload - We choose to put 20 bytes of data inside the ICMP payload, but we can choose any size for the payload
    */
    printf("\n\nEcho Request over IP over Ethernet...\n");
    count = 0;
    for (int i = 0; i < 14 + 60 + 8 + 20; i++)
    {
        printf("%.2X (%.3d) ", buffer[i], buffer[i]);
        count = (count == 8) ? 1 : count + 1;
        if (count == 8)
            printf("\n");
    }
    printf("\n");

    structlen = sizeof(struct sockaddr_ll);

    for (int i = 0; i < structlen; i++)
        ((char *)&sll)[i] = 0; // We reset all the bytes to zero

    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = if_nametoindex("eth0");

    // Socket creation
    s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    // Check the creation of the socket
    if (s == -1)
    {
        perror("Socket Failed");
        return -1;
    }

    // man 2 sendto
    size = sendto(s, buffer, 14 + 60 + 8 + 20, 0, (struct sockaddr *)&sll, structlen);

    if (size == -1)
    {
        perror("Sendto Failed");
        return -1;
    }

    for (int i = 0; i < structlen; i++)
        ((char *)&sll)[i] = 0; // We reset all the bytes to zero

    for (int i = 0; i < 1000; i++)
    {
        // man 2 recvfrom
        size = recvfrom(s, buffer, 1500, 0, (struct sockaddr *)&sll, &structlen);
        if (size == -1)
        {
            perror("Recvfrom Failed");
            return -1;
        }
        if (eth->type == ntohs(0x0800))
        {
            // When we are inside this if body we are in the ip layer, and we are receiving only ip packet
            if (ip->proto == 1)
            {
                // When we are inside this if body we are in the icmp layer, and we are receiving only icmp packet
                if (icmp->type == 0)
                {
                    printf("\n\nEcho Reply received...\n");
                    count = 0;
                    for (int j = 0; j < size; j++)
                    {
                        printf("%.2X (%.3d) ", buffer[j], buffer[j]);
                        count = (count == 8) ? 1 : count + 1;
                        if (count == 8)
                            printf("\n");
                    }
                    printf("\n");
                    return 0;
                }
            }
        }
    }
}

int resolve_ip(unsigned char *target_ip, unsigned char *target_mac)
{
    printf("\nResolving IP address...\n");

    // Size of the packet sent or received
    int size;

    // Length of the sockaddr_ll structure
    int structlen;

    // Buffer for the packet
    unsigned char buf[1500];

    struct eth_frame *eth = (struct eth_frame *)buf;
    struct arp_packet *arp = (struct arp_packet *)eth->payload; // or (*eth).payload

    // 0x0806 value of ULP for arp packet
    forge_ethernet(eth, broadcast, 0x0806);

    // Forge the ARP request
    forge_arp_request(arp, target_ip);

    printf("\nARP request prepared...\n");
    count = 0;
    // Why 14? Because the Ethernet header is 14 bytes, and the arp packet is after the Ethernet header,
    // so we must start to print from the first byte of the Ethernet header, and we must print all the bytes
    // of the Ethernet header and all the bytes of the arp packet, so we must print 14 + sizeof(struct arp_packet) bytes
    for (int i = 0; i < 14 + sizeof(struct arp_packet); i++)
    {
        printf("%.2X (%.3d) ", buf[i], buf[i]);
        count = (count == 8) ? 1 : count + 1;
        if (count == 8)
            printf("\n");
    }
    printf("\n");

    structlen = sizeof(struct sockaddr_ll);

    // Clean the structure
    for (int i = 0; i < structlen; i++)
    {
        ((char *)&sll)[i] = 0; // We reset all the bytes to zero
    }

    // Populate the structure
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = if_nametoindex("eth0");

    // Socket creation
    s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    // Check the creation of the socket
    if (s == -1)
    {
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
    size = sendto(s, buf, 14 + sizeof(struct arp_packet), 0, (struct sockaddr *)&sll, structlen);

    if (size == -1)
    {
        perror("Sendto Failed");
        return -1;
    }

    printf("Sending ARP request (size %d)...\n", size);

    // Clean the structure
    for (int i = 0; i < structlen; i++)
    {
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

    for (int i = 0; i < 1000; i++)
    {
        size = recvfrom(s, buf, 1500, 0, (struct sockaddr *)&sll, &structlen);
        if (size == -1)
        {
            perror("Recvfrom Failed");
            return -1;
        }
        if (eth->type == htons(0x0806))
        {
            // When we are inside this if body we are the arp layer and we are receving only arp packets
            if (arp->op == htons(0x0002))
            {
                // When we are inside this if body we are receiving only ARP reply
                printf("\nARP response received (size = %d)...\n", size);
                count = 0;
                for (int i = 0; i < size; i++)
                {
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
