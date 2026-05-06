# Active Open + Read/Write TCP

Questa nota riorganizza i concetti presenti nell'immagine `12.2 OVERVIEW #1: ACTIVE OPEN + READ + WRITE`.

Nota importante: il testo nello screenshot e' in parte coperto da evidenziature e annotazioni. Il codice qui sotto e' quindi una **ricostruzione fedele della logica** mostrata nell'immagine, ma non una trascrizione byte-per-byte perfetta.

## 1. Cosa mostra l'immagine

L'immagine descrive una piccola implementazione di TCP lato client, con questi elementi:

- API verso l'applicazione: `socket()`, `bind()`, `connect()`, `read()`, `write()`
- Tabella dei file descriptor, che collega ogni socket al suo stato e al suo `TCB`
- `TCB` (Transmission Control Block), che contiene tutto lo stato della connessione TCP
- Buffer di ricezione `RX`
- Buffer di trasmissione `TX`
- FSM TCP minima, sufficiente per l'`active open`
- Gestione asincrona dei pacchetti in arrivo tramite `SIGIO`
- Gestione dei timeout e delle ritrasmissioni tramite `SIGALRM`

In pratica, l'applicazione vede primitive semplici (`connect`, `read`, `write`), ma sotto c'e' una macchina a stati che gestisce:

- three-way handshake
- numeri di sequenza
- ACK cumulativi
- finestre
- segmenti fuori ordine
- ritrasmissioni

## 2. Idee chiave

### 2.1 Vista logica dall'alto

Il flusso generale e':

1. `mysocket()` crea una socket logica
2. `mybind()` assegna indirizzo/porta locale
3. `myconnect()` imposta peer remoto e innesca `APP_ACTIVE_OPEN`
4. La `FSM` invia il `SYN`
5. `SIGIO` riceve `SYN+ACK`
6. La `FSM` invia l'`ACK` finale
7. Lo stato diventa `ESTABLISHED`
8. `myread()` consuma dati dal buffer RX
9. `mywrite()` produce segmenti e li mette in TX
10. `SIGIO` rimuove dal TX i segmenti confermati dagli ACK
11. `SIGALRM` ritrasmette i segmenti andati in timeout

### 2.2 Tabella dei file descriptor

Ogni entry della tabella contiene almeno:

- stato lato socket: `FREE`, `TCP_UNBOUND`, `TCP_BOUND`, `TCB_CREATED`
- porta locale `l_port`
- indirizzo locale `l_addr`
- puntatore al `TCB`

La tabella serve a separare due livelli:

- stato "esterno" della socket, visto dalle syscall locali
- stato "interno" della connessione TCP, contenuto nel `TCB`

### 2.3 TCB

Il `TCB` contiene tipicamente:

- stato della FSM TCP: `TCP_CLOSED`, `SYN_SENT`, `ESTABLISHED`, `CLOSE_WAIT`, `TIME_WAIT`
- indirizzo e porta remoti: `r_addr`, `r_port`
- offset di sequenza locali/remoti: `seq_offs`, `ack_offs`
- ACK cumulativo di stream: `cumulativeack`
- inizio finestra RX: `rx_win_start`
- eventuale fine stream: `stream_end`
- finestra pubblicizzata: `adwin`
- spazio libero TX: `txfree`
- MSS
- lista dei segmenti ricevuti fuori ordine ma non ancora consumabili
- lista dei segmenti trasmessi e non ancora confermati
- eventuale timer della FSM

## 3. Significato dei buffer

### 3.1 RX buffer

Il buffer RX non contiene semplicemente "byte ricevuti". Contiene byte ricevuti in ordine o fuori ordine.

Per questo si usano due idee:

- `rx_win_start`: primo byte non ancora letto dall'applicazione
- `cumulativeack`: primo byte non ancora ricevuto in modo contiguo

Quindi:

- i byte compresi tra `rx_win_start` e `cumulativeack - 1` sono leggibili da `myread()`
- i byte oltre `cumulativeack` possono anche essere gia' arrivati, ma non sono ancora contigui

I segmenti fuori ordine vengono inseriti in una lista ordinata per `stream_offs`.

### 3.2 TX buffer

Nel TX non basta ricordare i dati da inviare: bisogna ricordare i dati **inviati ma non ancora ACKati**.

Per ogni segmento in volo si tiene almeno:

- puntatore al segmento TCP
- lunghezza totale
- lunghezza payload
- istante ultimo invio
- timeout
- puntatore al prossimo segmento

Quando arriva un ACK:

- i segmenti completamente confermati vengono rimossi dalla coda
- il relativo spazio torna disponibile in `txfree`

## 4. Stato e transizioni principali

### 4.1 `mysocket()`

`mysocket()`:

- verifica `AF_INET` e `SOCK_STREAM`
- cerca uno slot libero nella tabella dei descrittori
- azzera la struttura relativa
- imposta stato `TCP_UNBOUND`

Questa chiamata non crea ancora una connessione TCP. Crea solo una socket locale astratta.

### 4.2 `mybind()`

`mybind()`:

- controlla che la socket sia valida
- richiede che lo stato sia `TCP_UNBOUND`
- assegna la porta locale
- se la porta e' gia' in uso, fallisce
- imposta l'indirizzo locale
- porta la socket a `TCP_BOUND`

### 4.3 `myconnect()`

`myconnect()` fa tre cose molto importanti:

1. Se la socket non e' ancora bound, esegue un bind implicito su una porta effimera
2. Alloca il `TCB`
3. Attiva la FSM con l'evento `APP_ACTIVE_OPEN`

Da quel momento la connessione non e' piu' solo "una socket bound": diventa una vera connessione TCP con stato, sequence number e buffer dedicati.

### 4.4 FSM: `APP_ACTIVE_OPEN`

Nel caso `TCP_CLOSED`, l'evento `APP_ACTIVE_OPEN`:

- alloca `rxbuffer`
- inizializza `txfree`
- inizializza `rx_win_start`
- genera `seq_offs`
- azzera `cumulativeack`
- azzera `ack_offs`
- costruisce e invia un segmento `SYN`
- porta lo stato a `SYN_SENT`

### 4.5 FSM: ricezione `SYN+ACK`

Nel caso `SYN_SENT`, quando arriva un pacchetto con:

- flag `SYN`
- flag `ACK`
- `ack == seq_offs + 1`

allora:

- il nostro `SYN` e' stato confermato
- il sequence number remoto diventa la base del nostro `ack_offs`
- il vecchio segmento `SYN` nel TX viene rimosso
- viene inviato l'`ACK` finale
- lo stato diventa `ESTABLISHED`

Questa e' la chiusura del three-way handshake lato client.

## 5. Percorso di ricezione

### 5.1 `SIGIO` come punto di ingresso

Quando arriva un frame:

- si controlla che sia IPv4
- si controlla che il protocollo sia TCP
- si estrae il segmento TCP
- si cerca nella tabella dei descrittori la connessione giusta

Il match avviene confrontando:

- porta locale con `d_port`
- porta remota con `s_port`
- indirizzo remoto con `srcaddr`

Dopo il match, si chiama la FSM con `PKT_RCV`.

### 5.2 Inserimento dei dati in RX

Se il pacchetto contiene dati:

- si calcola `stream_offs`
- si verifica se il segmento cade ancora dentro la finestra RX
- si crea un nodo per la lista dei segmenti ricevuti
- si copia il payload nel `rxbuffer`
- si inserisce il nodo in lista in posizione ordinata

Se il segmento e':

- duplicato
- troppo vecchio
- fuori finestra

non viene "attaccato" alla lista utile.

### 5.3 Avanzamento dell'ACK cumulativo

Dopo l'inserimento, il ricevitore prova ad avanzare:

- finche' la testa della lista ha `stream_offs == cumulativeack`
- quel pezzo e' contiguo, quindi puo' essere assorbito
- `cumulativeack` avanza della dimensione del segmento
- la advertised window `adwin` viene aggiornata
- il nodo viene rimosso

Questa parte e' il cuore della ricezione ordinata.

### 5.4 Generazione di ACK

Dopo aver aggiornato la porzione contigua di stream:

- se la connessione non e' in `TIME_WAIT`
- viene preparato un `ACK`

L'ACK pubblica:

- il prossimo byte atteso (`ack_offs + cumulativeack`)
- la finestra disponibile `adwin`

## 6. Percorso di lettura

`myread()` legge dal `RX buffer`, non dalla rete direttamente.

Il comportamento e':

- se la socket non e' in uno stato valido, errore
- se `maxlen == 0`, ritorna `0`
- calcola quanti byte sono gia' disponibili:
  `MIN(maxlen, cumulativeack - rx_win_start)`
- se non c'e' nulla di disponibile, aspetta un segnale

Due casi importanti:

- se il peer ha inviato `FIN` e non resta piu' nulla da leggere, `read()` ritorna `0`
- se il `FIN` e' gia' entrato nella logica di stream, il byte virtuale di fine stream non deve essere restituito come dato

Poi:

- copia i byte da `rxbuffer[(rx_win_start + i) % RXBUFSIZE]`
- avanza `rx_win_start`
- ritorna il numero di byte copiati

## 7. Percorso di trasmissione

`mywrite()` non invia tutto "in un colpo".

Il comportamento mostrato nell'immagine e':

- prende al massimo lo spazio disponibile in `txfree`
- spezza i dati in chunk da massimo `mss`
- per ogni chunk crea un segmento TCP con `prepare_tcp(...)`
- inserisce il segmento nella coda TX
- diminuisce `txfree`

La protezione con `sigprocmask(SIG_BLOCK, ...)` serve a evitare corse critiche mentre si manipolano le strutture condivise con i signal handler.

## 8. Gestione ACK lato TX

Quando `SIGIO` riceve un segmento con ACK:

- controlla se l'ACK cade nell'intervallo dei segmenti attualmente in volo
- finche' la testa della coda TX e' completamente coperta dall'ACK:
  - rimuove il segmento
  - libera memoria
  - restituisce il payload a `txfree`

Questo e' il meccanismo che rende nuovamente disponibile spazio di trasmissione.

## 9. Gestione timeout

`SIGALRM` visita tutte le connessioni attive.

Per ciascuna:

- se esiste un timer di FSM scaduto, genera `TIMEOUT`
- altrimenti scorre la coda dei segmenti in volo
- se un segmento ha superato il suo timeout:
  - aggiorna l'header TCP
  - aggiorna il timestamp di invio
  - lo ritrasmette

Questa parte mostra che l'affidabilita' TCP non deriva dalla `write()`, ma dal ciclo:

- invio
- attesa ACK
- timeout
- ritrasmissione

## 10. Codice ricostruito

Di seguito trovi una versione C-like compatta e coerente con l'immagine.

```c
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FD 1024
#define RXBUFSIZE 8192
#define TXBUFSIZE 8192
#define TCP_PROTO 6

#define FREE 0
#define TCP_UNBOUND 1
#define TCP_BOUND 2
#define TCB_CREATED 3

#define TCP_CLOSED 10
#define SYN_SENT 11
#define ESTABLISHED 12
#define CLOSE_WAIT 13
#define TIME_WAIT 14

#define APP_ACTIVE_OPEN 100
#define PKT_RCV 101
#define TIMEOUT 102

#define SYN 0x02
#define ACK 0x10

#define MIN(a,b) ((a) < (b) ? (a) : (b))

struct tcp_segment {
    unsigned short s_port;
    unsigned short d_port;
    unsigned int seq;
    unsigned int ack;
    unsigned char hdrlen_flags[2];
    unsigned short window;
    unsigned short checksum;
    unsigned short urgptr;
};

struct rxcontrol {
    unsigned int stream_offs;
    unsigned int streamsegmentsize;
    struct rxcontrol *next;
};

struct txcontrolbuf {
    struct tcp_segment *segment;
    unsigned int totlen;
    unsigned int payloadlen;
    unsigned int txtime;
    unsigned int timeout;
    struct txcontrolbuf *next;
};

struct tcpctrlblk {
    int st;
    unsigned int r_addr;
    unsigned short r_port;

    unsigned int seq_offs;
    unsigned int ack_offs;
    unsigned int cumulativeack;
    unsigned int rx_win_start;
    unsigned int stream_end;
    unsigned short adwin;
    unsigned short mss;
    unsigned int txfree;

    unsigned int fsm_timer;

    unsigned char *rxbuffer;
    struct rxcontrol *unack;
    struct txcontrolbuf *txfirst;
    struct txcontrolbuf *txlast;
};

struct socket_info {
    int st;
    unsigned short l_port;
    unsigned int l_addr;
    struct tcpctrlblk *tcb;
};

struct socket_info fdinfo[MAX_FD];
unsigned int myip;
unsigned int tick;
int myerrno;
sigset_t mymask;

unsigned short get_free_port(void);
unsigned short checksum2(unsigned char *p1, int l1, unsigned char *p2, int l2);
void send_ip(unsigned char *payload, unsigned char *dst, int len, int proto);
void prepare_tcp(int s, int flags, unsigned char *payload, int payloadlen);

int port_in_use(unsigned short port) {
    int s;
    for (s = 3; s < MAX_FD; s++) {
        if (fdinfo[s].st != FREE && fdinfo[s].st != TCP_UNBOUND) {
            if (fdinfo[s].l_port == port) return 1;
        }
    }
    return 0;
}

int mysocket(int family, int type, int proto) {
    int t;
    if (family == AF_INET && type == SOCK_STREAM && proto == 0) {
        for (t = 3; t < MAX_FD && fdinfo[t].st != FREE; t++);
        if (t == MAX_FD) { myerrno = 1; return -1; }
        bzero(fdinfo + t, sizeof(struct socket_info));
        fdinfo[t].st = TCP_UNBOUND;
        myerrno = 0;
        return t;
    }
    myerrno = 2;
    return -1;
}

int mybind(int s, struct sockaddr *addr, int addrlen) {
    struct sockaddr_in *a;
    if (addr->sa_family != AF_INET) { myerrno = 2; return -1; }
    if (s < 3 || s >= MAX_FD) { myerrno = 3; return -1; }
    if (fdinfo[s].st != TCP_UNBOUND) { myerrno = 2; return -1; }

    a = (struct sockaddr_in *)addr;
    if (a->sin_port && port_in_use(a->sin_port)) { myerrno = 4; return -1; }

    fdinfo[s].l_port = a->sin_port ? a->sin_port : get_free_port();
    if (fdinfo[s].l_port == 0) return -1;

    fdinfo[s].l_addr = a->sin_addr.s_addr ? a->sin_addr.s_addr : (unsigned int)myip;
    fdinfo[s].st = TCP_BOUND;
    myerrno = 0;
    return 0;
}

void fsm(int s, int event, void *x) {
    struct tcpctrlblk *tcb = fdinfo[s].tcb;
    struct tcp_segment *tcp = (struct tcp_segment *)x;

    switch (tcb->st) {
    case TCP_CLOSED:
        if (event == APP_ACTIVE_OPEN) {
            tcb->rxbuffer = (unsigned char *)malloc(RXBUFSIZE);
            tcb->txfree = TXBUFSIZE;
            tcb->rx_win_start = 0;
            tcb->seq_offs = rand();
            tcb->cumulativeack = 0;
            tcb->ack_offs = 0;
            tcb->adwin = RXBUFSIZE;
            prepare_tcp(s, SYN, NULL, 0);
            tcb->st = SYN_SENT;
        }
        break;

    case SYN_SENT:
        if (event == PKT_RCV &&
            (tcp->hdrlen_flags[1] & SYN) &&
            (tcp->hdrlen_flags[1] & ACK) &&
            ntohl(tcp->ack) == tcb->seq_offs + 1) {

            tcb->seq_offs++;
            tcb->ack_offs = ntohl(tcp->seq);

            if (tcb->txfirst != NULL) {
                free(tcb->txfirst->segment);
                free(tcb->txfirst);
                tcb->txfirst = tcb->txlast = NULL;
            }

            prepare_tcp(s, ACK, NULL, 0);
            tcb->st = ESTABLISHED;
        }
        break;
    }
}

int myconnect(int s, struct sockaddr *addr, int addrlen) {
    struct sockaddr_in *a = (struct sockaddr_in *)addr;
    struct sockaddr_in local;

    if (addr->sa_family != AF_INET) { myerrno = 2; return -1; }
    if (s < 3 || s >= MAX_FD) { myerrno = 3; return -1; }

    if (fdinfo[s].st == TCP_UNBOUND) {
        local.sin_port = htons(0);
        local.sin_addr.s_addr = htonl(0);
        local.sin_family = AF_INET;
        if (-1 == mybind(s, (struct sockaddr *)&local, sizeof(struct sockaddr_in))) return -1;
    }

    if (fdinfo[s].st != TCP_BOUND) { myerrno = 3; return -1; }

    fdinfo[s].tcb = (struct tcpctrlblk *)malloc(sizeof(struct tcpctrlblk));
    bzero(fdinfo[s].tcb, sizeof(struct tcpctrlblk));
    fdinfo[s].st = TCB_CREATED;
    fdinfo[s].tcb->st = TCP_CLOSED;
    fdinfo[s].tcb->r_port = a->sin_port;
    fdinfo[s].tcb->r_addr = a->sin_addr.s_addr;
    fdinfo[s].tcb->mss = 1460;
    fsm(s, APP_ACTIVE_OPEN, NULL);

    while (sleep(10)) {
        if (fdinfo[s].tcb->st == ESTABLISHED) return 0;
        if (fdinfo[s].tcb->st == TCP_CLOSED) return -1;
    }
    return -1;
}

int myread(int s, unsigned char *buffer, int maxlen) {
    int i, actual_len;

    if (fdinfo[s].st != TCB_CREATED || fdinfo[s].tcb->st < ESTABLISHED) return -1;
    if (maxlen == 0) return 0;

    actual_len = MIN(maxlen, (int)(fdinfo[s].tcb->cumulativeack - fdinfo[s].tcb->rx_win_start));
    if ((fdinfo[s].tcb->cumulativeack - fdinfo[s].tcb->stream_end) > 0) actual_len--;

    if (actual_len == 0) {
        while (pause()) {
            actual_len = MIN(maxlen, (int)(fdinfo[s].tcb->cumulativeack - fdinfo[s].tcb->rx_win_start));
            if (actual_len != 0) break;
            if (fdinfo[s].tcb->rx_win_start == fdinfo[s].tcb->stream_end) return 0;
            if (fdinfo[s].tcb->st == CLOSE_WAIT && fdinfo[s].tcb->unack == NULL) return 0;
        }
    }

    for (i = 0; i < actual_len; i++) {
        buffer[i] = fdinfo[s].tcb->rxbuffer[(fdinfo[s].tcb->rx_win_start + i) % RXBUFSIZE];
    }
    fdinfo[s].tcb->rx_win_start += i;
    return i;
}

int mywrite(int s, unsigned char *buffer, int maxlen) {
    int j, len, actual_len, totlen = 0;

    do {
        actual_len = MIN(maxlen - totlen, (int)fdinfo[s].tcb->txfree);
        if (actual_len <= 0 || fdinfo[s].tcb->st == TCP_CLOSED) break;

        while (pause()) { }

        for (j = 0; j < actual_len; j += len) {
            len = MIN(fdinfo[s].tcb->mss, actual_len - j);
            sigprocmask(SIG_BLOCK, &mymask, NULL);
            prepare_tcp(s, ACK, buffer + totlen + j, len);
            sigprocmask(SIG_UNBLOCK, &mymask, NULL);
            fdinfo[s].tcb->txfree -= len;
            totlen += len;
        }
    } while (totlen < maxlen);

    return totlen;
}

void update_tcp_header(int s, struct txcontrolbuf *txcb) {
    struct tcpctrlblk *tcb = fdinfo[s].tcb;
    struct {
        unsigned int s_addr;
        unsigned int d_addr;
        unsigned char zero;
        unsigned char prot;
        unsigned short len;
    } pseudo;

    pseudo.s_addr = fdinfo[s].l_addr;
    pseudo.d_addr = tcb->r_addr;
    pseudo.zero = 0;
    pseudo.prot = TCP_PROTO;
    pseudo.len = htons(txcb->totlen);

    txcb->segment->checksum = htons(0);
    txcb->segment->ack = htonl(tcb->ack_offs + tcb->cumulativeack);
    txcb->segment->window = htons(tcb->adwin);
    txcb->segment->checksum = htons(
        checksum2((unsigned char *)&pseudo, 12, (unsigned char *)txcb->segment, txcb->totlen)
    );
}

void mytimer(int number) {
    int i;
    struct txcontrolbuf *txcb;

    sigprocmask(SIG_BLOCK, &mymask, NULL);
    tick++;

    for (i = 0; i < MAX_FD; i++) {
        if (fdinfo[i].st != TCB_CREATED) continue;

        if (fdinfo[i].tcb->fsm_timer != 0 && fdinfo[i].tcb->fsm_timer < tick) {
            fsm(i, TIMEOUT, NULL);
            continue;
        }

        for (txcb = fdinfo[i].tcb->txfirst; txcb != NULL; txcb = txcb->next) {
            if (txcb->txtime + txcb->timeout > tick) continue;
            txcb->txtime = tick;
            update_tcp_header(i, txcb);
            send_ip((unsigned char *)txcb->segment,
                    (unsigned char *)&fdinfo[i].tcb->r_addr,
                    txcb->totlen,
                    TCP_PROTO);
        }
    }

    sigprocmask(SIG_UNBLOCK, &mymask, NULL);
}
```

## 11. Interpretazione finale

Il punto piu' importante dell'immagine e' questo:

- `connect()` non "crea magicamente" una connessione
- `read()` non legge direttamente dalla rete
- `write()` non garantisce che i dati siano arrivati

La connessione TCP esiste davvero solo perche' sotto ci sono:

- una FSM
- un `TCB`
- buffer RX/TX
- ACK cumulativi
- timer
- ritrasmissioni

Se vuoi, nel passo successivo posso anche trasformare questa nota in:

- versione da orale, molto discorsiva
- versione schematicissima da ripasso in 1 pagina
- versione con diagramma Mermaid delle transizioni di stato
