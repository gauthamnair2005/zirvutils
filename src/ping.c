#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define NET_DEV "/zirv/net/eth0"

/* ── Packet structures ───────────────────────────────────────────────────── */
struct eth_hdr {
    uint8_t  dst[6];
    uint8_t  src[6];
    uint16_t type;
} __attribute__((packed));

struct arp_pkt {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t  hw_len;
    uint8_t  proto_len;
    uint16_t op;
    uint8_t  sender_mac[6];
    uint8_t  sender_ip[4];
    uint8_t  target_mac[6];
    uint8_t  target_ip[4];
} __attribute__((packed));

struct ip_hdr {
    uint8_t  ver_ihl;
    uint8_t  dscp_ecn;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_frag;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dst_ip;
} __attribute__((packed));

struct icmp_hdr {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} __attribute__((packed));

/* ── Constants ───────────────────────────────────────────────────────────── */
#define ETH_TYPE_ARP  0x0608
#define ETH_TYPE_IP   0x0008
#define ARP_OP_REQ    0x0100
#define ARP_OP_REP    0x0200
#define IP_PROTO_ICMP 1
#define ICMP_ECHO     8
#define ICMP_REPLY    0

#define BROADCAST_MAC  {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}
static const uint8_t g_bcast_mac[6] = BROADCAST_MAC;

/* Our assumed MAC (filled from device) and IP */
static uint8_t  g_our_mac[6];
static uint32_t g_our_ip  = (10u << 24) | (0 << 16) | (2 << 8) | 15;
static uint8_t  g_target_mac[6];

static int g_fd = -1;

/* ── IP checksum ─────────────────────────────────────────────────────────── */
static uint16_t ip_checksum(const void *buf, size_t len)
{
    uint32_t sum = 0;
    const uint16_t *p = (const uint16_t *)buf;
    while (len > 1) { sum += *p++; len -= 2; }
    if (len) sum += *(const uint8_t *)p;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return (uint16_t)~sum;
}

/* ─── IP string to uint32 ────────────────────────────────────────────────── */
static uint32_t parse_ip(const char *s)
{
    uint32_t ip = 0;
    int v = 0, shift = 24;
    while (*s) {
        if (*s == '.') { ip |= (uint32_t)(v & 0xFF) << shift; shift -= 8; v = 0; }
        else if (*s >= '0' && *s <= '9') v = v * 10 + (*s - '0');
        else return 0;
        s++;
    }
    ip |= (uint32_t)(v & 0xFF) << shift;
    return ip;
}

static void ip_to_str(uint32_t ip, char *buf)
{
    snprintf(buf, 64, "%u.%u.%u.%u",
        (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
        (ip >> 8) & 0xFF, ip & 0xFF);
}

/* ─── Get our MAC — fallback; updated from ARP reply dst on first RX ──────── */
static int get_our_mac(void)
{
    /* Use a non-zero fallback so ARP requests are well-formed.  The real MAC
       is learned later from the first ARP reply's Ethernet destination. */
    g_our_mac[0] = 0x02; g_our_mac[1] = 0x00;
    g_our_mac[2] = 0x00; g_our_mac[3] = 0x00;
    g_our_mac[4] = 0x00; g_our_mac[5] = 0x01;
    return 0;
}

/* ─── ARP resolve: get MAC for target IP ─────────────────────────────────── */
static int arp_resolve(uint32_t target_ip)
{
    uint8_t buf[sizeof(struct eth_hdr) + sizeof(struct arp_pkt)];
    struct eth_hdr *eth = (struct eth_hdr *)buf;
    struct arp_pkt *arp = (struct arp_pkt *)(eth + 1);

    memcpy(eth->dst, g_bcast_mac, 6);
    memcpy(eth->src, g_our_mac, 6);
    eth->type = ETH_TYPE_ARP;

    arp->hw_type    = 1;
    arp->proto_type = 0x0008;
    arp->hw_len     = 6;
    arp->proto_len  = 4;
    arp->op         = ARP_OP_REQ;
    memcpy(arp->sender_mac, g_our_mac, 6);
    arp->sender_ip[0] = (g_our_ip >> 24) & 0xFF;
    arp->sender_ip[1] = (g_our_ip >> 16) & 0xFF;
    arp->sender_ip[2] = (g_our_ip >> 8) & 0xFF;
    arp->sender_ip[3] = g_our_ip & 0xFF;
    memset(arp->target_mac, 0, 6);
    arp->target_ip[0] = (target_ip >> 24) & 0xFF;
    arp->target_ip[1] = (target_ip >> 16) & 0xFF;
    arp->target_ip[2] = (target_ip >> 8) & 0xFF;
    arp->target_ip[3] = target_ip & 0xFF;

    write(g_fd, buf, sizeof(buf));

    for (int i = 0; i < 5000; i++) {
        uint8_t rbuf[2048];
        int n = read(g_fd, rbuf, sizeof(rbuf));
        if (n < (int)(sizeof(struct eth_hdr) + sizeof(struct arp_pkt))) continue;

        struct eth_hdr *re = (struct eth_hdr *)rbuf;
        if (re->type != ETH_TYPE_ARP) continue;

        struct arp_pkt *ra = (struct arp_pkt *)(re + 1);
        if (ra->op != ARP_OP_REP) continue;

        uint32_t resp_ip = (uint32_t)ra->sender_ip[0] << 24 |
                          (uint32_t)ra->sender_ip[1] << 16 |
                          (uint32_t)ra->sender_ip[2] << 8  |
                          (uint32_t)ra->sender_ip[3];
        if (resp_ip != target_ip) continue;

        memcpy(g_target_mac, ra->sender_mac, 6);
        /* Learn our own MAC from the Ethernet dst of the received frame */
        if (g_our_mac[0] == 0 && g_our_mac[1] == 0 && g_our_mac[2] == 0)
            memcpy(g_our_mac, re->dst, 6);
        return 0;
    }
    return -1;
}

/* ─── Send ICMP echo request, wait for reply ─────────────────────────────── */
static int ping_send_recv(uint32_t target_ip, int seq, int *got_reply)
{
    *got_reply = 0;
    uint8_t pkt[sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct icmp_hdr) + 48];
    struct eth_hdr *eth = (struct eth_hdr *)pkt;
    struct ip_hdr  *ip  = (struct ip_hdr *)(eth + 1);
    struct icmp_hdr *icmp = (struct icmp_hdr *)(ip + 1);
    char *payload = (char *)(icmp + 1);

    int total_ip = sizeof(struct ip_hdr) + sizeof(struct icmp_hdr) + 48;
    int total_pkt = sizeof(struct eth_hdr) + total_ip;

    memcpy(eth->dst, g_target_mac, 6);
    memcpy(eth->src, g_our_mac, 6);
    eth->type = ETH_TYPE_IP;

    memset(ip, 0, sizeof(struct ip_hdr));
    ip->ver_ihl    = 0x45;
    ip->total_len  = (uint16_t)((total_ip >> 8) | (total_ip << 8));
    ip->id         = (uint16_t)((seq >> 8) | (seq << 8));
    ip->ttl        = 64;
    ip->protocol   = IP_PROTO_ICMP;
    ip->src_ip     = (g_our_ip >> 24) | ((g_our_ip >> 8) & 0xFF00) |
                     ((g_our_ip << 8) & 0xFF0000) | (g_our_ip << 24);
    ip->dst_ip     = (target_ip >> 24) | ((target_ip >> 8) & 0xFF00) |
                     ((target_ip << 8) & 0xFF0000) | (target_ip << 24);
    ip->checksum   = 0;
    ip->checksum   = ip_checksum(ip, sizeof(struct ip_hdr));

    icmp->type     = ICMP_ECHO;
    icmp->code     = 0;
    icmp->checksum = 0;
    icmp->id       = 0x0100;
    icmp->seq      = (uint16_t)((seq >> 8) | (seq << 8));
    memset(payload, 'Z', 48);
    icmp->checksum = ip_checksum(icmp, sizeof(struct icmp_hdr) + 48);

    write(g_fd, pkt, total_pkt);

    for (int i = 0; i < 5000; i++) {
        uint8_t rbuf[2048];
        int n = read(g_fd, rbuf, sizeof(rbuf));
        if (n < (int)(sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct icmp_hdr)))
            continue;

        struct eth_hdr *re = (struct eth_hdr *)rbuf;
        if (re->type != ETH_TYPE_IP) continue;

        struct ip_hdr *ri = (struct ip_hdr *)(re + 1);
        if (ri->protocol != IP_PROTO_ICMP) continue;

        struct icmp_hdr *ric = (struct icmp_hdr *)(ri + 1);
        if (ric->type != ICMP_REPLY) continue;
        if (ric->id != 0x0100) continue;

        *got_reply = 1;
        return n;
    }
    return -1;
}

/* ─── Main ───────────────────────────────────────────────────────────────── */
int main(int argc, char *argv[])
{
    uint32_t target;
    char target_str[64];
    int count = 4;

    if (argc >= 2) {
        target = parse_ip(argv[1]);
        if (target == 0) {
            /* Not a dotted IP — try DNS lookup */
            target = dns_lookup(argv[1]);
            if (target == 0) {
                printf("ping: bad IP '%s' and DNS lookup failed\n", argv[1]);
                return 1;
            }
            printf("ping: %s resolved to %u.%u.%u.%u\n", argv[1],
                (target >> 24) & 0xFF, (target >> 16) & 0xFF,
                (target >> 8) & 0xFF, target & 0xFF);
        }
    } else {
        target = (10u << 24) | (0 << 16) | (2 << 8) | 2; /* 10.0.2.2 */
        printf("ping: no target given, trying 10.0.2.2 (gateway)\n");
    }
    ip_to_str(target, target_str);

    g_fd = open(NET_DEV, 0);
    if (g_fd < 0) {
        printf("ping: cannot open %s\n", NET_DEV);
        return 1;
    }

    get_our_mac();

    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
        g_our_mac[0], g_our_mac[1], g_our_mac[2],
        g_our_mac[3], g_our_mac[4], g_our_mac[5]);

    printf("PING %s (%s) from %s\n", target_str, target_str, mac_str);

    if (arp_resolve(target) < 0) {
        printf("ping: %s: no ARP reply\n", target_str);
        close(g_fd);
        return 1;
    }

    char tmac[18];
    snprintf(tmac, sizeof(tmac), "%02x:%02x:%02x:%02x:%02x:%02x",
        g_target_mac[0], g_target_mac[1], g_target_mac[2],
        g_target_mac[3], g_target_mac[4], g_target_mac[5]);
    printf("ARP resolved %s at %s\n", target_str, tmac);

    /* Fallback: if we still don't know our MAC, fabricate one */
    if (g_our_mac[0] == 0 && g_our_mac[1] == 0 && g_our_mac[2] == 0) {
        g_our_mac[0] = 0x02; g_our_mac[1] = 0x00;
        g_our_mac[2] = 0x00; g_our_mac[3] = 0x00;
        g_our_mac[4] = 0x00; g_our_mac[5] = 0x01;
    }

    int sent = 0, received = 0;
    for (int seq = 1; seq <= count; seq++) {
        int got = 0;
        int n = ping_send_recv(target, seq, &got);
        sent++;
        if (got) {
            received++;
            printf("%d bytes from %s: icmp_seq=%d time=~0ms\n", n, target_str, seq);
        } else {
            printf("Request timeout for icmp_seq=%d\n", seq);
        }
    }

    printf("\n--- %s ping statistics ---\n", target_str);
    printf("%d packets transmitted, %d received, %d%% packet loss\n",
           sent, received, sent ? (sent - received) * 100 / sent : 0);

    close(g_fd);
    return (received > 0) ? 0 : 1;
}
