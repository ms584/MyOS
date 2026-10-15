#ifndef MYOS_USB_H
#define MYOS_USB_H
#include "os.h"
/* Bounded polling, root-attached boot mouse and SCSI/BOT storage. */
void usb_init(void);
void usb_tick(void);
bool usb_bulk_transfer(void *handle,bool input,void *buffer,unsigned length);
void usb_storage_attach(void *handle);
bool usb_storage_present(void);
uint32_t usb_storage_sectors(void);
bool usb_storage_read(uint32_t,void *);
bool usb_storage_write(uint32_t,const void *);
bool usb_storage_flush(void);
#endif
