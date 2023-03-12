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
#include <netinet/if_ether.h>
#include <linux/if_packet.h>

#include "pac-net.h"

static void
hwaddrfmt(unsigned char *mac, char *str) {
  snprintf(str, 128, "%02x:%02x:%02x:%02x:%02x:%02x", 
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void
ipaddrfmt(uint32_t ipaddr, char *str) {
  uint8_t *ip = (uint8_t *)&ipaddr;

  snprintf(str, 32, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
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
ipv4_packet_dump(unsigned char *buf, size_t size) {
  struct iphdr *iphdr = (struct iphdr *)buf;
  char dst[32] = {0};
  char src[32] = {0};

  ipaddrfmt(iphdr->daddr, dst);
  ipaddrfmt(iphdr->saddr, src);

  printf("ipv4 %s ---> %s %s\n", src, dst, ip_protocol_fmt(iphdr->protocol));
}

static void
ipv6_packet_dump(unsigned char *buf, size_t size) {
  ;
}

static void
packetdump(unsigned char *buf, size_t size) {
  struct ether_header *eth = (struct ether_header *)buf;
  unsigned short type = ntohs(eth->ether_type);

  buf += sizeof(struct ether_header);

  switch(type) {
    case 0x0800:    // ipv4
      ipv4_packet_dump(buf, size);
      break;
    case 0x86dd:    // ipv6
      ipv6_packet_dump(buf, size);
      break;
  }
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
