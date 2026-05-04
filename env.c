#include <poll.h>  // Include the poll.h header for poll-related functions and structures
#include <stdio.h> 
#include <signal.h> // Include the signal.h header for signal handling functions and structures
#include <fcntl.h> 
#include <sys/time.h> // Include the sys/time.h header for timer-related functions and structures
#include <unistd.h> 
#include <errno.h> 

#define n_desc 1 // Define a constant for the number of file descriptors to monitor
#define timeout 0 // Define a constant for the timeout value in milliseconds (0 means return immediately)
#define bufsize 100 // Define a constant for the size of the data buffer

unsigned char databuf[bufsize]; // Buffer to store data read from stdin
int bufstart = 0; 
 
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
    struct pollfd p[n_desc]; // Array of pollfd structures to monitor file descriptors
    
    int t = 1; // Variable to hold the number of bytes read from stdin, initialized to 1 to enter the loop
    
    printf("I/O handler called\n"); 

    p[0].fd = 0; //stdin 
    p[0].events = POLLIN; // POLLIN There is data to read.
    p[0].revents = 0; 

    /*
    n_desc is the number of file descriptors in the array, and timeout is in milliseconds.
    poll() restituisce: 
        > 0: quanti descriptor hanno almeno un evento (nel nostro caso, al massimo 1)
          0: è scaduto il timeout senza eventi
        - 1: errore, con errno impostato
    */
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
        // If there is data to read on stdin, read it into the buffer
        for (bufstart; t; bufstart += t) {  
            /*
            Read data from stdin into the buffer, starting at the current buffer index
            If we had more than 1 file descriptor, we would need to check which one has data to read using 
            a for loop and checking the revents field of each pollfd structure. In this case, we only have 
            one file descriptor (stdin), so we can directly read from it.
            */
            t = read(p[0].fd, databuf + bufstart, bufsize - bufstart); 
            if (t == -1) { 
                /*
                If the error is EAGAIN, it means that there is no more data to read at the moment, and we can exit the loop 
                If the error is EWOULDBLOCK, it means that the file descriptor is in non-blocking mode and there is no data to read, so we can also exit the loop.
                -- I'm not sure because we set t = 0 so we don't increase bufstart, and we stay inside the loop without reading more data
                If the error is not EAGAIN, it means that there was an actual error while reading, and we print an error message.
                */
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                    perror("Read failed"); 
                t = 0;
            } 
        }
    } 
}
 
void mytimer (int signal) { 
    printf("Timer handler called\n"); 
} 
 
/* man 2 sigaction for details on the sigaction structure and its members.
Must include <signal.h> to use the sigaction structure and related functions.
The sigaction structure is defined as something like:

    struct sigaction {
        void     (*sa_handler)(int);
        void     (*sa_sigaction)(int, siginfo_t *, void *);
        sigset_t   sa_mask;
        int        sa_flags;
        void     (*sa_restorer)(void);
    };

*/
struct sigaction sa_timer;
struct sigaction sa_io; 
 
int main() { 

    sigset_t mask; // Signal set to block signals during critical sections

    int s = 0; // File descriptor for standard input (stdin) is 0
    int flags = 0; // Variable to hold file status flags
    int t; // Variable to hold the number of bytes read from stdin

    struct itimerval itime; // Structure to specify the timer interval and initial expiration

    // Registra due handler: myio per SIGIO e mytimer per SIGALRM.
    sa_timer.sa_handler = mytimer; 
    sa_io.sa_handler = myio; 
    
    /*
    SIGIO per dire “c’è qualcosa da leggere su input”

    Registriamo il comportamento del processo quando arriva il segnale SIGIO.
    Ovvero
        SIGIO: il segnale che vogliamo gestire
        &sa_io: la struttura che descrive come gestirlo
        NULL: non ci interessa salvare la vecchia configurazione del segnale
    */
    if (-1 == sigaction(SIGIO, &sa_io, NULL)) {
        perror("SIGIO sigaction failed"); 
        return 1;
    } 

    /*
    SIGALRM per dire “è passato 1 secondo”
    
    Registriamo il comportamento del processo quando arriva il segnale SIGALRM.
    Ovvero
        SIGALRM: il segnale che vogliamo gestire
        &sa_timer: la struttura che descrive come gestirlo
        NULL: non ci interessa salvare la vecchia configurazione del segnale
    */
    if (-1 == sigaction(SIGALRM, &sa_timer, NULL)) {
        perror("SIGALRM sigaction failed"); 
        return 1;
    } 

    // Set the owner of the file descriptor to the current process
    // fcntl - manipulate file descriptor

    /*
    Informiamo il kernel rispetto a quale processo deve inviare i segnali asincroni associati al file descriptor s.
    significa:
        s: file descriptor interessato, qui vale 0, quindi stdin
        F_SETOWN: dice a fcntl() di impostare il “proprietario” del file descriptor

                Set the process ID or process group ID that will receive
                SIGIO and SIGURG signals for events on the file descriptor
                fd.  The target process or process group ID is specified in
                arg.  A process ID is specified as a positive value; a
                process group ID is specified as a negative value.  Most
                commonly, the calling process specifies itself as the owner
                (that is, arg is specified as getpid(2)).

        getpid() restituisce il PID del processo corrente

            #include <unistd.h>
            Returns the process ID (PID) of the calling process.
            (This is often used by routines that generate unique temporary filenames.)

    */
    if (-1 == fcntl(s, F_SETOWN, getpid())) {
        perror("fcntl F_SETOWN failed"); 
        return 1;
    } 
    
    flags = fcntl(s, F_GETFL); // Get the current file status flags. On error, -1 is returned, and errno is set to indicate the error.

    // Check if fcntl succeeded
    if(flags == -1) {
        perror("fcntl F_GETFL failed"); 
        return -1;
    }

    // Set the file status flags to enable asynchronous I/O and non-blocking mode

    /*
    
    O_ASYNC
        Quando il descriptor diventa pronto per I/O, il kernel invia un segnale 
        al processo proprietario nel tuo caso, dopo F_SETOWN, quel segnale è SIGIO

    O_NONBLOCK or O_NDELAY
        read() non resta in attesa se non ci sono dati invece di bloccarsi, fallisce 
        con -1 e errno = EAGAIN o EWOULDBLOCK
    
    Modifica dei flag del file descriptor s per renderlo:
        - asincrono con O_ASYNC
        - non bloccante con O_NONBLOCK

    */

    if ( -1 == fcntl(s, F_SETFL, flags | O_ASYNC | O_NONBLOCK)) { 
        perror("fcntl F_SETFL failed"); 
        return -1;
    }


    /* man 2 getitimer for details on the itimerval structure and its members.
    Must include <sys/time.h> to use the itimerval structure and related functions.
    Timer values are defined by the following structures:
        struct itimerval {
            struct timeval it_interval; // Interval for periodic timer
            struct timeval it_value;    // Time until next expiration
        };

        struct timeval {
            time_t tv_sec;         // seconds
            suseconds_t tv_usec;        // microseconds
        };

    La struttura ha due parti:
        it_value: tra quanto tempo deve scattare la prossima scadenza
        it_interval: ogni quanto tempo deve ripetersi dopo la prima scadenza

    Ognuna delle due è una struct timeval, quindi ha:
        tv_sec: secondi
        tv_usec: microsecondi
    */

    itime.it_interval.tv_sec = 1; // Set the timer to expire every 1 second
    itime.it_interval.tv_usec = 0; // Set the initial expiration of the timer to 0 seconds
    itime.it_value.tv_sec = 1; // Set the initial expiration of the timer to 1 second
    itime.it_value.tv_usec = 0; // Set the initial expiration of the timer to 0 microseconds

    // Set the timer to trigger SIGALRM every 1 second
    /*
    
    int setitimer(
                    int which, 
                    const struct itimerval *restrict new_value,
                    struct itimerval *_Nullable restrict old_value
                );
    arms or disarms the timer specified by which, by setting the timer to the value specified by new_value.
    If old_value is non-NULL, the buffer it points to is used to return the previous value of the timer

    - ITIMER_REAL 
        This timer counts down in real (i.e., wall clock) time. At each expiration, a SIGALRM signal is generated.
    - itime
        The structure that specifies the timer interval and initial expiration
    - NULL
        We don't care about the old timer value, so we pass NULL
    */
    if (-1 == setitimer(ITIMER_REAL, &itime, NULL)) {
        perror("setitimer failed"); 
        return -1;
    }

    /*
    Inizializziamo mask come insieme di segnali vuoto:
        - mask è una variabile di tipo sigset_t
        - sigemptyset() la svuota completamente
        - dopo questa chiamata, mask non contiene nessun segnale

    Aggiungiamo SIGIO e SIGALRM a mask:
        - sigaddset() aggiunge un segnale specificato a un insieme di segnali
        - dopo queste chiamate, mask contiene SIGIO e SIGALRM

    Quindi alla fine mask rappresenta un insieme di segnali che contiene quei due e nessun altro.
    Mentalmente possiamo immaginarlo come una specie di set:
        mask = { SIGIO, SIGALRM };
    */

    if (-1 == sigemptyset(&mask)) {
        perror("sigemptyset failed");
        return 1;
    } 

    // Add SIGIO to the signal set
    if (-1 == sigaddset(&mask, SIGIO)) {
        perror("sigaddset SIGIO failed");
        return 1;
    }

    // Add SIGALRM to the signal set
    if (-1 == sigaddset(&mask, SIGALRM)) {
        perror("sigaddset SIGALRM failed");
        return 1;
    }

    /*
    sigprocmask() Examine and change blocked signals
    SIG_UNBLOCK
            The signals in set are removed from the current set of
            blocked signals.  It is permissible to attempt to unblock a
            signal which is not blocked.

    Quindi, se quei segnali erano bloccati, da questo momento possono essere consegnati normalmente e 
    i relativi handler (myio e mytimer) possono partire. Se uno di quei segnali era arrivato mentre era 
    bloccato, può essere consegnato subito dopo lo sblocco.
    */
   
    if (-1 == sigprocmask(SIG_UNBLOCK, &mask, NULL)) {
        perror("setprocmask failed");
        return 1;
    }

    while(1) { 
        if(bufstart) { // If there is data in the buffer, print it and reset the buffer
            databuf[bufstart] = 0; // Null-terminate the buffer to make it a valid string 
            printf("Received data: %s\n", databuf); // Print the received data
            bufstart = 0; // Reset the buffer start index to 0 for the next input 
        } 
    }
} 