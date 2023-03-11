#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <linux/if_packet.h>

#include "pac-net.h"

static void
hwaddrfmt(unsigned char *mac, char *str) {
  snprintf(str, 128, "%02x:%02x:%02x:%02x:%02x:%02x", 
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void
packetdump(unsigned char *buf, size_t size) {
  struct ether_header *eth = (struct ether_header *)buf;
  char dst[128] = {0};
  char src[128] = {0};

  hwaddrfmt(eth->ether_dhost, dst);
  hwaddrfmt(eth->ether_shost, src);

  printf("dst %s src %s type %#x\n", dst, src, ntohs(eth->ether_type));
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
