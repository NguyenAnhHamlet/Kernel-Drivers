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

static DECLARE_WAIT_QUEUE_HEAD(dht11_gpio_wait_queue);
static struct task_struct *dht11_thread = NULL;
static struct kobject *dht11_kobj;
static struct class *dht11_class;
static struct device *dht11_device;
uint16_t major;

typedef struct {
    unsigned int gpio_p;
    uint8_t data[5];
    volatile int gpio_rising_occurred;
    int irq_num;
    int running;
    unsigned int init;
    
} dht11_ctx ;

static dht11_ctx ctx;
static int dht11_check_err(dht11_ctx* ctx);
static int dht11_init(dht11_ctx* ctx);
static int dht11_read(dht11_ctx* ctx);
static int dht11_reading_thread(void* data);
static void dht11_set_context_running(dht11_ctx* ctx);
static ssize_t device_read(struct file *file, char __user *buffer, 
                        size_t count, loff_t *ppos);

struct file_operations fops = 
{
    .read = device_read,
    .write = NULL,
    .open = NULL,
    .release = NULL 
};

static ssize_t device_read(struct file *file, char __user *buffer, 
                        size_t count, loff_t *ppos)
{
    char buf[32];
    int len;
    
    len = snprintf(buf, sizeof(buf), "%d %d\n", ctx.data[0], ctx.data[2]);

    if (*ppos >= len)
        return 0;

    if(copy_to_user(buffer, buf, sizeof(buf)))
        return -EFAULT; 

    *ppos += len;

    return len; 
}


static int wait_till_low(dht11_ctx* ctx, const unsigned int max)
{
    unsigned int st = ktime_to_us(ktime_get()); 
    unsigned int curr = 0;
start:
    curr = ktime_to_us(ktime_get());
    if(curr - st > max) return -1;
    if(gpio_get_value(ctx->gpio_p) == 0) return curr - st;
    goto start;
}

static int wait_till_high(dht11_ctx* ctx, const unsigned int max)
{
    unsigned int st = ktime_to_us(ktime_get());
    unsigned int curr = 0;
start:
    curr = ktime_to_us(ktime_get());
    if(curr - st > max) return -1;
    if(gpio_get_value(ctx->gpio_p) == 1) return curr - st;
    goto start;
}

static void dht11_set_context_running(dht11_ctx* ctx)
{
    ctx->running = 1;
}

static int read_gpio_p_dht11(dht11_ctx* ctx)
{
    struct device_node *np;
    u32 gpios[3];

    np = of_find_compatible_node(NULL, NULL, "dht11,sensor");

    if (!np) 
    {
        pr_err("dht11: Device tree node not found\n");
        return -ENODEV;
    }

    if(of_property_read_u32_array(np, "gpios", gpios, 2))
    {
        pr_err("DHT11: Failed to read GPIO number from device tree %d\n", ctx->gpio_p);
        return -EINVAL;
    }

    ctx->gpio_p = gpios[1];

    if (!gpio_is_valid(ctx->gpio_p))
    {
        pr_err("DHT11: dht11 gpio not found\n");
        return -EINVAL;
    }

    return 0;
}

static int dht11_check_err(dht11_ctx* ctx)
{
    return (ctx->data[0] + ctx->data[1] + ctx->data[2] + ctx->data[3]) == ctx->data[4]; 
}

static int dht11_init(dht11_ctx* ctx)
{   
    // MCU sends out start signal by pulling down 
    // voltage of gpio pin for at least 18 ms
    // Here push it to 20 ms
    gpio_direction_output(ctx->gpio_p, 0);
    if(gpio_get_value(ctx->gpio_p) == 1)
    {
    	pr_err("Value not set 0\n");
	    return -1;
    }

    msleep(30);

    // MCU pull up the voltage again for 20 - 40 
    // us and wait for DHT11's response
    gpio_direction_output(ctx->gpio_p, 1);
    usleep_range(40, 50);
    gpio_direction_input(ctx->gpio_p);

    if(wait_till_low(ctx, 50) == -1) 
    {
        pr_err("dht11 Init failure 1\n");
        return -1;
    }

    // wait for 80 us until DHT11 pull up voltage
    if(wait_till_high(ctx, 90) == -1) 
    {
        pr_err("dht11 Init failure 2\n");
        return -1;
    }   

    // wait for 80 us until DHT11 pull down voltage 
    if(wait_till_low(ctx, 90) == -1) 
    {
        pr_err("dht11 Init failure 3\n");
        return -1;
    }

    // the data transmission can start. Init completed
    ctx->init = 1;
    return 0;
}

static int dht11_read(dht11_ctx* ctx)
{
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

    for(int i=0;i<5;i++)
    {
	    for(int j=0;j<8;j++)
        {
            uint16_t time_wait = wait_till_high(ctx, 60);
            uint16_t time_bit = wait_till_low(ctx, 80);
            if(time_bit < time_wait)
            {
                (ctx->data[i] <<=1);
            }
            else
            {
                ctx->data[i] = (ctx->data[i] <<1) |1;
            }
	    }
	}

    // convert to int and dec 
    pr_info("Data dht11 : humid: %d.%d, temp: %d.%d, sum: %d\n", 
            ctx->data[0], ctx->data[1], ctx->data[2], ctx->data[3], ctx->data[4]);

    if(dht11_check_err(ctx) < 0)
    {
        pr_err("Error in data dht11\n");
        return 0;
    }

    return 1;
}

static int dht11_reading_thread(void* data)
{
    dht11_ctx* ctx = (dht11_ctx* ) data;

    pr_info("Reading sensor dht11 data... dht11\n");

    // sending interrupt signal whenever there is rising 
    // on pin 
    ctx->irq_num = gpio_to_irq(ctx->gpio_p);
    if (ctx->irq_num < 0) 
    {
        pr_err("Failed to get IRQ number for GPIO %d dht11\n", ctx->gpio_p);
        gpio_free(ctx->gpio_p);
        return 0;
    }

    while(ctx->running && !kthread_should_stop())
    {
        if(dht11_init(ctx) < 0)
        {
            pr_err("Failure init finale dht11\n");
	        continue;
        }

        if(dht11_read(ctx) < 0)
        {
            pr_err("Failure reading data dht11\n");
            continue;
        }

        msleep(1000);

        if (kthread_should_stop()) 
            break;
    }

    
    //free_irq(ctx->irq_num, ctx);
    gpio_free(ctx->gpio_p);
    dht11_thread = NULL;

    pr_info("Exiting DHT11 reading thread...\n");

    return 0;
}

static int __init dht11_driver_init(void)
{
    dht11_set_context_running(&ctx);

    if(read_gpio_p_dht11(&ctx) < 0)
    {
        pr_err("Failed to read gpio \n");
        return -1;
    }

    major = register_chrdev(0, "dht11_driver", &fops);
    if(major < 0)
    {
        pr_info("dht11 char device registered failed");
    }
    else 
    {
        pr_info("dht11 char device registered successfully");
    }

    // Create device class 
    dht11_class = class_create("dht11_driver");
    if (IS_ERR(dht11_class))
    {
        unregister_chrdev(major, "dht11_driver");
        return PTR_ERR(dht11_class);
    }

    // Create device node in /dev/
    dht11_device = device_create(dht11_class, NULL, MKDEV(major, 0), NULL, "dht11_driver");
    if (IS_ERR(dht11_device))
    {
        unregister_chrdev(major, "dht11_driver");
        class_destroy(dht11_class);
        pr_err("Failed to create device\n");
        return PTR_ERR(dht11_device);
    }

    pr_info("Device /dev/dht11_driver created successfully\n");

    dht11_kobj = kobject_create_and_add("dht11", kernel_kobj); 
    if (!dht11_kobj)
        return -ENOMEM;
    
    int ret = gpio_request(ctx.gpio_p, "dht11-pin"); 
    if (ret < 0) 
    {
        pr_err("Failed to request GPIO pin %d dht11. Error: %d\n", ctx.gpio_p, ret);
        return -1;
    }

    dht11_thread = kthread_run(dht11_reading_thread, (void*) &ctx, "dht11_thread");

    if (IS_ERR(dht11_thread)) 
    {
        pr_err("Failed to create thread dht11\n");
        return PTR_ERR(dht11_thread);
    }

    return 0;
}

static void __exit dht11_driver_exit(void)
{
    pr_info("Stopping dht11 module dht11\n");
    ctx.running = 0;

    if (dht11_kobj) 
    {
        kobject_del(dht11_kobj);
        kobject_put(dht11_kobj);
        dht11_kobj = NULL;
        pr_info("dht11 kobject removed successfully\n");
    }

    device_destroy(dht11_class, MKDEV(major, 0));
    class_destroy(dht11_class);
    unregister_chrdev(major, "dht11_driver");
    
    pr_info("dht11_driver char device removed");

    if (dht11_thread)
        kthread_stop(dht11_thread);
}

module_init(dht11_driver_init);
module_exit(dht11_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonate <sonate339@gmail.com>");
MODULE_DESCRIPTION("Simple DHT11 communication driver");
MODULE_VERSION("1.0");
