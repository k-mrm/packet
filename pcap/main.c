#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/if_ether.h>
#include <linux/if_packet.h>

#include "pac-net.h"

static void
hwaddrfmt(unsigned char *mac, char *str) {
  snprintf(str, 128, "%02x:%02x:%02x:%02x:%02x:%02x", 
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void
ipv4addrfmt(uint32_t ipaddr, char *str) {
  uint8_t *ip = (uint8_t *)&ipaddr;

  snprintf(str, 32, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

static void
ipv6addrfmt(struct in6_addr *ipaddr, char *str) {
  uint16_t *addr = (uint16_t *)ipaddr->s6_addr;

  snprintf(str, 64, "%x:%x:%x:%x:%x:%x:%x:%x",
           ntohs(addr[0]), ntohs(addr[1]), ntohs(addr[2]), ntohs(addr[3]),
           ntohs(addr[4]), ntohs(addr[5]), ntohs(addr[6]), ntohs(addr[7]));
}

static const char *
ip_protocol_fmt(uint8_t proto) {
  switch(proto) {
    case IPPROTO_TCP:
      return "tcp";
    case IPPROTO_UDP:
      return "udp";
    default:
      return "???";
  }
}

static void
ipv4_packet_dump(unsigned char *buf) {
  struct iphdr *iphdr = (struct iphdr *)buf;
  char dst[32] = {0};
  char src[32] = {0};

  ipv4addrfmt(iphdr->daddr, dst);
  ipv4addrfmt(iphdr->saddr, src);

  printf("ipv4 %s ---> %s %s", src, dst, ip_protocol_fmt(iphdr->protocol));
}

static void
ipv6_packet_dump(unsigned char *buf) {
  struct ip6_hdr *ip6 = (struct ip6_hdr *)buf;
  struct in6_addr *dst = &ip6->ip6_dst;
  struct in6_addr *src = &ip6->ip6_src;
  char d_str[64];
  char s_str[64];

  ipv6addrfmt(dst, d_str);
  ipv6addrfmt(src, s_str);

  printf("ipv6 %s ---> %s", s_str, d_str);
}

static void
packetdump(unsigned char *buf, size_t size) {
  struct ether_header *eth = (struct ether_header *)buf;
  unsigned short type = ntohs(eth->ether_type);

  buf += sizeof(struct ether_header);

  switch(type) {
    case 0x0800:    // ipv4
      ipv4_packet_dump(buf);
      break;
    case 0x86dd:    // ipv6
      ipv6_packet_dump(buf);
      break;
  }

  printf(" size %ld\n", size);
}

static int
pcap(struct netif *netif) {
  struct pollfd target;
  unsigned char buf[2048];
  size_t size;

  target.fd = netif->socket;
  target.events = POLLIN | POLLERR;

  for(;;) {
    switch(poll(&target, 1, 2000)) {
      case 0:
        break;
      case -1:
        return -1;

      default:
        if(target.revents & (POLLIN | POLLERR)) {
          if((size = read(netif->socket, buf, sizeof(buf))) <= 0) {
            netif_err(netif, "read");
            break;
          }
          packetdump(buf, size);
        }
        break;
    }
  }

  return 0;
}

int
main(int argc, char **argv) {
  struct netif *netif;

  if(argc != 2) {
    fprintf(stderr, "pcap <if1>\n");
    return -1;
  }

  netif = init_netif(argv[1], 1);
  if(!netif)
    panic("?");

  disable_ip_forward();

  return pcap(netif);
}
