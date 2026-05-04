#include <poll.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#define TIMEOUT 3 // Define a constant for the timeout value in seconds
#define n_desc 1 // Define a constant for the number of file descriptors to monitor
#define timeout 0 // Define a constant for the timeout value in milliseconds (0 means return immediately)
#define N 5 // Define a constant for the window size (number of packets that can be sent without acknowledgment)
#define SEQSPACE (2 * N) // In Selective Repeat the sequence space must be at least twice the window size, to avoid ambiguity between new packets and retransmissions of old packets

// Define a structure to represent a packet in the buffer
struct bufelem {
    int id; // Packet identifier
    int seq; // Packet sequence number
    int acked; // 1 if the packet has already been acknowledged
    int timer; // Per-packet timer, incremented at each tick
} pktbuf[N];

int winstart; // Variable to keep track of the start of the sliding window
int winsize; // Variable to keep track of the size of the sliding window
int seq = 0; // Variable to keep track of the sequence number

int pktsend(int id) {
    // Wait until there is space in the window to send a new packet
    while (winsize == N)
        pause(); 

    // Get a pointer to the packet in the buffer where the new packet will be stored
    struct bufelem *pkt = &pktbuf[(winstart + winsize) % N];

    pkt->id = id; // Set the packet identifier
    pkt->seq = seq; // Set the packet sequence number
    pkt->acked = 0; // The packet is still waiting for its ACK
    pkt->timer = 0; // The timeout starts from this send

    printf("Sending pkt: \tid = %d\tseq = %d\n", pkt->id, pkt->seq);

    seq = (seq + 1) % SEQSPACE; // Increment the sequence number and wrap around in the Selective Repeat sequence space

    winsize++; // Increment the size of the sliding window

    return 1; 
}

void myio (int signal) { 
    /*
    The set of file descriptors to be monitored is specified in the
    fds argument, which is an array of structures of the following
    form:

        struct pollfd {
            int   fd;         // file descriptor 
            short events;     // requested events 
            short revents;    // returned events 
        };

    poll() 
        performs a similar task to select(2): it waits for one of a
        set of file descriptors to become ready to perform I/O. 
    */
    struct pollfd p[n_desc];

    int recv_ack; // Variable to hold the acknowledgment received from stdin

    int found = 0;

    printf("I/O handler called\n"); 

    p[0].fd = 0; //stdin 
    p[0].events = POLLIN; // POLLIN There is data to read.
    p[0].revents = 0; 

    // # file descriptor = 1 
    // timeout = 0 ms (return immediately)
     if (-1 == poll(p, n_desc, timeout)) {
        perror("Poll failed\n");
        return;
    }

    /*
    revents non contiene “un solo evento restituito”, ma un insieme di flag. Quindi può succedere che:
        p[0].events  = POLLIN;
        p[0].revents = POLLIN | POLLHUP;
    Per questo la scelta giusta è ragionare a bitmask
    */
    if (p[0].revents & POLLIN) {
        if (scanf("%d", &recv_ack)) {
            for (int i = 0; i < winsize; i++) {
                struct bufelem *pkt = &pktbuf[(winstart + i) % N];

                if (pkt->seq == recv_ack) {
                    found = 1;
                    if (pkt->acked) {
                        printf("Duplicate ACK ignored for seq = %d\n", recv_ack);
                    } else {
                        pkt->acked = 1;
                        pkt->timer = 0;
                        printf("ACK received for pkt: \tid = %d\tseq = %d\n",
                               pkt->id, pkt->seq);
                        while (winsize > 0 && pktbuf[winstart].acked) {
                            printf("Sliding window over pkt: id = %d\tseq = %d\n",
                                pktbuf[winstart].id, pktbuf[winstart].seq);
                            
                            pktbuf[winstart].id = 0;
                            pktbuf[winstart].seq = 0;
                            pktbuf[winstart].acked = 0;
                            pktbuf[winstart].timer = 0;
                            
                            winstart = (winstart + 1) % N;
                            winsize--;
                        }
                    }
                    break;
                } 
            }

            if (!found)
                printf("ACK %d ignored: packet not in current window\n", recv_ack);
        }
    } 
}

void mytimer (int signal) { 
    printf("Timer handler called\n"); 
    // In Selective Repeat ogni pacchetto ha il proprio timeout.
    for (int i = 0; i < winsize; i++) {
        struct bufelem *pkt = &pktbuf[(winstart + i) % N];

        if (pkt->acked) // If the packet has already been acknowledged, skip it
            continue;

        if (++pkt->timer < TIMEOUT) // Increment the timer for the packet and check if it has reached the timeout value
            continue;

        pkt->timer = 0;
        printf("Resending pkt: \tid = %d\tseq = %d\n", pkt->id, pkt->seq);
    }
}

struct sigaction sa_timer;
struct sigaction sa_io;

int main() {
    sigset_t mask;

    int s = 0; // File descriptor for standard input (stdin) is 0

    int flags = 0; // Variable to hold file status flags

    int id = 0; // Variable to keep track of the packet identifier, initialized to 0 and incremented for each new packet sent

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
    itime.it_interval.tv_usec = 0; // Set the initial expiration of the timer
    itime.it_value.tv_sec = 1; // Set the initial expiration of the timer to 1 second
    itime.it_value.tv_usec = 0; // Set the initial expiration of the timer

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

    if (-1 == sigprocmask(SIG_UNBLOCK, &mask, NULL)) {
        perror("sigprocmask failed");
        return 1;
    }
    
    while (1) {
        while(pktsend(id))
            id++;
    }
}
