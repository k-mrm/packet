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

struct l2sw {
  struct netif *ifs[16];
  int ifs_idx;
};

static void
switchloop(struct l2sw *l2sw) {
  struct pollfd targets[16];
  unsigned char buf[2048];
  int nready, ntargets = l2sw->ifs_idx;
  size_t size;
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
            packetwrite(l2sw->ifs[i], buf, 64);
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
