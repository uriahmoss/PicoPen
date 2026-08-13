#include "picopen/recon.h"
#include <string.h>
#include "lwip/dns.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip.h"
#include "lwip/ip4_addr.h"
#include "lwip/pbuf.h"
#include "lwip/raw.h"
#include "lwip/tcp.h"
#include "picopen/engagement.h"
#define RECON_TIMEOUT_MS 5000u
#define RECON_RATE_MS 1000u
static picopen_recon_snapshot_t current;
static uint64_t started_ms, deadline_ms, next_allowed_ms;
static ip_addr_t resolved;
static struct tcp_pcb *tcp_pcb;
static struct raw_pcb *raw_pcb;
static uint16_t echo_identifier, echo_sequence;

static bool target_allowed(const char *target,const ip_addr_t *address,uint16_t port,uint64_t now_ms) {
    if (picopen_engagement_session_allows_hostname(target,port,now_ms)) return true;
    return address && IP_IS_V4(address) &&
        picopen_engagement_session_allows_ipv4(lwip_ntohl(ip_2_ip4(address)->addr),port,now_ms);
}
static void finish(picopen_recon_state_t state,int result) {
    current.state=state; current.result=result;
    if(tcp_pcb){tcp_abort(tcp_pcb);tcp_pcb=NULL;}
    if(raw_pcb){raw_remove(raw_pcb);raw_pcb=NULL;}
}
static err_t tcp_connected(void *arg,struct tcp_pcb *pcb,err_t error) {
    (void)arg; tcp_pcb=NULL; if(error==ERR_OK){(void)tcp_close(pcb);finish(PICOPEN_RECON_COMPLETE,0);}else finish(PICOPEN_RECON_ERROR,error); return ERR_OK;
}
static void tcp_error(void *arg,err_t error){(void)arg;tcp_pcb=NULL;finish(PICOPEN_RECON_ERROR,error);}
static u8_t echo_received(void *arg,struct raw_pcb *pcb,struct pbuf *packet,const ip_addr_t *address) {
    (void)arg;(void)pcb;(void)address;
    if(packet->tot_len>=IP_HLEN+sizeof(struct icmp_echo_hdr)){
        struct icmp_echo_hdr header;
        pbuf_copy_partial(packet,&header,sizeof(header),IP_HLEN);
        if(ICMPH_TYPE(&header)==ICMP_ER && header.id==echo_identifier && header.seqno==echo_sequence){finish(PICOPEN_RECON_COMPLETE,0);return 1u;}
    }
    return 0u;
}
static bool start_operation(void) {
    if(current.kind==PICOPEN_RECON_DNS){finish(PICOPEN_RECON_COMPLETE,0);return true;}
    if(current.kind==PICOPEN_RECON_TCP){
        tcp_pcb=tcp_new_ip_type(IPADDR_TYPE_V4); if(!tcp_pcb)return false;
        tcp_arg(tcp_pcb,NULL);tcp_err(tcp_pcb,tcp_error);
        return tcp_connect(tcp_pcb,&resolved,current.port,tcp_connected)==ERR_OK;
    }
    raw_pcb=raw_new_ip_type(IPADDR_TYPE_V4,IP_PROTO_ICMP); if(!raw_pcb)return false;
    raw_recv(raw_pcb,echo_received,NULL);
    struct pbuf *packet=pbuf_alloc(PBUF_IP,sizeof(struct icmp_echo_hdr),PBUF_RAM); if(!packet)return false;
    struct icmp_echo_hdr *header=(struct icmp_echo_hdr *)packet->payload;
    ICMPH_TYPE_SET(header,ICMP_ECHO);ICMPH_CODE_SET(header,0);header->id=echo_identifier;
    header->seqno=echo_sequence;header->chksum=0;header->chksum=inet_chksum(header,sizeof(*header));
    const err_t result=raw_sendto(raw_pcb,packet,&resolved);pbuf_free(packet);return result==ERR_OK;
}
static void dns_complete(const char *name,const ip_addr_t *address,void *arg) {
    (void)name;(void)arg;if(current.state!=PICOPEN_RECON_RUNNING)return;
    if(!address){finish(PICOPEN_RECON_ERROR,ERR_VAL);return;} resolved=*address;
    (void)ip4addr_ntoa_r(ip_2_ip4(&resolved),current.address,sizeof(current.address));
    if(!target_allowed(current.target,&resolved,current.port,started_ms)){finish(PICOPEN_RECON_DENIED,ERR_VAL);return;}
    if(!start_operation())finish(PICOPEN_RECON_ERROR,ERR_IF);
}
void picopen_recon_init(void){current=(picopen_recon_snapshot_t){.state=PICOPEN_RECON_IDLE};}
bool picopen_recon_start(picopen_recon_kind_t kind,const char *target,uint16_t port,uint64_t now_ms,bool locally_confirmed){
    if(!locally_confirmed||!target||!target[0]||port==0u||current.state==PICOPEN_RECON_RUNNING||now_ms<next_allowed_ms)return false;
    current=(picopen_recon_snapshot_t){.kind=kind,.state=PICOPEN_RECON_RUNNING,.port=port};
    strncpy(current.target,target,sizeof(current.target)-1u);started_ms=now_ms;deadline_ms=now_ms+RECON_TIMEOUT_MS;next_allowed_ms=now_ms+RECON_RATE_MS;
    echo_identifier=(uint16_t)now_ms;echo_sequence++;
    if(ipaddr_aton(target,&resolved))dns_complete(target,&resolved,NULL);
    else { const err_t result=dns_gethostbyname(target,&resolved,dns_complete,NULL); if(result==ERR_OK)dns_complete(target,&resolved,NULL);else if(result!=ERR_INPROGRESS)finish(PICOPEN_RECON_ERROR,result); }
    return current.state!=PICOPEN_RECON_DENIED && current.state!=PICOPEN_RECON_ERROR;
}
bool picopen_recon_cancel(void){if(current.state!=PICOPEN_RECON_RUNNING)return false;finish(PICOPEN_RECON_CANCELLED,0);return true;}
bool picopen_recon_poll(uint64_t now_ms){
    if(current.state!=PICOPEN_RECON_RUNNING)return false;
    current.elapsed_ms=(uint32_t)(now_ms-started_ms);
    if(!target_allowed(current.target,current.address[0]?&resolved:NULL,current.port,now_ms)){
        finish(PICOPEN_RECON_DENIED,ERR_VAL);
        return true;
    }
    if(now_ms>=deadline_ms){finish(PICOPEN_RECON_TIMEOUT,ERR_TIMEOUT);return true;}
    return false;
}
void picopen_recon_snapshot(picopen_recon_snapshot_t *snapshot){if(snapshot)*snapshot=current;}
const char *picopen_recon_state_name(picopen_recon_state_t state){static const char *const names[]={"IDLE","RUNNING","COMPLETE","DENIED","TIMEOUT","CANCELLED","ERROR"};return (unsigned)state<sizeof(names)/sizeof(names[0])?names[state]:"UNKNOWN";}
