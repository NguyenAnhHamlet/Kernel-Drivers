#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
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

#ifdef CONFIG_MITIGATION_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const char ____versions[]
__used __section("__versions") =
	"\x10\x00\x00\x00\xd8\x7e\x99\x92"
	"_printk\0"
	"\x14\x00\x00\x00\x26\x6a\xdc\xcf"
	"kobject_del\0"
	"\x14\x00\x00\x00\x39\x2e\x66\xc8"
	"kobject_put\0"
	"\x18\x00\x00\x00\x8c\xb2\x2e\x92"
	"device_destroy\0\0"
	"\x18\x00\x00\x00\x02\x9b\x8a\x31"
	"class_destroy\0\0\0"
	"\x1c\x00\x00\x00\xc0\xfb\xc3\x6b"
	"__unregister_chrdev\0"
	"\x18\x00\x00\x00\x70\xc7\x6b\xf8"
	"kthread_stop\0\0\0\0"
	"\x20\x00\x00\x00\x16\x2f\xc5\xf9"
	"of_find_compatible_node\0"
	"\x2c\x00\x00\x00\x2e\x32\xb6\x6d"
	"of_property_read_variable_u32_array\0"
	"\x1c\x00\x00\x00\xbd\xa4\xe2\xfb"
	"__register_chrdev\0\0\0"
	"\x18\x00\x00\x00\x4b\x09\xa6\x47"
	"class_create\0\0\0\0"
	"\x18\x00\x00\x00\x26\x4c\x0e\x6c"
	"device_create\0\0\0"
	"\x14\x00\x00\x00\x05\x86\x72\x7b"
	"kernel_kobj\0"
	"\x20\x00\x00\x00\xec\x66\x68\xdc"
	"kobject_create_and_add\0\0"
	"\x18\x00\x00\x00\x5c\x9b\x22\x47"
	"gpio_request\0\0\0\0"
	"\x20\x00\x00\x00\x6a\x03\x35\xc7"
	"kthread_create_on_node\0\0"
	"\x18\x00\x00\x00\x3d\x8f\x11\xec"
	"wake_up_process\0"
	"\x1c\x00\x00\x00\xcb\xf6\xfd\xf0"
	"__stack_chk_fail\0\0\0\0"
	"\x14\x00\x00\x00\x65\x93\x3f\xb4"
	"ktime_get\0\0\0"
	"\x18\x00\x00\x00\x40\x2b\xea\x8a"
	"gpio_to_desc\0\0\0\0"
	"\x1c\x00\x00\x00\x34\x9f\x23\x74"
	"gpiod_get_raw_value\0"
	"\x14\x00\x00\x00\x6e\x4a\x6e\x65"
	"snprintf\0\0\0\0"
	"\x1c\x00\x00\x00\x54\xfc\xbb\x6c"
	"__arch_copy_to_user\0"
	"\x1c\x00\x00\x00\x9a\xe6\x97\xd6"
	"trace_hardirqs_on\0\0\0"
	"\x1c\x00\x00\x00\xef\x6d\x5c\xa6"
	"alt_cb_patch_nops\0\0\0"
	"\x1c\x00\x00\x00\x1b\x2e\x3d\xec"
	"trace_hardirqs_off\0\0"
	"\x18\x00\x00\x00\x62\x5e\x13\x23"
	"gpiod_to_irq\0\0\0\0"
	"\x14\x00\x00\x00\x52\x00\x99\xfe"
	"gpio_free\0\0\0"
	"\x28\x00\x00\x00\xb3\x1c\xa2\x87"
	"__ubsan_handle_out_of_bounds\0\0\0\0"
	"\x10\x00\x00\x00\xf9\x82\xa4\xf9"
	"msleep\0\0"
	"\x1c\x00\x00\x00\x6e\x64\xf7\xb3"
	"kthread_should_stop\0"
	"\x24\x00\x00\x00\xa4\xee\x8c\x19"
	"gpiod_direction_output_raw\0\0"
	"\x1c\x00\x00\x00\x20\x5d\x05\xc3"
	"usleep_range_state\0\0"
	"\x20\x00\x00\x00\x2a\x85\x9c\x68"
	"gpiod_direction_input\0\0\0"
	"\x18\x00\x00\x00\x91\x64\x59\xe6"
	"module_layout\0\0\0"
	"\x00\x00\x00\x00\x00\x00\x00\x00";

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "646140E3127EA8FAA101CF7");
