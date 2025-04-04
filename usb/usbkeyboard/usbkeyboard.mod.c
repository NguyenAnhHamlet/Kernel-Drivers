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
	"\x18\x00\x00\x00\xb8\x27\xe3\xd7"
	"usb_submit_urb\0\0"
	"\x10\x00\x00\x00\xd8\x7e\x99\x92"
	"_printk\0"
	"\x14\x00\x00\x00\xf3\x03\xde\xcd"
	"input_event\0"
	"\x1c\x00\x00\x00\xfb\xd5\x66\x78"
	"usb_register_driver\0"
	"\x18\x00\x00\x00\xe3\x55\x1c\xff"
	"usb_free_urb\0\0\0\0"
	"\x1c\x00\x00\x00\x45\x70\xe8\xc7"
	"usb_free_coherent\0\0\0"
	"\x20\x00\x00\x00\xab\x78\x9a\x51"
	"input_unregister_device\0"
	"\x10\x00\x00\x00\xba\x0c\x7a\x03"
	"kfree\0\0\0"
	"\x1c\x00\x00\x00\xc0\xfb\xc3\x6b"
	"__unregister_chrdev\0"
	"\x18\x00\x00\x00\xd0\xb4\x7e\x49"
	"usb_kill_urb\0\0\0\0"
	"\x18\x00\x00\x00\xb1\x40\x5a\x51"
	"usb_deregister\0\0"
	"\x28\x00\x00\x00\xb3\x1c\xa2\x87"
	"__ubsan_handle_out_of_bounds\0\0\0\0"
	"\x1c\x00\x00\x00\x63\xa5\x03\x4c"
	"random_kmalloc_seed\0"
	"\x18\x00\x00\x00\x70\x26\x45\xa1"
	"kmalloc_caches\0\0"
	"\x20\x00\x00\x00\x7e\xfb\xa7\x5f"
	"__kmalloc_cache_noprof\0\0"
	"\x18\x00\x00\x00\x25\x70\x00\xe8"
	"usb_alloc_urb\0\0\0"
	"\x1c\x00\x00\x00\xf3\xa1\x0a\xee"
	"usb_alloc_coherent\0\0"
	"\x20\x00\x00\x00\x1b\xe6\xdc\x4a"
	"input_allocate_device\0\0\0"
	"\x14\x00\x00\x00\x6e\x4a\x6e\x65"
	"snprintf\0\0\0\0"
	"\x10\x00\x00\x00\x94\xb6\x16\xa9"
	"strnlen\0"
	"\x10\x00\x00\x00\x7e\xa4\x29\x48"
	"memcpy\0\0"
	"\x20\x00\x00\x00\x3a\xab\x12\xf1"
	"input_register_device\0\0\0"
	"\x18\x00\x00\x00\xb5\x79\xca\x75"
	"__fortify_panic\0"
	"\x18\x00\x00\x00\x91\x64\x59\xe6"
	"module_layout\0\0\0"
	"\x00\x00\x00\x00\x00\x00\x00\x00";

MODULE_INFO(depends, "");

MODULE_ALIAS("usb:v258Ap002Ad*dc*dsc*dp*ic*isc*ip*in*");

MODULE_INFO(srcversion, "497CBA63234358BAD6EC871");
