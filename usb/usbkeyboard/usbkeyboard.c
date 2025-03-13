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
MODULE_DESCRIPTION("Keyboard USB driver");
MODULE_VERSION("0.1");

const uint8_t keys[] = {0x02, 0x04, 0x16, 0x07};
const uint8_t device_id[2] = {}; 


static struct keyboard
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
    // TODO: add infos
    {USB_DEVICE(device_id[0] ,device_id[1])},  
    {}
};

MODULE_DEVICE_TABLE(usb, skel_table );

void complete_handler(struct urb * urb);
static int skel_probe(struct usb_interface *interface, const struct usb_device_id *id);
static int keyboard_open(struct input_dev* dev);
static void keyboard_close(struct input_dev *dev);
static void skel_disconnect(struct usb_interface *interface);
static struct keyboard *keyboarddev;

static struct usb_driver skel_driver = 
{
    .name = "usb_keyboard_driver",
    .id_table = skel_table,
    .probe = skel_probe,
    .disconnect = skel_disconnect,
};

static int skel_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    struct usb_host_interface* iface_desc;
    struct usb_endpoint_descriptor *endpoint;
    unsigned int pipe;
    
    keyboarddev = kzalloc(sizeof(struct keyboard), GFP_KERNEL);

    keyboarddev->urb = usb_alloc_urb(0, GFP_ATOMIC);
    if(!keyboarddev->urb)
    {
        pr_err(KERN_ERR "Failed to allocate memory for keyboarddev->urb");
        return -ENOMEM;
    }

    keyboarddev->udev = interface_to_usbdev(interface);
    if(!keyboarddev->udev)
    {
        pr_err(KERN_ERR "Failed to retrieve usb_device");
        return -ENODEV;
    }
    
    // in endpoint for keyboard device
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
    
    pipe = usb_rcvintpipe(keyboarddev->udev, endpoint->bEndpointAddress);
    keyboarddev->data = usb_alloc_coherent(keyboarddev->udev, 8, GFP_ATOMIC, &keyboarddev->dma);
    if(!keyboarddev->data)
    {
        pr_err("usb_alloc_coherent failed\n");
        return -ENOMEM;
    }

    keyboarddev->packet_size = usb_maxpacket(keyboarddev->udev, pipe);
    keyboarddev->dev = input_allocate_device();
    usb_make_path(keyboarddev->udev, keyboarddev->phys, sizeof(keyboarddev->phys));    
    strlcat(keyboarddev->phys, "/keyboard0", sizeof(keyboarddev->phys));
    
    keyboarddev->dev->name = keyboarddev->name;
	keyboarddev->dev->phys = keyboarddev->phys;

    // set relationship between usb device and input device
    usb_to_input_id(keyboarddev->udev, &keyboarddev->dev->id);
    keyboarddev->dev->dev.parent = &interface->dev;

    // TODO: update to work with keyboard instead of mouse
    // relative movement and key events
    keyboarddev->dev->evbit[0] = BIT_MASK(EV_KEY) ;

    __set_bit(KEY_A, keyboarddev->dev->keybit); 
    __set_bit(KEY_B, keyboarddev->dev->keybit); 
    __set_bit(KEY_C, keyboarddev->dev->keybit);
    __set_bit(KEY_ENTER, keyboarddev->dev->keybit);    
    __set_bit(KEY_BACKSPACE, keyboarddev->dev->keybit); 
    __set_bit(KEY_SPACE, keyboarddev->dev->keybit);
    __set_bit(KEY_LEFTCTRL, keyboarddev->dev->keybit);
    __set_bit(KEY_LEFTSHIFT, keyboarddev->dev->keybit);
    __set_bit(KEY_LEFTALT, keyboarddev->dev->keybit); 

    keyboarddev->dev->open = keyboard_open;
    keyboarddev->dev->close = keyboard_close;

    input_set_drvdata(keyboarddev->dev, keyboarddev);

    usb_fill_int_urb(keyboarddev->urb, keyboarddev->udev, pipe, keyboarddev->data, 8, 
                     complete_handler,  keyboarddev, endpoint->bInterval);
    
    keyboarddev->urb->transfer_dma = keyboarddev->dma;
	keyboarddev->urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

    input_register_device(keyboarddev->dev);

	return 0;

}

void complete_handler(struct urb * urb)
{
    struct keyboard* keyboarddev = urb->context;
	char *data = keyboarddev->data;
	struct input_dev *dev = keyboarddev->dev;
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

    if (data[0] == keys[0])
    {
        pr_info("Left Shift pressed");
    } 
    
    if (data[2] == keys[1])
    {
        pr_info("A key pressed");
    }

}

static int keyboard_open(struct input_dev* dev)
{
    struct keyboard *keyboarddev;
    keyboarddev = input_get_drvdata(dev);
    if(!keyboarddev)
	{
		printk(KERN_ALERT "no keyboard\n");
		return -1;
	}
	if(!keyboarddev->udev)
    {
		printk(KERN_ALERT "no keyboard->usbdev\n");
		return -1;
	}
	if(!keyboarddev->urb)
    {
		printk(KERN_ALERT "No keyboard->urb\n");
		return -1;
	}

    // set the usb_device for urb
    keyboarddev->urb->dev = keyboarddev->udev;

    int ret = usb_submit_urb(keyboarddev->urb, GFP_KERNEL);

    if (ret) 
    {
        printk(KERN_ALERT "Failed submitting urb: error code %d\n", ret);
    }

    printk("Keyboard open");

    return 0;
}

static void keyboard_close(struct input_dev *dev)
{
	struct keyboard *keyboarddev = input_get_drvdata(dev);

	usb_kill_urb(keyboarddev->urb);
}

static void skel_disconnect(struct usb_interface *interface)
{
	printk(KERN_ALERT "USB disconnecting\n");
	usb_deregister_dev(interface, NULL);
    usb_free_urb(keyboarddev->urb);
    usb_free_coherent(keyboarddev->udev, 8, keyboarddev->data, keyboarddev->dma);
    input_free_device(keyboarddev->dev);
    kfree(keyboarddev);
	unregister_chrdev(0, "mykeyboard");
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
