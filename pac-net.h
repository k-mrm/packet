#ifndef PAC_NET_H
#define PAC_NET_H

struct netif {
  int socket;
  struct ifreq *ifr;
  const char *ifname;
  unsigned char hwaddr[6];
};

void panic(const char *why);
void netif_err(struct netif *netif, const char *msg);
int netif_set_ifflags(struct netif *netif, int flags);
int netif_set_macaddr(struct netif *netif);
void hwaddrdump(unsigned char *hwaddr);
struct netif *init_netif(const char *ifname, int promisc);
int disable_ip_forward(void);
void packetwrite(struct netif *netif, unsigned char *buf, size_t size);

#endif
