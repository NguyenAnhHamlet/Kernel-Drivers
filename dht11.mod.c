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
	"\x18\x00\x00\x00\x4c\xfe\xcb\x01"
	"gpio_to_desc\0\0\0\0"
	"\x1c\x00\x00\x00\xb5\xbc\x7b\x64"
	"gpiod_get_raw_value\0"
	"\x18\x00\x00\x00\x90\x86\x0b\x05"
	"kthread_stop\0\0\0\0"
	"\x14\x00\x00\x00\x65\x93\x3f\xb4"
	"ktime_get\0\0\0"
	"\x14\x00\x00\x00\x44\x43\x96\xe2"
	"__wake_up\0\0\0"
	"\x20\x00\x00\x00\xc2\x9a\x68\x36"
	"of_find_compatible_node\0"
	"\x2c\x00\x00\x00\x7d\x8d\x79\xb2"
	"of_property_read_variable_u32_array\0"
	"\x18\x00\x00\x00\x5c\x9b\x22\x47"
	"gpio_request\0\0\0\0"
	"\x20\x00\x00\x00\x2d\x78\x99\x04"
	"kthread_create_on_node\0\0"
	"\x18\x00\x00\x00\xfb\xe5\x0e\x28"
	"wake_up_process\0"
	"\x1c\x00\x00\x00\xcb\xf6\xfd\xf0"
	"__stack_chk_fail\0\0\0\0"
	"\x24\x00\x00\x00\x1f\x04\x73\xad"
	"autoremove_wake_function\0\0\0\0"
	"\x28\x00\x00\x00\xb3\x1c\xa2\x87"
	"__ubsan_handle_out_of_bounds\0\0\0\0"
	"\x1c\x00\x00\x00\x20\x5d\x05\xc3"
	"usleep_range_state\0\0"
	"\x20\x00\x00\x00\x3c\xb2\x06\xb1"
	"gpiod_direction_input\0\0\0"
	"\x18\x00\x00\x00\xf1\x90\xfd\xd5"
	"prepare_to_wait\0"
	"\x1c\x00\x00\x00\xad\x8a\xdd\x8d"
	"schedule_timeout\0\0\0\0"
	"\x18\x00\x00\x00\x65\x0c\xbe\xda"
	"gpiod_to_irq\0\0\0\0"
	"\x14\x00\x00\x00\x52\x00\x99\xfe"
	"gpio_free\0\0\0"
	"\x20\x00\x00\x00\x8e\x83\xd5\x92"
	"request_threaded_irq\0\0\0\0"
	"\x10\x00\x00\x00\xf9\x82\xa4\xf9"
	"msleep\0\0"
	"\x1c\x00\x00\x00\x6e\x64\xf7\xb3"
	"kthread_should_stop\0"
	"\x24\x00\x00\x00\xd9\xba\x2f\xe8"
	"gpiod_direction_output_raw\0\0"
	"\x14\x00\x00\x00\x3b\x4a\x51\xc1"
	"free_irq\0\0\0\0"
	"\x18\x00\x00\x00\x59\xa6\x03\x33"
	"module_layout\0\0\0"
	"\x00\x00\x00\x00\x00\x00\x00\x00";

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "8F641DA8C37E74EA355D11C");
