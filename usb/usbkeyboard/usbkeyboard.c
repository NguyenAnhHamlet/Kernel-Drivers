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

static const int key_map[256] = 
{
    [0 ... 255] = 0, // Initialize all values to KEY_RESERVED

    /* Letters */
    [0x04] = KEY_A, [0x05] = KEY_B, [0x06] = KEY_C, [0x07] = KEY_D,
    [0x08] = KEY_E, [0x09] = KEY_F, [0x0A] = KEY_G, [0x0B] = KEY_H,
    [0x0C] = KEY_I, [0x0D] = KEY_J, [0x0E] = KEY_K, [0x0F] = KEY_L,
    [0x10] = KEY_M, [0x11] = KEY_N, [0x12] = KEY_O, [0x13] = KEY_P,
    [0x14] = KEY_Q, [0x15] = KEY_R, [0x16] = KEY_S, [0x17] = KEY_T,
    [0x18] = KEY_U, [0x19] = KEY_V, [0x1A] = KEY_W, [0x1B] = KEY_X,
    [0x1C] = KEY_Y, [0x1D] = KEY_Z,

    /* Numbers */
    [0x1E] = KEY_1, [0x1F] = KEY_2, [0x20] = KEY_3, [0x21] = KEY_4,
    [0x22] = KEY_5, [0x23] = KEY_6, [0x24] = KEY_7, [0x25] = KEY_8,
    [0x26] = KEY_9, [0x27] = KEY_0,

    /* Special Characters */
    [0x28] = KEY_ENTER, [0x29] = KEY_ESC, [0x2A] = KEY_BACKSPACE,
    [0x2B] = KEY_TAB, [0x2C] = KEY_SPACE, [0x2D] = KEY_MINUS,
    [0x2E] = KEY_EQUAL, [0x2F] = KEY_LEFTBRACE, [0x30] = KEY_RIGHTBRACE,
    [0x31] = KEY_BACKSLASH, [0x32] = KEY_BACKSLASH, // Non-US
    [0x33] = KEY_SEMICOLON, [0x34] = KEY_APOSTROPHE, [0x35] = KEY_GRAVE,
    [0x36] = KEY_COMMA, [0x37] = KEY_DOT, [0x38] = KEY_SLASH,

    /* Function Keys */
    [0x39] = KEY_CAPSLOCK, [0x3A] = KEY_F1, [0x3B] = KEY_F2,
    [0x3C] = KEY_F3, [0x3D] = KEY_F4, [0x3E] = KEY_F5,
    [0x3F] = KEY_F6, [0x40] = KEY_F7, [0x41] = KEY_F8,
    [0x42] = KEY_F9, [0x43] = KEY_F10, [0x44] = KEY_F11,
    [0x45] = KEY_F12,

    /* Control Keys */
    [0x46] = KEY_SYSRQ, [0x47] = KEY_SCROLLLOCK, [0x48] = KEY_PAUSE,
    [0x49] = KEY_INSERT, [0x4A] = KEY_HOME, [0x4B] = KEY_PAGEUP,
    [0x4C] = KEY_DELETE, [0x4D] = KEY_END, [0x4E] = KEY_PAGEDOWN,
    [0x4F] = KEY_RIGHT, [0x50] = KEY_LEFT, [0x51] = KEY_DOWN, [0x52] = KEY_UP,

    /* Keypad */
    [0x53] = KEY_NUMLOCK, [0x54] = KEY_KPSLASH, [0x55] = KEY_KPASTERISK,
    [0x56] = KEY_KPMINUS, [0x57] = KEY_KPPLUS, [0x58] = KEY_KPENTER,
    [0x59] = KEY_KP1, [0x5A] = KEY_KP2, [0x5B] = KEY_KP3, [0x5C] = KEY_KP4,
    [0x5D] = KEY_KP5, [0x5E] = KEY_KP6, [0x5F] = KEY_KP7, [0x60] = KEY_KP8,
    [0x61] = KEY_KP9, [0x62] = KEY_KP0, [0x63] = KEY_KPDOT,

    /* Extra Function Keys */
    [0x64] = KEY_102ND, [0x65] = KEY_COMPOSE, [0x66] = KEY_POWER,
    [0x67] = KEY_KPEQUAL, [0x68] = KEY_F13, [0x69] = KEY_F14,
    [0x6A] = KEY_F15, [0x6B] = KEY_F16, [0x6C] = KEY_F17,
    [0x6D] = KEY_F18, [0x6E] = KEY_F19, [0x6F] = KEY_F20,
    [0x70] = KEY_F21, [0x71] = KEY_F22, [0x72] = KEY_F23,
    [0x73] = KEY_F24, [0x74] = KEY_OPEN, [0x75] = KEY_HELP,
    [0x76] = KEY_PROPS, [0x77] = KEY_FRONT, [0x78] = KEY_STOP,
    [0x79] = KEY_AGAIN, [0x7A] = KEY_UNDO, [0x7B] = KEY_CUT,
    [0x7C] = KEY_COPY, [0x7D] = KEY_PASTE, [0x7E] = KEY_FIND,
    [0x7F] = KEY_MUTE, [0x80] = KEY_VOLUMEUP, [0x81] = KEY_VOLUMEDOWN,

    /* Modifier Keys */
    [0xE0] = KEY_LEFTCTRL, [0xE1] = KEY_LEFTSHIFT,
    [0xE2] = KEY_LEFTALT, [0xE3] = KEY_LEFTMETA,
    [0xE4] = KEY_RIGHTCTRL, [0xE5] = KEY_RIGHTSHIFT,
    [0xE6] = KEY_RIGHTALT, [0xE7] = KEY_RIGHTMETA
};


int usb_hid_to_linux_keycode(int hid_code);
void complete_handler(struct urb * urb);
static int skel_probe(struct usb_interface *interface, const struct usb_device_id *id);
static int keyboard_open(struct input_dev* dev);
static void keyboard_close(struct input_dev *dev);
static void skel_disconnect(struct usb_interface *interface);
static struct keyboard *keyboarddev;

static struct usb_device_id skel_table[] =
{
    // TODO: add infos
    {USB_DEVICE(0x258a,0x002a)},  
    {}
};

MODULE_DEVICE_TABLE(usb, skel_table );

static struct usb_driver skel_driver = 
{
    .name = "usb_keyboard_driver",
    .id_table = skel_table,
    .probe = skel_probe,
    .disconnect = skel_disconnect,
};

void set_keyboard_keybits(struct keyboard *keyboarddev) 
{
    for (int i = 0; i < 256; i++) {
        if (key_map[i] != KEY_RESERVED) {
            __set_bit(key_map[i], keyboarddev->dev->keybit);
        }
    }
}

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

    set_keyboard_keybits(keyboarddev);

    keyboarddev->dev->open = keyboard_open;
    keyboarddev->dev->close = keyboard_close;

    input_set_drvdata(keyboarddev->dev, keyboarddev);

    usb_fill_int_urb(keyboarddev->urb, keyboarddev->udev, pipe, keyboarddev->data, 8, 
                     complete_handler,  keyboarddev, endpoint->bInterval);
    
    keyboarddev->urb->transfer_dma = keyboarddev->dma;
	keyboarddev->urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

    input_register_device(keyboarddev->dev);
    usb_set_intfdata(interface, keyboarddev);

	return 0;

}

void complete_handler(struct urb * urb)
{
    struct keyboard* keyboarddev = urb->context;
	char *data = keyboarddev->data;
	struct input_dev *dev = keyboarddev->dev;
	int status;

	switch (urb->status) 
    {
	case 0:			
    {
        printk(KERN_INFO "USB Data: %02X %02X %02X %02X %02X %02X %02X %02X\n",
        data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

        int lkey = usb_hid_to_linux_keycode(data[2]);
        printk(KERN_INFO "HID Code: 0x%X -> Linux Keycode: %d\n", data[0], lkey);
        if(lkey != 0)
        {
            input_report_key(keyboarddev->dev, lkey, 1);
            input_report_key(keyboarddev->dev, lkey,  0);
            input_sync(keyboarddev->dev);
        }
		break;
    }
	case -ECONNRESET:
	case -ENOENT:
	case -ESHUTDOWN:
		return;
	default:		
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

    printk("press key");
}

int usb_hid_to_linux_keycode(int hid_code)
{
    return key_map[hid_code]; 
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
    struct keyboard* keyboarddev = usb_get_intfdata(interface);
    usb_free_urb(keyboarddev->urb);
    usb_free_coherent(keyboarddev->udev, 8, keyboarddev->data, keyboarddev->dma);
    input_unregister_device(keyboarddev->dev); 
    kfree(keyboarddev);
    usb_set_intfdata(interface, NULL);
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
