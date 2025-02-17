#include <linux/module.h>
#include <linux/init.h>
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

#define GPIO_21 (21)

static DECLARE_WAIT_QUEUE_HEAD(dht11_gpio_wait_queue);
static struct task_struct *dht11_thread;

typedef struct {
    unsigned int gpio_p;
    
    unsigned int temp_int;
    unsigned int temp_dec;
    unsigned int humid_int;
    unsigned int humid_dec;
    unsigned int sum;

    ktime_t pre;
    ktime_t curr; 
    volatile int gpio_rising_occurred;
    int irq_num;
    int running;

} dht11_ctx ;

static dht11_ctx ctx;
static int dht11_read_bit(dht11_ctx* ctx);
static int convert_bin(int* data);
static int dht11_check_err(dht11_ctx* ctx);
static int dht11_init(dht11_ctx* ctx);
static int dht11_read(dht11_ctx* ctx);
static int dht11_reading_thread(void* data);
static irqreturn_t dht11_get_toggle_time(int irq, void *dev_id);
static void dht11_set_context_running(dht11_ctx* ctx);

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

    pr_err("PIN : %d", ctx->gpio_p);

    if (!gpio_is_valid(ctx->gpio_p))
    {
        pr_err("DHT11: dht11 gpio not found\n");
        return -EINVAL;
    }

    pr_err("Pin number is: %d\n", ctx->gpio_p);

    return 0;
}

static int convert_bin(int* data)
{
    int ret;
    for (int i = 0; i < 8; i++) 
    {
        if (data[i] != 0 && data[i] != 1) {
            pr_err("Invalid binary value at index %d: %d\n", i, data[i]);
            return -EINVAL; 
        }
        ret |= (data[i] << (7 - i));
    }

    return ret;
}

static int dht11_check_err(dht11_ctx* ctx)
{
    int sum = 0;

    sum = sum | ctx->temp_int | ctx->temp_dec | ctx->humid_int | ctx->humid_dec;

    if(sum != ctx->sum)
        return -1;
    
    return 0;
}

static int dht11_init(dht11_ctx* ctx)
{   
    // MCU sends out start signal by pulling down 
    // voltage of gpio pin for at least 18 ms
    // Here push it to 20 ms
    gpio_direction_output(ctx->gpio_p, 0);
    msleep(20);

    // MCU pull up the voltage again for 20 - 40 
    // us and wait for DHT11's response
    gpio_direction_output(ctx->gpio_p, 1);
    usleep_range(40, 45);

    // Check for DHT11's response
    gpio_direction_input(ctx->gpio_p);
    if(gpio_get_value(ctx->gpio_p))
    {
        pr_err("dht11 Init failure 1\n");
        return -1;
    }

    // wait for 80 us until DHT11 pull up voltage
    usleep_range(80, 85);

    if(gpio_get_value(ctx->gpio_p) < 0)
    {
        pr_err("dht11 Init failure 2\n");
        return 0;
    }   

    // wait for 80 us until DHT11 pull down voltage 
    usleep_range(80, 85);

    if(gpio_get_value(ctx->gpio_p))
    {
        pr_err("dht11 Init failure 3\n");
        return 0;
    }

    // the data transmission can start. Init completed
    return 1;
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
    int data[40];

    for(int i=0; i< 40; i++)
    {
        data[i] = dht11_read_bit(ctx);
        if(data[i] == -1)
        {
            pr_err("Falied to read bit number %d\n dht11", i);
            return 0;
        }
    }

    // convert to int and dec 
    ctx->temp_int = convert_bin(data);
    ctx->temp_dec = convert_bin(data + 8);
    ctx->humid_int = convert_bin(data + 16);
    ctx->humid_dec = convert_bin(data + 24);
    ctx->sum = convert_bin(data + 32);

    pr_info("Data dht11 : temp: %d.%d, humid: %d.%d, sum: %d\n", 
            ctx->temp_int, ctx->temp_dec, ctx->humid_int, ctx->humid_dec, ctx->sum);

    if(dht11_check_err(ctx) < 0)
    {
        pr_err("Error in data dht11\n");
        return 0;
    }

    return 1;
}

static int dht11_read_bit(dht11_ctx* ctx)
{
    usleep_range(50, 55);
    gpio_direction_input(ctx->gpio_p);
    if(!gpio_get_value(ctx->gpio_p))
    {
        pr_err("Fail to get value gpio dht11\n");
        return -1;
    }

    ctx->pre = ktime_get();

    DEFINE_WAIT(wait);
    
    prepare_to_wait(&dht11_gpio_wait_queue, &wait, TASK_INTERRUPTIBLE);
    schedule_timeout(msecs_to_jiffies(1)); 
    if (ctx->gpio_rising_occurred || signal_pending(current)) 
    {
        int interval = ctx->curr - ctx->pre;
        if(interval > 50 && interval < 80) 
            return 1;
        if(interval > 80)
            return -1;
        return 0;
    }
    else 
    {
        pr_err(KERN_INFO "Timeout occurred, checking again. dht11\n");
        return -1;
    }


    return -1;
}

// call back function executed when there is change state in the 
// pin to update the time between the last change with current 
// change
static irqreturn_t dht11_get_toggle_time(int irq, void *dev_id)
{
    dht11_ctx* ctx = (dht11_ctx *) dev_id;
    ctx->curr = ktime_get();
    ctx->gpio_rising_occurred = 1;
    wake_up_interruptible(&dht11_gpio_wait_queue);

    return IRQ_HANDLED; 
}

static int dht11_reading_thread(void* data)
{
    dht11_ctx* ctx = (dht11_ctx* ) data;

    pr_info("Reading sensor dht11 data... dht11\n");
    while(ctx->running && !kthread_should_stop())
    {
        if(dht11_init(ctx) < 0)
        {
            pr_err("Failure init finale dht11\n");
            break;
        }

        if (ctx->irq_num > 0) 
        {
            free_irq(ctx->irq_num, ctx);
        }

        ctx->irq_num = gpio_to_irq(ctx->gpio_p);
        if (ctx->irq_num < 0) 
        {
            pr_err("Failed to get IRQ number for GPIO %d dht11\n", ctx->gpio_p);
            gpio_free(ctx->gpio_p);
            break;
        }

        int ret = request_irq(  ctx->irq_num, dht11_get_toggle_time,
                                IRQF_TRIGGER_RISING ,
                                "dht11_gpio_interrupt", (void*) ctx);
        if(ret < 0)
        {
            pr_err("Failure creating interrupt dht11. Error: %d\n", ret);
            break; 
        }

        if(dht11_read(ctx) < 0)
        {
            pr_err("Failure reading data dht11\n");
            break;
        }

        msleep(1000);

        if (kthread_should_stop()) 
            break;
    }

    
    free_irq(ctx->irq_num, ctx);
    gpio_free(ctx->gpio_p);

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

    pr_err("PIN : %d", ctx.gpio_p);
    int ret = gpio_request(533, "dht11-pin"); 
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
    if (dht11_thread)
        kthread_stop(dht11_thread);
}

module_init(dht11_driver_init);
module_exit(dht11_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonate <sonate339@gmail.com>");
MODULE_DESCRIPTION("Simple DHT11 communication driver");
MODULE_VERSION("1.0");