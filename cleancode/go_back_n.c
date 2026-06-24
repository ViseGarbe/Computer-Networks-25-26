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
#define N 4 // Define a constant for the window size (number of packets that can be sent without acknowledgment)

// Define a structure to represent a packet in the buffer
struct bufelem {
    int id; // Packet identifier
    int seq; // Packet sequence number, the sequence number is in the range [0, N] and wraps around after reaching N
} pktbuf[N];

int mytime = 0; // Variable to keep track of the time elapsed since the last packet was sent
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

    printf("Sending pkt: \tid = %d\tseq = %d\n", pkt->id, pkt->seq);

    seq = (seq + 1) % (N + 1); // Increment the sequence number and wrap around after reaching N

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

    int t = 1; // Variable to hold the number of bytes read from stdin, initialized to 1 to enter the loop
    
    int recv_ack; // Variable to hold the acknowledgment received from stdin

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
                // Non viene rimosso solo quel pacchetto, ma quel pacchetto e tutti i precedenti ancora nella finestra.
                if (pktbuf[(winstart + i) % N].seq == recv_ack) {
                    printf("Dequeuing %d packets\n", i + 1);
                    winstart = (winstart + i + 1) % N;
                    winsize -= (i + 1);
                    mytime = 0;
                    break;
                }
            }
        }
    } 
}

void mytimer (int signal) { 
    printf("Timer handler called\n"); 
    if (++mytime < TIMEOUT)
        return; // Timer not expired yet

    mytime = 0; 

    // Se alla scadenza del timer non è arrivato alcun ACK, si ritrasmettono tutti i pacchetti ancora nella finestra.
    for (int i = 0; i < winsize; i++) {
        printf("Resending pkt: \tid = %d\tseq = %d\n", 
                pktbuf[(winstart + i) % N].id, 
                pktbuf[(winstart + i) % N].seq);
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
/*
OUTPUT:
Sending pkt: 	id = 0	seq = 0
Sending pkt: 	id = 1	seq = 1
Sending pkt: 	id = 2	seq = 2
Sending pkt: 	id = 3	seq = 3

Timer handler called
Timer handler called
Timer handler called

Resending pkt: 	id = 0	seq = 0
Resending pkt: 	id = 1	seq = 1
Resending pkt: 	id = 2	seq = 2
Resending pkt: 	id = 3	seq = 3

0 <- Cause the I/O handler calling

I/O handler called
Dequeuing 1 packets
Sending pkt: 	id = 4	seq = 4

Timer handler called
Timer handler called
Timer handler called

Resending pkt: 	id = 1	seq = 1
Resending pkt: 	id = 2	seq = 2
Resending pkt: 	id = 3	seq = 3
Resending pkt: 	id = 4	seq = 4

Timer handler called

4 <- Cause the I/O handler calling

Timer handler called

I/O handler called
Dequeuing 4 packets <- Becuause it recieved ack for id 4

Sending pkt: 	id = 5	seq = 0 
Sending pkt: 	id = 6	seq = 1
Sending pkt: 	id = 7	seq = 2
Sending pkt: 	id = 8	seq = 3

Timer handler called
Timer handler called
Timer handler called

Resending pkt: 	id = 5	seq = 0
Resending pkt: 	id = 6	seq = 1
Resending pkt: 	id = 7	seq = 2
Resending pkt: 	id = 8	seq = 3

Timer handler called
Timer handler called
Timer handler called

Resending pkt: 	id = 5	seq = 0
Resending pkt: 	id = 6	seq = 1
Resending pkt: 	id = 7	seq = 2
Resending pkt: 	id = 8	seq = 3

^C <- To end the program
*/