#include <sys/types.h>
#include <errno.h> // man 3 perror
#include <arpa/inet.h> //used for htons (from Big Endian to Little Endian)
#include <stdio.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <net/if.h>
#include <poll.h>  // Include the poll.h header for poll-related functions and structures
#include <signal.h> // Include the signal.h header for signal handling functions and structures
#include <fcntl.h> 
#include <sys/time.h> // Include the sys/time.h header for timer-related functions and structures
#include <unistd.h> 

/*
For the ARP Cache
*/
#define ARP_FREE 'F'
#define ARP_PENDING 'P'
#define ARP_RESOLVED 'R'

#define ARP_CACHE_SIZE 8
#define ARP_ENTRY_TTL 30 // Time to live of an ARP cache entry in seconds

/*
For the Environment
*/
#define n_desc 1 // Define a constant for the number of file descriptors to monitor
#define timeout 0 // Define a constant for the timeout value in milliseconds (0 means return immediately)
#define bufsize 100 // Define a constant for the size of the data buffer

unsigned char databuf[bufsize]; // Buffer to store data read from stdin
int bufstart = 0; 

/*
Configuration of the node
*/ 
unsigned char my_mac[6] = {0x22, 0x00, 0xb6, 0xec, 0x6a, 0x3c}; // ifconfig to obtain the real mac
unsigned char broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Default
unsigned char my_ip[4] = {139, 162, 197, 68};

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

/*
#define ARP_FREE 'F'
#define ARP_PENDING 'P'
#define ARP_RESOLVED 'R'
*/
struct arp_cache_entry {
    int state;
    unsigned char ip[4];
    unsigned char mac[6];
    int ttl;
    int retry_timer;
} ace[ARP_CACHE_SIZE]; // ARP cache with a fixed size of 8 entries

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

// IP address to resolve
unsigned char target_ip[4] = {139, 162, 197, 70}; 

// Buffer to store the resolved MAC address
unsigned char target_mac[6]; 

// To use the poll function
struct sigaction sa_timer;
struct sigaction sa_io; 

int resolve_ip (unsigned char * target_ip, unsigned char * target_mac); 

void init_cache();

int cache_lookup(unsigned char * ip);

int set_cache_mac(unsigned char * ip, unsigned char * mac);

int get_free_cache_entry(); 

void print_cache_entries();

void myio (int signal); 

void mytimer (int signal);

int main() {

    // Socket creation
    s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    // Check the creation of the socket
    if (s == -1) {
        perror("Socket Failed");
        return -1;
    }

    /*
    ===================================================
                        ENVIRONMENT
    ===================================================
    */
    sigset_t mask; // Signal set to block signals during critical sections

    int fd = 0; // File descriptor for standard input (stdin) is 0
    int flags = 0; // Variable to hold file status flags
    int t; // Variable to hold the number of bytes read from stdin

    struct itimerval itime; // Structure to specify the timer interval and initial expiration

    sa_timer.sa_handler = mytimer;
    sa_timer.sa_flags = 0;
    sigemptyset(&sa_timer.sa_mask);
    sigaddset(&sa_timer.sa_mask, SIGIO);

    sa_io.sa_handler = myio;
    sa_io.sa_flags = 0;
    sigemptyset(&sa_io.sa_mask);
    sigaddset(&sa_io.sa_mask, SIGALRM);

    if (-1 == sigaction(SIGIO, &sa_io, NULL)) {
        perror("sigaction SIGIO failed");
        return 1;
    }

    if (-1 == sigaction(SIGALRM, &sa_timer, NULL)) {
        perror("sigaction SIGALRM failed");
        return 1;
    }

    if (-1 == fcntl(s, F_SETOWN, getpid())) {
        perror("fcntl F_SETOWN failed");
        return 1;
    }

    flags = fcntl(s, F_GETFL);
    if (flags == -1) {
        perror("fcntl F_GETFL failed");
        return -1;
    }

    if (-1 == fcntl(s, F_SETFL, flags | O_ASYNC | O_NONBLOCK)) {
        perror("fcntl F_SETFL failed");
        return -1;
    }

    itime.it_interval.tv_sec = 1; // Set the timer to expire every 1 second
    itime.it_interval.tv_usec = 0; // Set the initial expiration of the timer to 0 microseconds
    itime.it_value.tv_sec = 1; // Set the initial expiration of the timer to 1 second
    itime.it_value.tv_usec = 0; // Set the initial expiration of the timer to 0 microseconds

    if (-1 == setitimer(ITIMER_REAL, &itime, NULL)) {
        perror("setitimer failed");
        return -1;
    }

    if (-1 == sigemptyset(&mask)) {
        perror("sigemptyset failed");
        return 1;
    }

    if (-1 == sigaddset(&mask, SIGIO)) {
        perror("sigaddset SIGIO failed");
        return 1;
    }
    
    if (-1 == sigaddset(&mask, SIGALRM)) {
        perror("sigaddset SIGALRM failed");
        return 1;
    }


    /*
    ===================================================
                        CACHE
    ===================================================
    */

    init_cache(); 

    if (target_ip == NULL || target_mac == NULL) { 
        printf("Invalid input parameters for resolve_ip\n");
        return -1;
    }

    if (cache_lookup(target_ip) == -1) { // Cache miss
        if (resolve_ip(target_ip, target_mac) == -1) {
            return -1;
        }
    }

    printf("\nResolved MAC is...\n");
    for (int i = 0; i < 6; i++) {
        printf("%.2X (%.3d) ", target_mac[i], target_mac[i]);
    }
    printf("\n");

    return 0; 
}

/*
Return values: 
    -> -1: Sendto/Recvfrom Failed
    ->  1: Operation Successful
*/
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

    printf("\n\nSending ARP request for IP address...\n");
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

    while (1) {
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
                for (int j = 0; j < size; j++) {
                    printf("%.2X (%.3d) ", buf[j], buf[j]);
                    count = (count == 8) ? 1 : count + 1;
                    if (count == 8)
                        printf("\n");
                }
                printf("\n");

                int match = 1;
                for (int j = 0; j < 4; j++) {
                    if (arp->dst_ip[j] != my_ip[j] || arp->src_ip[j] != target_ip[j]) { 
                        match = 0;
                        break;
                    }
                }
                
                if (match) {
                    for (int j = 0; j < 6; j++) {
                        target_mac[j] = arp->src_mac[j]; // Set the target MAC address with the source MAC address of the ARP reply
                    }

                    // We set the cache entry with the source IP and the source MAC address of the ARP reply                     
                    if (set_cache_mac(arp->src_ip, arp->src_mac) == -1) {
                        printf("Warning: MAC resolved but cache is full\n");
                    }
                    
                    return 1; 
                }
            }
        }
    }
}

/*
===================================================
                    CACHE
===================================================
*/

/*
Return values:
    -> -1: No matching entry found
    -> index of the entry in the cache where the IP-MAC mapping is stored (if the entry already exists)
*/
int cache_lookup(unsigned char * ip) {

    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if(ace[i].state == ARP_FREE || ace[i].state == ARP_PENDING) {
            continue;
        }

        int match = 1;

        for (int j = 0; j < 4; j++) {
            if (ace[i].ip[j] != ip[j]) {
                match = 0;
                break;
            }
        }

        if (match) {
            for (int j = 0; j < 6; j++) {
                target_mac[j] = ace[i].mac[j]; // Set the target MAC address with the one found in the cache
            }
            return i; // Return the index of the matching entry
        }
    }

    return -1; // Return -1 if no matching entry is found
}

/*
Return values:
    -> -1: No free entry found
    -> index of the entry in the cache where the IP-MAC mapping is stored (if the entry already exists or if a new entry is created)
*/
int set_cache_mac(unsigned char * ip, unsigned char * mac) {
    /* 
    Cerco di capire se l'entry i-esima della cache corrisponde all'IP passato come parametro, 
    se si aggiorno il MAC address e lo stato dell'entry, altrimenti passo all'entry successiva 
    */

    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (ace[i].state == ARP_PENDING || ace[i].state == ARP_RESOLVED) {
        
            int match = 1;

            for (int j = 0; j < 4; j++) {

                if (ace[i].ip[j] != ip[j]) {
                    match = 0;
                    break;
                }
                
            }

            // Se l'entry i-esima della cache corrisponde all'IP passato come parametro, aggiorno il MAC address e lo stato dell'entry
            if (match) {

                for (int j = 0; j < 6; j++) {
                    ace[i].mac[j] = mac[j]; // Set the cache mac address with the one passed as parameter
                }

                ace[i].state = ARP_RESOLVED;

                ace[i].ttl = ARP_ENTRY_TTL; // Reset the TTL of the entry

                ace[i].retry_timer = 0; // Reset the retry timer of the entry

                printf("\n\t===== DEBUG =====\n");
                print_cache_entries();
                printf("\n\t===== DEBUG =====\n");
                
                return i; // Return the index of the matching entry
            }
        }
    }

    /* 
    Se non esiste un'entry corrispondente all'IP passato come parametro, cerco un'entry libera per creare una nuova entry 
    con l'IP e il MAC address passati come parametro 
    */

    int index = get_free_cache_entry();

    if (index != -1) {

        ace[index].state = ARP_RESOLVED;

        for (int i = 0; i < 4; i++) {
            ace[index].ip[i] = ip[i];
        }

        for (int i = 0; i < 6; i++) {
            ace[index].mac[i] = mac[i];
        }

        ace[index].ttl = ARP_ENTRY_TTL; 

        ace[index].retry_timer = 0;
    }

    return index; // Return the index of the entry or -1 if no free entry is empty
}

int get_free_cache_entry() {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (ace[i].state == ARP_FREE) {
            return i; // Return the index of the free entry
        }
    }

    return -1; // Return -1 if no free entry is found
}

/*
struct arp_cache_entry {
    int state;
    unsigned char ip[4];
    unsigned char mac[6];
    int ttl;
    int retry_timer;
};
*/
void print_cache_entries() {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        printf("\nEntry: %d", i); 
        printf("\n\tState: %c", ace[i].state);
        for (int j = 0; j < 4; j++) {
            printf("\n\tIP: %d", ace[i].ip[j]);
        }
        for (int j = 0; j < 6; j++) {
            printf("\n\tMAC: %02X", ace[i].mac[j]);
        }
        printf("\n\tRetry Timer: %ds", ace[i].retry_timer);
    }
}

// Reset the ARP cache entries to the initial state
void init_cache() {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        ace[i].state = ARP_FREE; 

        for (int j = 0; j < 4; j++) {
            ace[i].ip[j] = 0;
        }

        for (int j = 0; j < 6; j++) {
            ace[i].mac[j] = 0;
        }

        ace[i].ttl = 0;

        ace[i].retry_timer = 0;
    }
}

/*
===================================================
                    ENVIRONMENT
===================================================
*/

void myio (int signal) { 
    printf("IO handler called\n"); 
}

void mytimer (int signal) { 
    printf("Timer handler called\n"); 
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (ace[i].state == ARP_FREE) // If the packet has already been acknowledged, skip it
            continue;

        if (++ace[i].retry_timer < ace[i].ttl) // Increment the timer for the packet and check if it has reached the timeout value
            continue;

        ace[i].retry_timer = 0;
        printf("Resending pkt"); 
    }
}