#include <kern/e1000.h>
#include <kern/pmap.h>

volatile char *e1000;

// LAB 6: Your driver code here
int e1000_82540EM_attach_function(struct pci_func *f) {
  pci_func_enable(f);
  e1000 = mmio_map_region(f->reg_base[0], f->reg_size[0]);
  return 0;
}