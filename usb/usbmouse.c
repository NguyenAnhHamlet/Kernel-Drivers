#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>
#include <linux/fs.h>
//#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/device.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NGUYEN ANH");
MODULE_DESCRIPTION("Mouse USB driver");
MODULE_VERSION("0.1");

static struct mouse
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


static struct usb_device_id skel_table[] =
{
    {USB_DEVICE(0x046d ,0xc52b)},  
    {}
};

MODULE_DEVICE_TABLE(usb, skel_table );

void complete_handler(struct urb * urb);
static int skel_probe(struct usb_interface *interface, const struct usb_device_id *id);
static ssize_t usb_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos);
static int mouse_open(struct input_dev* dev);
static void mouse_close(struct input_dev *dev);
static void skel_disconnect(struct usb_interface *interface);

static struct usb_driver skel_driver = 
{
    .name = "usb_mouse_driver",
    .id_table = skel_table,
    .probe = skel_probe,
    .disconnect = skel_disconnect,
};

static struct file_operations usbdriver_fops = 
{
	.read = usb_read,
	.write = NULL,
	.open = NULL,
	.release = NULL,
};


static int skel_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    struct usb_host_interface* iface_desc;
    struct usb_endpoint_descriptor *endpoint;
    unsigned int pipe;
    
    struct mouse* mousedev = kzalloc(sizeof(struct mouse), GFP_KERNEL);

    mousedev->urb = usb_alloc_urb(0, GFP_ATOMIC);
    if(!mousedev->urb)
    {
        pr_err(KERN_ERR "Failed to allocate memory for mousedev->urb");
        return -ENOMEM;
    }

    mousedev->udev = interface_to_usbdev(interface);
    if(!mousedev->udev)
    {
        pr_err(KERN_ERR "Failed to retrieve usb_device");
        return -ENODEV;
    }
    
    // in endpoint for mouse device
    iface_desc = interface->cur_altsetting;
    int i = 0;
    for (i = 0; i < iface_desc->desc.bNumEndpoints; i++)
    {
        endpoint = &iface_desc->endpoint[i].desc;
        if(usb_endpoint_is_int_in(endpoint))
        {
            break;  
        }
    }
    
    pipe = usb_rcvintpipe(mousedev->udev, endpoint->bEndpointAddress);
    mousedev->data = usb_alloc_coherent(mousedev->udev, 8, GFP_ATOMIC, &mousedev->dma);
    if(!mousedev->data)
    {
        pr_err("usb_alloc_coherent failed\n");
        return -ENOMEM;
    }

    mousedev->packet_size = usb_maxpacket(mousedev->udev, pipe);
    mousedev->dev = input_allocate_device();
    usb_make_path(mousedev->udev, mousedev->phys, sizeof(mousedev->phys));    
    strlcat(mousedev->phys, "/input0", sizeof(mousedev->phys));
    
    mousedev->dev->name = mousedev->name;
	mousedev->dev->phys = mousedev->phys;

    // set relationship between usb device and input device
    usb_to_input_id(mousedev->udev, &mousedev->dev->id);
    mousedev->dev->dev.parent = &interface->dev;

    // relative movement and key events
    mousedev->dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);

    // supported button
    mousedev->dev->keybit[BIT_WORD(BTN_MOUSE)] = BIT_MASK(BTN_LEFT) |
		             BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_MIDDLE);
    
    // x and y coordination and wheel
    mousedev->dev->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y) | BIT_MASK(REL_WHEEL);

    // side button and additional buttons
    mousedev->dev->keybit[BIT_WORD(BTN_MOUSE)] |= BIT_MASK(BTN_SIDE) |
		              BIT_MASK(BTN_EXTRA);
    
    
    mousedev->dev->open = mouse_open;
    mousedev->dev->close = mouse_close;

    input_set_drvdata(mousedev->dev, mousedev);

    usb_fill_int_urb(mousedev->urb, mousedev->udev, pipe, mousedev->data, 8, 
                     complete_handler,  mousedev, endpoint->bInterval);
    
    mousedev->urb->transfer_dma = mousedev->dma;
	mousedev->urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

    input_register_device(mousedev->dev);

    int retcode = register_chrdev(0, "mouse", &usbdriver_fops);
	if(retcode<0) 
	{
		pr_info("mouse registration failed\n");
	}
	else 
	{
		pr_info("mouse registration successful\n");
	}

	return 0;

}

void complete_handler(struct urb * urb)
{
    struct mouse* mousedev = urb->context;
	char *data = mousedev->data;
	struct input_dev *dev = mousedev->dev;
	int status;

	switch (urb->status) {
	case 0:			/* success */
        input_sync(dev);
		break;
	case -ECONNRESET:	/* unlink */
	case -ENOENT:
	case -ESHUTDOWN:
		return;
	default:		/* error */
        break;
	}    

    status = usb_submit_urb(urb, GFP_ATOMIC);
    if(status)
    {
        pr_err("Failed to submit");
    }

    if(data[1] & 0x01)
    {
		pr_info("Left mouse button clicked!\n");
		
	}
	else if(data[1] & 0x02)
    {
		pr_info("Right mouse button clicked!\n");
	}
	else if(data[1] & 0x04)
    {
		pr_info("Wheel button clicked!\n");
	}
	else if(data[5])
    {
		pr_info("Wheel moves!\n");
	}
}

static ssize_t usb_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos)
{
	printk(KERN_ALERT "Reading device...\n");
	return 0;
} 

static int mouse_open(struct input_dev* dev)
{
    struct mouse *mousedev;
    mousedev = input_get_drvdata(dev);
    if(!mousedev)
	{
		printk(KERN_ALERT "no mouse\n");
		return -1;
	}
	if(!mousedev->udev)
    {
		printk(KERN_ALERT "no mouse->usbdev\n");
		return -1;
	}
	if(!mousedev->urb)
    {
		printk(KERN_ALERT "No mouse->urb\n");
		return -1;
	}

    printk("RUNNNING");
    
    // set the usb_device for urb
    mousedev->urb->dev = mousedev->udev;

    int ret = usb_submit_urb(mousedev->urb, GFP_KERNEL);

    if (ret) 
    {
        printk(KERN_ALERT "Failed submitting urb: error code %d\n", ret);
    }

    printk("Mouse open");

    return 0;
}

static void mouse_close(struct input_dev *dev)
{
	struct mouse *mousedev = input_get_drvdata(dev);

	usb_kill_urb(mousedev->urb);
}

static void skel_disconnect(struct usb_interface *interface)
{
	printk(KERN_ALERT "USB disconnecting\n");
	usb_deregister_dev(interface, NULL);
	unregister_chrdev(0, "mymouse");
}

static int __init usb_skel_init(void)
{
        int result;

        /* register this driver with the USB subsystem */
        result = usb_register(&skel_driver);
        if (result < 0) {
                pr_err("usb_register failed for the %s driver. Error number %d\n",
                       skel_driver.name, result);
                return -1;
        }

        return 0;
}

static void __exit usb_skel_exit(void)
{
    /* deregister this driver with the USB subsystem */
    usb_deregister(&skel_driver);
    
}

module_init(usb_skel_init);
module_exit(usb_skel_exit);
