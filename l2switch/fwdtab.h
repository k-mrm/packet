#ifndef PAC_FWDTAB_H
#define PAC_FWDTAB_H

#include <stdint.h>
#include "pac-net.h"

struct fwdtab_ent {
  uint8_t mac[6];
  struct netif *netif;
};

struct fwdtab {
  struct fwdtab_ent ents[256];
  int nents;
};

void insert(struct fwdtab *tab, struct netif *netif, uint8_t *mac);
struct netif *search(struct fwdtab *tab, uint8_t *mac);

#endif
