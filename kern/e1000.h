#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

#define E1000_DEVICE_ID_82540EM 0x100E
#define E1000_VENDOR_ID_82540EM 0x8086

#define E1000_STATUS   0x00008

int e1000_82540EM_attach_function(struct pci_func *);

#endif	// JOS_KERN_E1000_H
