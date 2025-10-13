#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xba8fbd64, "_raw_spin_lock" },
	{ 0x122c3a7e, "_printk" },
	{ 0xed6af888, "__platform_driver_register" },
	{ 0xc503a308, "kobject_del" },
	{ 0x317df83, "kobject_put" },
	{ 0xe095e43a, "device_destroy" },
	{ 0x4a41ecb3, "class_destroy" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x16d0c724, "devm_hwmon_device_register_with_groups" },
	{ 0xb720d070, "devm_pinctrl_get" },
	{ 0xc3663688, "pinctrl_lookup_state" },
	{ 0x26248d71, "pinctrl_select_state" },
	{ 0x832481e, "devm_gpiod_get" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x5d9d9fd4, "cdev_init" },
	{ 0xcc335c1c, "cdev_add" },
	{ 0xf311fc60, "class_create" },
	{ 0x93ab9e33, "device_create" },
	{ 0xad811a0a, "kobject_create_and_add" },
	{ 0x47229b5c, "gpio_request" },
	{ 0x11437cf9, "devm_request_threaded_irq" },
	{ 0xfa61d21, "devm_kmalloc" },
	{ 0xf810f451, "_dev_err" },
	{ 0xa223bac6, "platform_driver_unregister" },
	{ 0x13929d91, "gpio_to_desc" },
	{ 0x1db5c2ad, "gpiod_get_raw_value" },
	{ 0xb43f9365, "ktime_get" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x1000e51, "schedule" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0x92540fbf, "finish_wait" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x3ce4ca6f, "disable_irq" },
	{ 0x3e5d99eb, "gpiod_direction_output_raw" },
	{ 0xf9a482f9, "msleep" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0x54e5a03b, "gpiod_direction_input" },
	{ 0xfcec0987, "enable_irq" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x656e4a6e, "snprintf" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0xb5b54b34, "_raw_spin_unlock" },
	{ 0x39ff040a, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cdht11,sensor");
MODULE_ALIAS("of:N*T*Cdht11,sensorC*");

MODULE_INFO(srcversion, "66A80D5263D8ACD7C4CCAC9");
