#include <linux/slab.h>
#include <linux/ktime.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/gpio.h>
#include <linux/wait.h>
#include <linux/sched.h>  
#include <linux/module.h> 
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/pinctrl/consumer.h>

// TODO 
// HWMON 
// spinlock_t multiple process

static DECLARE_WAIT_QUEUE_HEAD(dht11_gpio_wait_queue);

static const struct of_device_id dht11_of_match[] = {
    { .compatible = "dht11,sensor" },
    {}
};

MODULE_DEVICE_TABLE(of, dht11_of_match);

static struct sensor_datas { 
    struct task_struct *dht11_thread;
    struct class *dht11_class;
    struct kobject *dht11_kobj;
    struct device *dht11_device;
    struct pinctrl* dht11_pinctrl; 
    struct pinctrl_state* pins_default;
    struct device_node *np;
    struct cdev cdev;
    struct gpio_desc* dht11_gpio;
    dev_t dev_num;
    uint16_t major;
    spinlock_t* op_lock;

    unsigned int gpio_p;
    unsigned int gpio_val;
    uint32_t data[5];
    volatile int gpio_toggle_occurred;
    int dht11_irq;
    int running;
    unsigned int init;
} sensor_datas;

static int dht11_check_err(struct sensor_datas* dev_data);
static int dht11_init(struct sensor_datas* pdev);
static int dht11_read(struct sensor_datas* pdev);
static int dht11_set_context_running(struct platform_device* pdev);
static ssize_t dht11f_read( struct file *file, char __user *buffer, 
                            size_t count, loff_t *ppos);
static irqreturn_t dht11_interrupt_gpio_handler(int irq, void* dev_id);
static int wait_till_toggle_interruptable(struct sensor_datas* pdev,
                                          const unsigned int max);
static int dht11_probe(struct platform_device* pdev);
static void dht11_remove(struct platform_device* pdev);
static long dht11_etx_ioctl(struct file *file, 
                            unsigned int cmd, 
                            unsigned long arg);
static int dht11f_open(struct inode* inode, struct file* filep);

#define HUMID_VALUE     _IOR('W', 62, int32_t)
#define TEMP_VALUE_C    _IOR('W', 63, int32_t)
#define TEMP_VALUE_F    _IOR('W', 64, int32_t)
#define CRC_VALUE       _IOR('W', 65, int32_t)

static struct platform_driver dht11_driver = {
    .probe = dht11_probe,
    .remove = dht11_remove,
    .driver = {
        .name = "dht11 driver",
        .of_match_table = of_match_ptr(dht11_of_match),
        .probe_type = PROBE_PREFER_ASYNCHRONOUS,
    },
};

struct file_operations fops =  {
    .read = dht11f_read,
    .write = NULL,
    .open = dht11f_open,
    .unlocked_ioctl = dht11_etx_ioctl,
    .release = NULL 
};

static int dht11f_open(struct inode* inode, 
                       struct file* filep) {
    // set private data for filep
    struct sensor_datas* dht11_data; 

    dht11_data = container_of(inode->i_cdev, struct sensor_datas, cdev);

    if(!dht11_data) {
        pr_err("Failed to get dht11_data in dht11f_open");
        return -ENOMEM; 
    }

    // set pri data 
    filep->private_data = dht11_data;

    // lock so other user process can not perform 
    // operation during the duration in which the 
    // sensor being read
    spin_lock(dht11_data->op_lock);

    return 0;
}

static long dht11_etx_ioctl(struct file *file, 
                            unsigned int cmd, 
                            unsigned long arg) {
    int ret;
    struct sensor_datas* dev_data;
    
    dev_data = file->private_data;
    if (!dev_data) {
        ret = -EFAULT;
        goto err;
    }

    dht11_init(dev_data);
    dht11_read(dev_data);

    switch(cmd) {
        case HUMID_VALUE: 
            ret = copy_to_user((int32_t*) arg, 
                                dev_data->data[1], 
                                sizeof(dev_data->data[0]));
            break;
        case TEMP_VALUE_C: 
            ret = copy_to_user((int32_t*) arg, 
                                dev_data->data[1], 
                                sizeof(dev_data->data[1]));
            break;
        case TEMP_VALUE_F: 
            ret = 0;
            // TODO
            break;
        case CRC_VALUE:
            ret = copy_to_user((int32_t*) arg, 
                                dev_data->data[1], 
                                sizeof(dev_data->data[2])); 
            break;
        default: 
            ret = -ENOTTY;
    }

    if (ret > 0) {
        ret = -EFAULT;
        pr_err("Failed to copy to user space");
    }

err:
    spin_unlock(dev_data->op_lock);
    return ret;

}

// call back function to handle toggle interrupt on gpio
// ACK and set gpio_toggle_occured
static irqreturn_t dht11_interrupt_gpio_handler (int irq, 
                                                 void* dev_id) {
    struct platform_device* pdev;
    struct sensor_datas* pdev_datas;

    pdev = (struct platform_device* ) dev_id;
    pdev_datas = dev_get_drvdata(&pdev->dev);
    if (!pdev_datas)
        return -ENODEV;
    
    int gpio_curr_val = gpio_get_value(pdev_datas->gpio_p);
    if (gpio_curr_val ==  pdev_datas->gpio_val) {
        // irq is not on this device, toggle not occured
        return IRQ_NONE;
    }
    pdev_datas->gpio_toggle_occurred = 1;

    return IRQ_HANDLED;
}

static ssize_t dht11f_read(struct file *file, 
                           char __user *buffer, 
                           size_t count, loff_t *ppos) {
    char buf[32];
    int len;
    int ret;
    struct sensor_datas* dev_data;

    dev_data = file->private_data;
    if (!dev_data) {
        ret = -EFAULT;
        return ret;
    }
    
    dht11_init(dev_data);
    dht11_read(dev_data);
    
    len = snprintf(buf, sizeof(buf), "%d %d\n", dev_data->data[0], dev_data->data[2]);

    if (*ppos >= len)
        return 0;

    if(copy_to_user(buffer, buf, sizeof(buf)))
        return -EFAULT; 

    *ppos += len;

    return len; 
}

static int dht11_probe(struct platform_device* pdev) {
    int ret;

    dev_set_drvdata(&pdev->dev, &sensor_datas);
    dht11_set_context_running(pdev);

    sensor_datas.dht11_pinctrl = devm_pinctrl_get(&pdev->dev);

    if(IS_ERR(sensor_datas.dht11_pinctrl)) {
        dev_err(&pdev->dev, "Failed to get pinctrl handler");
        ret = PTR_ERR(sensor_datas.dht11_pinctrl);
        goto err0;
    }

    sensor_datas.pins_default = pinctrl_lookup_state(sensor_datas.dht11_pinctrl, 
                                                    "default");
    if (IS_ERR(sensor_datas.pins_default)) {
        dev_err(&pdev->dev, "Failed to get default pin state");
        ret = PTR_ERR(sensor_datas.pins_default);
        goto err1;
    }

    ret = pinctrl_select_state(sensor_datas.dht11_pinctrl, sensor_datas.pins_default);

    if(ret) {
        dev_err(&pdev->dev, "Failed to set default pin state");
        goto err2;
    }

    sensor_datas.dht11_gpio = devm_gpiod_get(&pdev->dev, "data", GPIOD_OUT_LOW);
    if (IS_ERR(sensor_datas.dht11_gpio)) {
        dev_err(&pdev->dev, "Failed to get GPIO descriptor\n");
        ret = PTR_ERR(sensor_datas.dht11_gpio);
        goto err3;
    }

    ret = alloc_chrdev_region(&sensor_datas.dev_num, 0, 1, "dht11_driver");
    if (ret < 0) {
        pr_info("dht11 char device registered failed");
        goto err3;
    }
    else {
        pr_info("dht11 char device registered successfully");
    }

    cdev_init(&sensor_datas.cdev, &fops);
    sensor_datas.cdev.owner = THIS_MODULE;
    ret = cdev_add(&sensor_datas.cdev, sensor_datas.dev_num, 1 );

    if (ret < 0) {
        pr_info("Failed to add character device into the system");
        goto err4;
    }

    // Create device class 
    sensor_datas.dht11_class = class_create("dht11_driver");
    ret = IS_ERR(sensor_datas.dht11_class); 
    if (ret) {
        goto err5;
    }

    // Create device node in /dev/
    sensor_datas.dht11_device = device_create(sensor_datas.dht11_class, NULL, 
                                             sensor_datas.dev_num, 
                                             &sensor_datas, "dht11_driver");
    ret = IS_ERR(sensor_datas.dht11_device);
    if (ret) {
        pr_err("Failed to create device\n");
        goto err6;
    }

    pr_info("Device /dev/dht11_driver created successfully\n");

    sensor_datas.dht11_kobj = kobject_create_and_add("dht11", 
                                                   sensor_datas.dht11_kobj); 
    if (!sensor_datas.dht11_kobj) {
        pr_info("Failed to create kobject");
        ret = -ENOMEM;
        goto err7;
    }
    
    ret = gpio_request(sensor_datas.gpio_p, "dht11-pin"); 
    if (ret < 0) {
        pr_err("Failed to request GPIO pin %d dht11."
                "Error: %d\n", sensor_datas.gpio_p, ret);
        goto err8;
    }

    // request IRQ for falling and rising trigger for gpio 
    ret = devm_request_irq(&pdev->dev, sensor_datas.dht11_irq, 
                           dht11_interrupt_gpio_handler,
                           IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, 
                           pdev->name, pdev);
    
    spin_lock_init(sensor_datas.op_lock);

    return 0;

err8:
err7:
err6:
err5: 
err4:
err3:
err2:
    class_destroy(sensor_datas.dht11_class);
    unregister_chrdev_region(sensor_datas.dev_num, 1);
err1:
err0:
    return ret;
}

static void dht11_remove(struct platform_device* pdev) {
    struct sensor_datas* pdev_datas;
    pdev_datas = (struct sensor_datas* ) dev_get_drvdata(&pdev->dev);
    if (!pdev_datas)
        return ;

    pr_info("Stopping dht11 module dht11\n");
    pdev_datas->running = 0;
    kobject_del(pdev_datas->dht11_kobj);
    kobject_put(pdev_datas->dht11_kobj);
    pdev_datas->dht11_kobj = NULL;
    pr_info("dht11 kobject removed successfully\n");
    device_destroy(pdev_datas->dht11_class, pdev_datas->dev_num);
    class_destroy(pdev_datas->dht11_class);
    unregister_chrdev_region(pdev_datas->dev_num, 1);
    pr_info("dht11_driver char device removed");
}

static int wait_till_toggle_interruptable(struct sensor_datas* dev_data, 
                                          const unsigned int max) {
    if (!dev_data)
        return -ENODEV;

    unsigned int st = ktime_to_us(ktime_get()); 
    unsigned int curr = 0;

    wait_event_interruptible(dht11_gpio_wait_queue, 
                             dev_data->gpio_toggle_occurred);

    // toggle event ocurred, unset gpio_toggle_occurred 
    dev_data->gpio_toggle_occurred = 0;
    curr = ktime_to_us(ktime_get());
    if(curr - st > max) {
        return -1;
    } 

    return curr - st;
}

static int dht11_set_context_running(struct platform_device* pdev) {
    struct sensor_datas* dev_data;
    dev_data = dev_get_drvdata(&pdev->dev);
    if (!dev_data)
        return -ENODEV;
    dev_data->running = 1;
    return 0;
}

static int dht11_check_err(struct sensor_datas* dev_data) {
    int ret;
    if (!dev_data)
        return -ENODEV;
    ret = (dev_data->data[0] + dev_data->data[1] + 
            dev_data->data[2] + dev_data->data[3]) == dev_data->data[4];
    
    return ret;
}

static int dht11_init(struct sensor_datas* dev_data) {   
    if (!dev_data)
        return -ENODEV;

    // MCU sends out start signal by pulling down 
    // voltage of gpio pin for at least 18 ms
    // Here push it to 20 ms
    gpio_direction_output(dev_data->gpio_p, 0);
    if(gpio_get_value(dev_data->gpio_p) == 1)
    {
    	pr_err("Value not set 0\n");
	    return -1;
    }

    msleep(30);

    // MCU pull up the voltage again for 20 - 40 
    // us and wait for DHT11's response
    gpio_direction_output(dev_data->gpio_p, 1);
    usleep_range(40, 50);
    gpio_direction_input(dev_data->gpio_p);

    enable_irq(dev_data->dht11_irq);

    if(wait_till_toggle_interruptable(dev_data, 50) == -1) 
    {
        pr_err("dht11 Init failure 1\n");
        return -1;
    }

    // wait for 80 us until DHT11 pull up voltage
    if(wait_till_toggle_interruptable(dev_data, 90) == -1) 
    {
        pr_err("dht11 Init failure 2\n");
        return -1;
    }   

    // wait for 80 us until DHT11 pull down voltage 
    if(wait_till_toggle_interruptable(dev_data, 90) == -1) 
    {
        pr_err("dht11 Init failure 3\n");
        return -1;
    }

    // the data transmission can start. Init completed
    dev_data->init = 1;
    return 0;
}

static int dht11_read(struct sensor_datas* dev_data) {
    if (!dev_data)
        return -ENODEV;

    // The data transmission format consists of five 8-bit bytes, 
    // arranged as follows:

    // 8-bit integer part of relative humidity (RH)
    // 8-bit decimal part of relative humidity (RH)
    // 8-bit integer part of temperature (T)
    // 8-bit decimal part of temperature (T)
    // 8-bit checksum

    // To verify the integrity of the transmitted data, 
    // the checksum (the fifth byte) should equal the 
    // last 8 bits of the sum of the first four bytes 
    // (integer RH, decimal RH, integer T, and decimal T).  
    // If the calculated checksum matches the received checksum, 
    // the data is considered valid.

    for(int i=0;i<5;i++) {
	    for(int j=0;j<8;j++) {
            uint16_t time_wait = wait_till_toggle_interruptable(dev_data, 60);
            uint16_t time_bit = wait_till_toggle_interruptable(dev_data, 80);
            if(time_bit < time_wait)
            {
                (dev_data->data[i] <<=1);
            }
            else
            {
                dev_data->data[i] = (dev_data->data[i] <<1) |1;
            }
	    }
	}

    // Done with reading, disable the interrupt for now since we don't need to 
    // use ISR for reading data
    disable_irq(dev_data->dht11_irq);

    // convert to int and dec 
    pr_info("Data dht11 : humid: %d.%d, temp: %d.%d, sum: %d\n", 
            dev_data->data[0], dev_data->data[1], dev_data->data[2], 
            dev_data->data[3], dev_data->data[4]);

    if(dht11_check_err(dev_data) < 0) {
        pr_err("Error in data dht11\n");
        return 0;
    }

    return 1;
}

int32_t dht11_get_tempC(struct platform_device* pdev) {
    struct sensor_datas* dev_data;

    dev_data = (struct sensor_datas*) dev_get_drvdata(&pdev->dev);
    if (!dev_data)
        return -ENODEV;
    
    return dev_data->data[1];
}

int32_t dht11_get_tempF(struct platform_device* pdev) {
    return 0;

    // TODO
}

int32_t dht11_get_CRC(struct platform_device* pdev) {
    struct sensor_datas* dev_data;

    dev_data = (struct sensor_datas*) dev_get_drvdata(&pdev->dev);
    if (!dev_data)
        return -ENODEV;
    
    return dev_data->data[2];
}

int32_t dht11_get_humid(struct platform_device* pdev) {
    struct sensor_datas* dev_data;

    dev_data = (struct sensor_datas*) dev_get_drvdata(&pdev->dev);
    if (!dev_data)
        return -ENODEV;
    
    return dev_data->data[0];
}

module_platform_driver(dht11_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonate <sonate339@gmail.com>");
MODULE_DESCRIPTION("Simple DHT11 communication driver");
MODULE_VERSION("1.0");
