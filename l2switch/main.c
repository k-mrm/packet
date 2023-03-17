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
#include "fwdtab.h"

struct l2sw {
  struct netif *ifs[16];
  int ifs_idx;
  struct fwdtab fwdtab;
};

static uint8_t bcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static ssize_t
netif_send_packet(struct netif *netif, uint8_t *packet, size_t size) {
  return write(netif->socket, packet, size);
}

static int
l2bcast(struct l2sw *sw, struct netif *from, uint8_t *packet, size_t size) {
  for(int i = 0; i < sw->ifs_idx; i++) {
    struct netif *netif = sw->ifs[i];
    if(netif != from) {
      if(netif_send_packet(netif, packet, size) < 0) {
        netif_err(netif, "send");
        return -1;
      }
    }
  }

  return 0;
}

static void
packetswitch(struct l2sw *sw, struct netif *from, unsigned char *packet, size_t size) {
  struct ether_header *eth = (struct ether_header *)packet;
  ssize_t sz;

  learn(&sw->fwdtab, from, eth->ether_shost);

  if(memcmp(eth->ether_dhost, bcast_mac, 6) == 0) {
    // bcast
    if(l2bcast(sw, from, packet, size) < 0)
      netif_err(from, "bcast");
  } else {
    // unicast
    struct netif *to = search(&sw->fwdtab, eth->ether_dhost);
    if(to) {
      if((sz = write(to->socket, packet, size)) <= 0)
        netif_err(to, "write");
    } else {
      if(l2bcast(sw, from, packet, size) < 0)
        netif_err(from, "bcast");
    }
  }
}

static void
switchloop(struct l2sw *l2sw) {
  struct pollfd targets[16];
  unsigned char buf[2048];
  int nready, ntargets = l2sw->ifs_idx;
  ssize_t size;
  // initialize targets
  for(int i = 0; i < ntargets; i++) {
    struct pollfd *fd = &targets[i];
    fd->fd = l2sw->ifs[i]->socket;
    fd->events = POLLIN | POLLERR;
  }
  // main loop
  for(;;) {
    switch(nready = poll(targets, ntargets, 2000)) {
      case 0:
        break;
      case -1:
        return;

      default:
        for(int i = 0; i < ntargets; i++) {
          if(targets[i].revents & (POLLIN | POLLERR)) {
            if((size = read(l2sw->ifs[i]->socket, buf, sizeof(buf))) <= 0) {
              netif_err(l2sw->ifs[i], "read");
              break;
            }
            packetswitch(l2sw, l2sw->ifs[i], buf, size);
          }
        }
        break;
    }
  }
}

int
main(int argc, char **argv) {
  struct l2sw l2sw = {0};

  if(argc < 3) {
    fprintf(stderr, "l2sw <if1> <if2> ...\n");
    return -1;
  }

  for(int i = 1; i < argc; i++) {
    struct netif *netif = init_netif(argv[i], 1);
    if(!netif)
      panic("netif!");

    hwaddrdump(netif->hwaddr);
    l2sw.ifs[l2sw.ifs_idx++] = netif;
  }

  disable_ip_forward();

  switchloop(&l2sw);
  
  return 0;
}
