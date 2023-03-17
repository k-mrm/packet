#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "fwdtab.h"

int
learn(struct fwdtab *tab, struct netif *netif, uint8_t *mac) {
  struct fwdtab_ent *ent;

  if(search(tab, mac))
    return 0;
  if(tab->nents == 256)
    return -1;

  ent = &tab->ents[tab->nents++];

  memcpy(ent->mac, mac, 6);
  ent->netif = netif;

  return 0;
}

struct netif *
search(struct fwdtab *tab, uint8_t *mac) {
  for(struct fwdtab_ent *ent = tab->ents;
      ent < &tab->ents[tab->nents]; ent++) {
    if(memcmp(ent->mac, mac, 6) == 0)
      return ent->netif;
  }

  return NULL;
}
