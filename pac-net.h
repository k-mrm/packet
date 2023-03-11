#ifndef PAC_NET_H
#define PAC_NET_H

struct netif {
  int socket;
  struct ifreq *ifr;
  const char *ifname;
  unsigned char hwaddr[6];
};

struct l2sw {
  struct netif *ifs[16];
  int ifs_idx;
};

#endif
