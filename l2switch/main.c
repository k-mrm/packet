#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <linux/if_packet.h>

#include "netdev.h"

static void
panic(const char *why) {
  fprintf(stderr, "%s\n", why);
  exit(1);
}

static void
netif_err(struct netif *netif, const char *msg) {
  fprintf(stderr, "%s(%d): ", netif->ifname, netif->socket);
  perror(msg);
}

static int
netif_set_ifflags(struct netif *netif, int flags) {
  if(ioctl(netif->socket, SIOCGIFFLAGS, netif->ifr) < 0) {
    netif_err(netif, "gifflags");
    return -1;
  }

  netif->ifr->ifr_flags |= flags;

  if(ioctl(netif->socket, SIOCSIFFLAGS, netif->ifr) < 0) {
    netif_err(netif, "sifflags");
    return -1;
  }

  return 0;
}

static int
netif_set_macaddr(struct netif *netif) {
  if(ioctl(netif->socket, SIOCGIFHWADDR, netif->ifr) < 0)
    return -1;
  memcpy(netif->hwaddr, netif->ifr->ifr_hwaddr.sa_data, 6);

  return 0;
}

static void
hwaddrdump(unsigned char *hwaddr) {
  printf("macaddr: %2x:%2x:%2x:%2x:%2x:%2x\n",
         hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
}

static struct netif *
init_netif(const char *ifname, int promisc) {
  int soc;
  struct sockaddr_ll sa;
  struct netif *netif = malloc(sizeof(*netif));
  struct ifreq *ifr = malloc(sizeof(*netif->ifr));

  if(!netif || !ifr)
    return NULL;

  netif->ifname = ifname;
  netif->ifr = ifr;

  if((soc = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
    netif_err(netif, "socket");
    goto err;
  }
  netif->socket = soc;

  strncpy(ifr->ifr_name, ifname, sizeof(ifr->ifr_name) - 1);
  if(ioctl(soc, SIOCGIFINDEX, ifr) < 0) {
    netif_err(netif, "ioctl");
    goto err;
  }

  sa.sll_family = PF_PACKET;
  sa.sll_protocol = htons(ETH_P_ALL);
  sa.sll_ifindex = ifr->ifr_ifindex;

  if(bind(soc, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
    netif_err(netif, "bind");
    goto err;
  }

  if(promisc) {
    if(netif_set_ifflags(netif, IFF_PROMISC) < 0)
      goto err;
  }

  netif_set_macaddr(netif);

  return netif;

err:
  free(netif);
  free(ifr);
  close(soc);

  return NULL;
}

static int
disable_ip_forward() {
  FILE *fp;

  if(!(fp = fopen("/proc/sys/net/ipv4/ip_forward", "w")))
    return -1;

  fputs("0", fp);
  fclose(fp);

  return 0;
}

static void
switchloop(struct l2sw *l2sw) {
  ;
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
