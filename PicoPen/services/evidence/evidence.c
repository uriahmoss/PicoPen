#include "picopen/evidence.h"
#include <stdio.h>
#include <string.h>
#include "mbedtls/sha256.h"
#define EVIDENCE_MAX_SIZE UINT32_C(262144)
static picopen_evidence_snapshot_t current;
static picopen_storage_service_t *source;
static mbedtls_sha256_context hash;
static uint8_t string_run;
static uint32_t parse_offset;
static bool little_endian;
static uint32_t word(const uint8_t *p,bool little){return little?((uint32_t)p[0]|(uint32_t)p[1]<<8u|(uint32_t)p[2]<<16u|(uint32_t)p[3]<<24u):((uint32_t)p[0]<<24u|(uint32_t)p[1]<<16u|(uint32_t)p[2]<<8u|p[3]);}
static bool read_at(uint32_t offset,uint8_t *data,size_t capacity,size_t *length){bool truncated=false;return picopen_storage_read_file(source,current.path,offset,data,capacity,length,&truncated)==PICOPEN_STORAGE_OK;}
void picopen_evidence_init(void){current=(picopen_evidence_snapshot_t){.state=PICOPEN_EVIDENCE_IDLE};mbedtls_sha256_init(&hash);}
bool picopen_evidence_start(picopen_storage_service_t *storage,const char *path,uint32_t size,bool locally_confirmed){
    if(!locally_confirmed||!storage||!path||!path[0]||size>EVIDENCE_MAX_SIZE||current.state==PICOPEN_EVIDENCE_HASHING||current.state==PICOPEN_EVIDENCE_PARSING)return false;
    current=(picopen_evidence_snapshot_t){.state=PICOPEN_EVIDENCE_HASHING,.size=size};strncpy(current.path,path,sizeof(current.path)-1u);source=storage;string_run=0u;parse_offset=0u;
    mbedtls_sha256_free(&hash);mbedtls_sha256_init(&hash);return mbedtls_sha256_starts(&hash,0)==0;
}
bool picopen_evidence_cancel(void){if(current.state!=PICOPEN_EVIDENCE_HASHING&&current.state!=PICOPEN_EVIDENCE_PARSING)return false;current.state=PICOPEN_EVIDENCE_CANCELLED;mbedtls_sha256_free(&hash);return true;}
static void finish_hash(void){uint8_t digest[32];if(string_run>=4u)++current.string_count;string_run=0u;if(mbedtls_sha256_finish(&hash,digest)!=0){current.state=PICOPEN_EVIDENCE_ERROR;return;}for(size_t i=0u;i<32u;++i)snprintf(&current.sha256[i*2u],3u,"%02x",digest[i]);mbedtls_sha256_free(&hash);current.state=PICOPEN_EVIDENCE_PARSING;parse_offset=0u;}
bool picopen_evidence_poll(void){
    if(current.state==PICOPEN_EVIDENCE_HASHING){if(current.processed>=current.size){finish_hash();return true;}uint8_t data[256];size_t length=0u;const size_t wanted=current.size-current.processed<sizeof(data)?current.size-current.processed:sizeof(data);if(!read_at(current.processed,data,wanted,&length)||length!=wanted||mbedtls_sha256_update(&hash,data,length)!=0){current.state=PICOPEN_EVIDENCE_ERROR;return true;}for(size_t i=0u;i<length;++i){if(data[i]>=32u&&data[i]<=126u){if(string_run<255u)++string_run;}else{if(string_run>=4u)++current.string_count;string_run=0u;}}current.processed+=(uint32_t)length;return true;}
    if(current.state!=PICOPEN_EVIDENCE_PARSING)return false;
    uint8_t header[24];size_t length=0u;if(parse_offset==0u){if(!read_at(0u,header,sizeof(header),&length)||length<12u){current.state=PICOPEN_EVIDENCE_ERROR;return true;}const uint32_t magic=word(header,true);if(magic==UINT32_C(0xA1B2C3D4)||magic==UINT32_C(0xD4C3B2A1)){current.capture=PICOPEN_CAPTURE_PCAP;little_endian=magic==UINT32_C(0xA1B2C3D4);parse_offset=24u;}else if(magic==UINT32_C(0x0A0D0D0A)){const uint32_t byte_order=word(&header[8],true);if(byte_order==UINT32_C(0x1A2B3C4D))little_endian=true;else if(byte_order==UINT32_C(0x4D3C2B1A))little_endian=false;else{current.state=PICOPEN_EVIDENCE_ERROR;return true;}current.capture=PICOPEN_CAPTURE_PCAPNG;parse_offset=word(&header[4],little_endian);if(parse_offset<28u||parse_offset>current.size){current.state=PICOPEN_EVIDENCE_ERROR;}}else{current.state=PICOPEN_EVIDENCE_COMPLETE;}return true;}
    const size_t needed=current.capture==PICOPEN_CAPTURE_PCAP?16u:12u;if(!read_at(parse_offset,header,needed,&length)||length<needed){current.state=PICOPEN_EVIDENCE_COMPLETE;return true;}const uint32_t type=word(header,little_endian);uint32_t step=current.capture==PICOPEN_CAPTURE_PCAP?16u+word(&header[8],little_endian):word(&header[4],little_endian);if(step<needed||step>current.size-parse_offset){current.state=PICOPEN_EVIDENCE_ERROR;return true;}if(current.capture==PICOPEN_CAPTURE_PCAP||type==6u)++current.packet_count;parse_offset+=step;if(parse_offset>=current.size)current.state=PICOPEN_EVIDENCE_COMPLETE;return true;
}
void picopen_evidence_snapshot(picopen_evidence_snapshot_t *snapshot){if(snapshot)*snapshot=current;}
const char *picopen_evidence_state_name(picopen_evidence_state_t state){static const char *const names[]={"IDLE","HASHING","PARSING","COMPLETE","CANCELLED","DENIED","ERROR"};return (unsigned)state<sizeof(names)/sizeof(names[0])?names[state]:"UNKNOWN";}
