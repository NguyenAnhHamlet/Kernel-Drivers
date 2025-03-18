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
    {USB_DEVICE(0x30fa ,0x1701)},  
    {}
};

static const int button_map[17] = 
{
    [0 ... 16] = 0,

    [0x01] = BTN_LEFT, [0x02] = BTN_RIGHT, [0x04] = BTN_MIDDLE,
    [0x08] = BTN_SIDE, [0x10] = BTN_EXTRA 
}; 

MODULE_DEVICE_TABLE(usb, skel_table );

void complete_handler(struct urb * urb);
static int skel_probe(struct usb_interface *interface, const struct usb_device_id *id);
static int mouse_open(struct input_dev* dev);
static void mouse_close(struct input_dev *dev);
static void skel_disconnect(struct usb_interface *interface);
static struct mouse  *mousedev;

static struct usb_driver skel_driver = 
{
    .name = "usb_mouse_driver",
    .id_table = skel_table,
    .probe = skel_probe,
    .disconnect = skel_disconnect,
};

static void setup_mouse_keymap(struct mouse *mousedev)
{
    __set_bit(BTN_LEFT, mousedev->dev->keybit);
    __set_bit(BTN_RIGHT, mousedev->dev->keybit);
    __set_bit(BTN_MIDDLE, mousedev->dev->keybit);
    __set_bit(BTN_SIDE, mousedev->dev->keybit);     // Side button (if available)
    __set_bit(BTN_EXTRA, mousedev->dev->keybit);    // Extra button (if available)
    __set_bit(REL_X, mousedev->dev->relbit);        // X-axis movement
    __set_bit(REL_Y, mousedev->dev->relbit);        // Y-axis movement
    __set_bit(REL_WHEEL, mousedev->dev->relbit);    // Scroll wheel
}

static int hid_to_linux_code(uint8_t hid)
{
    return button_map[hid];
}

static int skel_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    struct usb_host_interface* iface_desc;
    struct usb_endpoint_descriptor *endpoint;
    unsigned int pipe;
    
    mousedev = kzalloc(sizeof(struct mouse), GFP_KERNEL);

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
    strlcat(mousedev->phys, "/mouse0", sizeof(mousedev->phys));
    
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
    
    setup_mouse_keymap(mousedev);
    
    mousedev->dev->open = mouse_open;
    mousedev->dev->close = mouse_close;

    input_set_drvdata(mousedev->dev, mousedev);

    usb_fill_int_urb(mousedev->urb, mousedev->udev, pipe, mousedev->data, 8, 
                     complete_handler,  mousedev, endpoint->bInterval);
    
    mousedev->urb->transfer_dma = mousedev->dma;
	mousedev->urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

    input_register_device(mousedev->dev);
    usb_set_intfdata(interface, mousedev);

	return 0;
}

void complete_handler(struct urb * urb)
{
    struct mouse* mousedev = urb->context;
	char *data = mousedev->data;
	int status;

    printk("RUNNNING HERE");

	switch (urb->status) 
    {
	case 0:			/* success */
		break;
	case -ECONNRESET:	/* unlink */
	case -ENOENT:
	case -ESHUTDOWN:
		return;
	default:		/* error */
        break;
	}    

    

    printk(KERN_INFO "USB Data: %02X %02X %02X %02X %02X %02X %02X %02X\n",
    data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

    int clicked = hid_to_linux_code(data[1]);
    int rel_move_mouse = data[3] | data[5];  
    int rel_move_wheel = data[6];  

    if(clicked) 
    {
        input_report_key(mousedev->dev, clicked, 1);
        input_report_key(mousedev->dev, clicked, 0);
    }

    if (rel_move_wheel)
    {
       input_report_rel(mousedev->dev, REL_WHEEL, (int8_t)data[6]); 
    }

    if (rel_move_mouse)
    {
        input_report_rel(mousedev->dev, REL_X, (int8_t) data[2]);
        input_report_rel(mousedev->dev, REL_Y, (int8_t) data[3]);
    }

    input_sync(mousedev->dev);

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
	else if(data[6])
    {
		pr_info("Wheel moves!\n");
	}
    
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
	if(mousedev->udev == NULL)
        usb_kill_urb(mousedev->urb);
    printk(KERN_ALERT "Mouse close");
}

static void skel_disconnect(struct usb_interface *interface)
{
	printk(KERN_ALERT "USB disconnecting\n");
    struct mouse* mousedev = usb_get_intfdata(interface);
    usb_free_urb(mousedev->urb);
    usb_free_coherent(mousedev->udev, 8, mousedev->data, mousedev->dma);
    input_unregister_device(mousedev->dev);
    kfree(mousedev);
    usb_set_intfdata(interface, NULL);
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
