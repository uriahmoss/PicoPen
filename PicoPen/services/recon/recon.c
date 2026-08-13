#include "picopen/recon.h"

#include <ctype.h>
#include <stdio.h>
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

#define RECON_TIMEOUT_MS UINT64_C(7000)
#define RECON_RATE_MS UINT64_C(1000)
#define RECON_PROGRESS_INTERVAL_MS UINT64_C(250)
#define RECON_RESPONSE_LIMIT 512u

static picopen_recon_snapshot_t current;
static picopen_recon_snapshot_t history[PICOPEN_RECON_HISTORY_CAPACITY];
static size_t history_count;
static size_t history_next;
static uint64_t started_ms;
static uint64_t deadline_ms;
static uint64_t next_allowed_ms;
static uint64_t next_progress_ms;
static bool snapshot_dirty;
static ip_addr_t resolved;
static struct tcp_pcb *tcp_control;
static struct raw_pcb *raw_control;
static uint16_t echo_identifier;
static uint16_t echo_sequence;
static bool require_boundary;

static bool terminal_state(picopen_recon_state_t state) {
    return state >= PICOPEN_RECON_COMPLETE;
}

static void remember_result(void) {
    history[history_next] = current;
    history_next = (history_next + 1u) % PICOPEN_RECON_HISTORY_CAPACITY;
    if (history_count < PICOPEN_RECON_HISTORY_CAPACITY) ++history_count;
}

static bool target_allowed(const char *target, const ip_addr_t *address,
                           uint16_t port, uint64_t now_ms) {
    if (!picopen_engagement_session_active(now_ms)) return !require_boundary;
    if (picopen_engagement_session_allows_hostname(target, port, now_ms)) {
        return true;
    }
    return address && IP_IS_V4(address) &&
        picopen_engagement_session_allows_ipv4(
            lwip_ntohl(ip_2_ip4(address)->addr), port, now_ms);
}

static void release_transport(bool abort_tcp) {
    if (tcp_control) {
        struct tcp_pcb *const control = tcp_control;
        tcp_control = NULL;
        if (abort_tcp || tcp_close(control) != ERR_OK) tcp_abort(control);
    }
    if (raw_control) {
        raw_remove(raw_control);
        raw_control = NULL;
    }
}

static void finish(picopen_recon_state_t state, int result,
                   const char *detail) {
    if (terminal_state(current.state)) return;
    current.state = state;
    current.result = result;
    if (detail && detail != current.detail)
        snprintf(current.detail, sizeof(current.detail), "%s", detail);
    release_transport(state != PICOPEN_RECON_COMPLETE);
    remember_result();
    snapshot_dirty = true;
}

static void service_name(uint16_t port, char *output, size_t size) {
    const char *name = "UNKNOWN";
    if (port == 22u) name = "SSH";
    else if (port == 53u) name = "DNS";
    else if (port == 80u) name = "HTTP";
    else if (port == 443u) name = "HTTPS";
    else if (port == 445u) name = "SMB";
    else if (port == 3389u) name = "RDP";
    snprintf(output, size, "%s", name);
}

static void append_printable(const struct pbuf *packet) {
    uint8_t bytes[RECON_RESPONSE_LIMIT];
    const size_t remaining = RECON_RESPONSE_LIMIT - current.bytes_received;
    const size_t count = packet->tot_len < remaining ? packet->tot_len : remaining;
    if (count == 0u) return;
    pbuf_copy_partial(packet, bytes, count, 0u);
    current.bytes_received += (uint32_t)count;
    size_t length = strlen(current.detail);
    for (size_t index = 0u;
         index < count && length + 1u < sizeof(current.detail); ++index) {
        const char value = (char)bytes[index];
        if (value == '\r' || value == '\n') {
            if (length && current.detail[length - 1u] != ' ') current.detail[length++] = ' ';
        } else if (isprint((unsigned char)value)) {
            current.detail[length++] = value;
        }
    }
    current.detail[length] = '\0';
}

static err_t tcp_received(void *argument, struct tcp_pcb *control,
                          struct pbuf *packet, err_t error) {
    (void)argument;
    if (error != ERR_OK) {
        if (packet) pbuf_free(packet);
        finish(PICOPEN_RECON_ERROR, error, "RECEIVE ERROR");
        return ERR_OK;
    }
    if (!packet) {
        finish(PICOPEN_RECON_COMPLETE, ERR_OK,
               current.detail[0] ? current.detail : "REMOTE CLOSED");
        return ERR_OK;
    }
    tcp_recved(control, packet->tot_len);
    append_printable(packet);
    pbuf_free(packet);
    if (current.detail[0] || current.bytes_received >= RECON_RESPONSE_LIMIT) {
        finish(PICOPEN_RECON_COMPLETE, ERR_OK, current.detail);
    }
    return ERR_OK;
}

static err_t tcp_connected(void *argument, struct tcp_pcb *control,
                           err_t error) {
    (void)argument;
    if (error != ERR_OK) {
        finish(error == ERR_RST ? PICOPEN_RECON_REFUSED : PICOPEN_RECON_ERROR,
               error, error == ERR_RST ? "CONNECTION REFUSED" : "CONNECT ERROR");
        return ERR_OK;
    }
    tcp_recv(control, tcp_received);
    if (current.kind == PICOPEN_RECON_TCP) {
        finish(PICOPEN_RECON_COMPLETE, ERR_OK, "PORT OPEN");
        return ERR_OK;
    }
    if (current.kind == PICOPEN_RECON_TLS_METADATA) {
        finish(PICOPEN_RECON_UNAVAILABLE, ERR_VAL,
               "TLS CLIENT DISABLED PENDING RAM REVIEW");
        return ERR_OK;
    }
    if (current.kind == PICOPEN_RECON_HTTP_HEAD) {
        char request[192];
        const int length = snprintf(request, sizeof(request),
            "HEAD / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n"
            "User-Agent: PicoPen/0.0.1\r\n\r\n", current.target);
        if (length <= 0 || (size_t)length >= sizeof(request) ||
            tcp_write(control, request, (u16_t)length, TCP_WRITE_FLAG_COPY) != ERR_OK ||
            tcp_output(control) != ERR_OK) {
            finish(PICOPEN_RECON_ERROR, ERR_IF, "HTTP SEND ERROR");
        }
    }
    return ERR_OK;
}

static void tcp_error(void *argument, err_t error) {
    (void)argument;
    tcp_control = NULL;
    finish(error == ERR_RST ? PICOPEN_RECON_REFUSED : PICOPEN_RECON_ERROR,
           error, error == ERR_RST ? "CONNECTION REFUSED" : "TCP ERROR");
}

static u8_t echo_received(void *argument, struct raw_pcb *control,
                          struct pbuf *packet, const ip_addr_t *address) {
    (void)argument;
    (void)control;
    (void)address;
    if (packet->tot_len >= IP_HLEN + sizeof(struct icmp_echo_hdr)) {
        struct icmp_echo_hdr header;
        pbuf_copy_partial(packet, &header, sizeof(header), IP_HLEN);
        if (ICMPH_TYPE(&header) == ICMP_ER && header.id == echo_identifier &&
            header.seqno == echo_sequence) {
            finish(PICOPEN_RECON_COMPLETE, ERR_OK, "ECHO REPLY");
            return 1u;
        }
    }
    return 0u;
}

static bool start_operation(void) {
    if (current.kind == PICOPEN_RECON_DNS) {
        finish(PICOPEN_RECON_COMPLETE, ERR_OK, "RESOLVED");
        return true;
    }
    if (current.kind != PICOPEN_RECON_ICMP) {
        tcp_control = tcp_new_ip_type(IPADDR_TYPE_V4);
        if (!tcp_control) return false;
        tcp_arg(tcp_control, NULL);
        tcp_err(tcp_control, tcp_error);
        return tcp_connect(tcp_control, &resolved, current.port,
                           tcp_connected) == ERR_OK;
    }
    raw_control = raw_new_ip_type(IPADDR_TYPE_V4, IP_PROTO_ICMP);
    if (!raw_control) return false;
    raw_recv(raw_control, echo_received, NULL);
    struct pbuf *packet = pbuf_alloc(PBUF_IP, sizeof(struct icmp_echo_hdr), PBUF_RAM);
    if (!packet) return false;
    struct icmp_echo_hdr *header = (struct icmp_echo_hdr *)packet->payload;
    ICMPH_TYPE_SET(header, ICMP_ECHO);
    ICMPH_CODE_SET(header, 0);
    header->id = echo_identifier;
    header->seqno = echo_sequence;
    header->chksum = 0;
    header->chksum = inet_chksum(header, sizeof(*header));
    const err_t result = raw_sendto(raw_control, packet, &resolved);
    pbuf_free(packet);
    return result == ERR_OK;
}

static void dns_complete(const char *name, const ip_addr_t *address,
                         void *argument) {
    (void)name;
    (void)argument;
    if (current.state != PICOPEN_RECON_RUNNING) return;
    if (!address) {
        finish(PICOPEN_RECON_ERROR, ERR_VAL, "DNS FAILURE");
        return;
    }
    resolved = *address;
    (void)ip4addr_ntoa_r(ip_2_ip4(&resolved), current.address,
                         sizeof(current.address));
    if (!target_allowed(current.target, &resolved, current.port, started_ms)) {
        finish(PICOPEN_RECON_DENIED, ERR_VAL, "OUTSIDE ACTIVE SCOPE");
        return;
    }
    if (!start_operation()) finish(PICOPEN_RECON_ERROR, ERR_IF, "START ERROR");
}

void picopen_recon_init(void) {
    current = (picopen_recon_snapshot_t){.state = PICOPEN_RECON_IDLE};
    history_count = 0u;
    history_next = 0u;
    snapshot_dirty = false;
}

bool picopen_recon_start(picopen_recon_kind_t kind, const char *target,
                         uint16_t port, uint64_t now_ms,
                         bool locally_confirmed, bool boundary_required) {
    if (!locally_confirmed || !target || !target[0] || port == 0u ||
        current.state == PICOPEN_RECON_RUNNING || now_ms < next_allowed_ms ||
        kind > PICOPEN_RECON_TLS_METADATA) return false;
    current = (picopen_recon_snapshot_t){
        .kind = kind, .state = PICOPEN_RECON_RUNNING, .port = port,
    };
    snprintf(current.target, sizeof(current.target), "%s", target);
    service_name(port, current.service, sizeof(current.service));
    require_boundary = boundary_required;
    started_ms = now_ms;
    deadline_ms = now_ms + RECON_TIMEOUT_MS;
    next_progress_ms = now_ms + RECON_PROGRESS_INTERVAL_MS;
    next_allowed_ms = now_ms + RECON_RATE_MS;
    snapshot_dirty = true;
    echo_identifier = (uint16_t)now_ms;
    ++echo_sequence;
    if (ipaddr_aton(target, &resolved)) dns_complete(target, &resolved, NULL);
    else {
        const err_t result = dns_gethostbyname(target, &resolved,
                                                dns_complete, NULL);
        if (result == ERR_OK) dns_complete(target, &resolved, NULL);
        else if (result != ERR_INPROGRESS)
            finish(PICOPEN_RECON_ERROR, result, "DNS START ERROR");
    }
    return current.state != PICOPEN_RECON_DENIED &&
           current.state != PICOPEN_RECON_ERROR;
}

bool picopen_recon_cancel(void) {
    if (current.state != PICOPEN_RECON_RUNNING) return false;
    finish(PICOPEN_RECON_CANCELLED, ERR_ABRT, "CANCELLED LOCALLY");
    return true;
}

bool picopen_recon_poll(uint64_t now_ms) {
    if (current.state != PICOPEN_RECON_RUNNING) {
        const bool changed = snapshot_dirty;
        snapshot_dirty = false;
        return changed;
    }
    current.elapsed_ms = (uint32_t)(now_ms - started_ms);
    if (!target_allowed(current.target,
                        current.address[0] ? &resolved : NULL,
                        current.port, now_ms)) {
        finish(PICOPEN_RECON_DENIED, ERR_VAL, "SCOPE EXPIRED OR REVOKED");
        snapshot_dirty = false;
        return true;
    }
    if (now_ms >= deadline_ms) {
        finish(PICOPEN_RECON_TIMEOUT, ERR_TIMEOUT, "DEADLINE EXCEEDED");
        snapshot_dirty = false;
        return true;
    }
    if (snapshot_dirty || now_ms >= next_progress_ms) {
        snapshot_dirty = false;
        next_progress_ms = now_ms + RECON_PROGRESS_INTERVAL_MS;
        return true;
    }
    return false;
}

void picopen_recon_snapshot(picopen_recon_snapshot_t *snapshot) {
    if (snapshot) *snapshot = current;
}

size_t picopen_recon_history_count(void) { return history_count; }

bool picopen_recon_history_get(size_t newest_index,
                               picopen_recon_snapshot_t *snapshot) {
    if (!snapshot || newest_index >= history_count) return false;
    const size_t index = (history_next + PICOPEN_RECON_HISTORY_CAPACITY - 1u -
                          newest_index) % PICOPEN_RECON_HISTORY_CAPACITY;
    *snapshot = history[index];
    return true;
}

const char *picopen_recon_kind_name(picopen_recon_kind_t kind) {
    static const char *const names[] = {
        "DNS", "ICMP", "TCP", "HTTP HEAD", "SSH BANNER", "TLS META",
    };
    return (unsigned)kind < sizeof(names) / sizeof(names[0])
        ? names[kind] : "UNKNOWN";
}

const char *picopen_recon_state_name(picopen_recon_state_t state) {
    static const char *const names[] = {
        "IDLE", "RUNNING", "COMPLETE", "DENIED", "TIMEOUT", "CANCELLED",
        "REFUSED", "UNAVAILABLE", "ERROR",
    };
    return (unsigned)state < sizeof(names) / sizeof(names[0])
        ? names[state] : "UNKNOWN";
}
