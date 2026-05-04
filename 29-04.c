

struct tcpctrlblk { 
    struct txcontrolbuf *txfirst, * txlast; 
    int st; 
    unsigned short r_port; 
    unsigned int r_addr; 
    unsigned short adwin; 
    unsigned char * rxbuffer; 
    unsigned int rx_win_start; 
    struct rxcontrol * unack; 
    unsigned int cumulativeack; 
    unsigned int ack_offs, seq_offs; 
    unsigned int sequence; 
}; 

 
 
struct txcontrolbuf { 
    struct tcp_segment * segment; 
    int totlen; 
    int payloadlen; 
    long long int expire; 
    struct txcontrolbuf * next; 
}; 




 
struct rxcontrol { 
    unsigned int stream_offs; 
    unsigned int streamsegmentsize; 
    struct rxcontrol * next; 
}; 


struct tcp_segment { 
unsigned short s_port; 
unsigned short d_port; 
unsigned int seq; 
unsigned int ack; 
unsigned char d_offs_res; 
unsigned char flags; 
unsigned short window; 
unsigned short checksum; 
unsigned short urgp; 
unsigned char payload[TCP_MSS]; 
}; 



int prepare_tcp(int s, unsigned char flags, unsigned char * payload, int payloadlen) { 
    struct tcpctrlblk *t = fdinfo[s].tcb; 
    struct tcp_segment * tcp; 
    struct txcontrolbuf * txcb = (struct txcontrolbuf*) malloc(sizeof( struct txcontrolbuf)); 
    
    txcb->expire = tick - 1; 
    txcb->payloadlen = payloadlen; 
    txcb->totlen = payloadlen + 20; 
    tcp = txcb->segment = (struct tcp_segment *) malloc(sizeof(struct tcp_segment)); 
    
    tcp->s_port = fdinfo[s].l_port ; 
    tcp->d_port = t->r_port; 
    tcp->seq = htonl(t->seq_offs+t->sequence); 
    tcp->d_offs_res=0x50; 
    tcp->flags=flags&0x3F; 
    tcp->urgp=0; 

    if(payload != NULL) {
        memcpy(tcp->payload,payload,payloadlen);
    }
    txcb->next=NULL; 
    if(t->txfirst == NULL) { 
        t->txlast = t->txfirst = txcb;
    } else {
        t->txlast->next = txcb; 
        t->txlast = t->txlast->next; 
    } 
    printf("Packet seq inserted %d:%d\n",t->sequence, t->sequence+payloadlen); 
    t->sequence += payloadlen; 
    //DEFERRED FIELDS 
    //tcp->ack; 
    //tcp->window; 
    //tcp->checksum=0; 

}