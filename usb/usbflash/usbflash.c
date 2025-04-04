#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/device.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NGUYEN ANH");
MODULE_DESCRIPTION("Flash USB driver");
MODULE_VERSION("0.1");

static struct flash 
{
    char name[64];
    char phys[64];
    struct usb_device *udev;
    struct usb_interface *interface;
    const struct usb_device_id *id;
    struct urb* urb;
    dma_addr_t dma;
    char* data;
    unsigned int packet_size;
    struct input_dev *dev;
};

struct flash *flashdev;

static int skel_probe() {
    struct usb_host_interface* iface_decs;
    struct usb_endpoint_descriptor *endpoint;
    unsigned int pipe;

    flashdev = kzalloc(sizeof(struct flash), GFP_KERNEL); 
        

}
