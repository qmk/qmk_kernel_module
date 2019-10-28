// SPDX-License-Identifier: GPL-2.0+
/*
 * hid.c -- HID Composite driver
 *
 * Based on multi.c
 *
 * Copyright (C) 2010 Fabien Chouteau <fabien.chouteau@barco.com>
 */


#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/usb/composite.h>
#include <linux/usb/g_hid.h>

#define DRIVER_DESC     "QMK HID Gadget"
#define DRIVER_VERSION      "2010/03/16"

#include "qmk.h"
#include "u_hid.h"

/*-------------------------------------------------------------------------*/

#define HIDG_VENDOR_NUM     0x03A8  /* QMK */
#define HIDG_PRODUCT_NUM    0x0068  /* Planck */

/*-------------------------------------------------------------------------*/

struct hidg_func_node {
    struct usb_function_instance *fi;
    struct usb_function *f;
    struct list_head node;
    struct qmk_platform_data *pdata;
};

static LIST_HEAD(hidg_func_list);

/*-------------------------------------------------------------------------*/
USB_GADGET_COMPOSITE_OPTIONS();

static struct usb_device_descriptor device_desc = {
    .bLength =      sizeof device_desc,
    .bDescriptorType =  USB_DT_DEVICE,

    /* .bcdUSB = DYNAMIC */

    .bDeviceClass =     USB_CLASS_HID,
    .bDeviceSubClass =  2,
    .bDeviceProtocol =  1,
    /* .bMaxPacketSize0 = f(hardware) */

    /* Vendor and product id can be overridden by module parameters.  */
    .idVendor =     cpu_to_le16(HIDG_VENDOR_NUM),
    .idProduct =    cpu_to_le16(HIDG_PRODUCT_NUM),
    /* .bcdDevice = f(hardware) */
    /* .iManufacturer = DYNAMIC */
    /* .iProduct = DYNAMIC */
    /* NO SERIAL NUMBER */
    .bNumConfigurations =   1,
};

static unsigned char report_desc[63] = { 0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x05,
            0x07, 0x19, 0xe0, 0x29, 0xe7, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 
            0x95, 0x08, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x81, 0x03, 0x95, 
            0x05, 0x75, 0x01, 0x05, 0x08, 0x19, 0x01, 0x29, 0x05, 0x91, 0x02, 
            0x95, 0x01, 0x75, 0x03, 0x91, 0x03, 0x95, 0x06, 0x75, 0x08, 0x15, 
            0x00, 0x25, 0x65, 0x05, 0x07, 0x19, 0x00, 0x29, 0x65, 0x81, 0x00, 
            0xc0 };

static const struct usb_descriptor_header *otg_desc[2];

/* string IDs are assigned dynamically */
static struct usb_string strings_dev[] = {
    [USB_GADGET_MANUFACTURER_IDX].s = "OLKB",
    [USB_GADGET_PRODUCT_IDX].s = "Planck",
    [USB_GADGET_SERIAL_IDX].s = "1234",
    {  } /* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
    .language   = 0x0409,   /* en-us */
    .strings    = strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
    &stringtab_dev,
    NULL,
};



/****************************** Configurations ******************************/

static int do_config(struct usb_configuration *c)
{
    struct hidg_func_node *e, *n;
    int status = 0;

    if (gadget_is_otg(c->cdev->gadget)) {
        c->descriptors = otg_desc;
        c->bmAttributes |= USB_CONFIG_ATT_WAKEUP;
    }

    list_for_each_entry(e, &hidg_func_list, node) {
        e->f = usb_get_function(e->fi);
        if (IS_ERR(e->f))
            goto put;
        status = usb_add_function(c, e->f);
        if (status < 0) {
            usb_put_function(e->f);
            goto put;
        }
    }

    return 0;
put:
    list_for_each_entry(n, &hidg_func_list, node) {
        if (n == e)
            break;
        usb_remove_function(c, n->f);
        usb_put_function(n->f);
    }
    return status;
}

static struct usb_configuration config_driver = {
    .label          = "HID Gadget",
    .bConfigurationValue    = 1,
    /* .iConfiguration = DYNAMIC */
    .bmAttributes       = USB_CONFIG_ATT_SELFPOWER,
};

/****************************** Gadget Bind ******************************/

static int hid_bind(struct usb_composite_dev *cdev)
{
    struct usb_gadget *gadget = cdev->gadget;
    struct list_head *tmp;
    struct hidg_func_node *n, *m;
    struct f_hid_opts *hid_opts;
    int status, funcs = 0;

    list_for_each(tmp, &hidg_func_list)
        funcs++;

    if (!funcs)
        return -ENODEV;

    list_for_each_entry(n, &hidg_func_list, node) {
        n->fi = usb_get_function_instance("hid");
        if (IS_ERR(n->fi)) {
            status = PTR_ERR(n->fi);
            goto put;
        }
        hid_opts = container_of(n->fi, struct f_hid_opts, func_inst);
        // hid_opts->subclass = n->pdata->subclass;
        // hid_opts->protocol = n->pdata->protocol;
        // hid_opts->report_length = n->pdata->report_length;
        // hid_opts->report_desc_length = n->pdata->report_desc_length;
        // hid_opts->report_desc = n->pdata->report_desc;

        hid_opts->subclass = 1;
        hid_opts->protocol = 1;
        hid_opts->report_length = 8;
        hid_opts->report_desc_length = 63;
        hid_opts->report_desc = report_desc;
                
    }


    /* Allocate string descriptor numbers ... note that string
     * contents can be overridden by the composite_dev glue.
     */

    status = usb_string_ids_tab(cdev, strings_dev);
    if (status < 0)
        goto put;
    device_desc.iManufacturer = strings_dev[USB_GADGET_MANUFACTURER_IDX].id;
    device_desc.iProduct = strings_dev[USB_GADGET_PRODUCT_IDX].id;

    if (gadget_is_otg(gadget) && !otg_desc[0]) {
        struct usb_descriptor_header *usb_desc;

        usb_desc = usb_otg_descriptor_alloc(gadget);
        if (!usb_desc)
            goto put;
        usb_otg_descriptor_init(gadget, usb_desc);
        otg_desc[0] = usb_desc;
        otg_desc[1] = NULL;
    }

    /* register our configuration */
    status = usb_add_config(cdev, &config_driver, do_config);
    if (status < 0)
        goto free_otg_desc;

    usb_composite_overwrite_options(cdev, &coverwrite);
    dev_info(&gadget->dev, DRIVER_DESC ", version: " DRIVER_VERSION "\n");

    return 0;

free_otg_desc:
    kfree(otg_desc[0]);
    otg_desc[0] = NULL;
put:
    list_for_each_entry(m, &hidg_func_list, node) {
        if (m == n)
            break;
        usb_put_function_instance(m->fi);
    }
    return status;
}

static int hid_unbind(struct usb_composite_dev *cdev)
{
    struct hidg_func_node *n;

    list_for_each_entry(n, &hidg_func_list, node) {
        usb_put_function(n->f);
        usb_put_function_instance(n->fi);
    }

    kfree(otg_desc[0]);
    otg_desc[0] = NULL;

    return 0;
}

static struct usb_composite_driver hidg_driver = {
    .name       = "qmk_hid",
    .dev        = &device_desc,
    .strings    = dev_strings,
    .max_speed  = USB_SPEED_HIGH,
    .bind       = hid_bind,
    .unbind     = hid_unbind,
};

int hidg_plat_driver_probe(struct platform_device *pdev)
{
    struct qmk_platform_data *pdata = dev_get_platdata(&pdev->dev);
    struct hidg_func_node *entry;

    int status = usb_composite_probe(&hidg_driver);
    if (status < 0) {
        dev_err(&pdev->dev, "USB composite probe failed\n");
        return status;
    }

    entry = kzalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry)
        return -ENOMEM;

    entry->pdata = pdata;
    list_add_tail(&entry->node, &hidg_func_list);

    return 0;
}

int hidg_plat_driver_remove(struct platform_device *pdev)
{
    struct hidg_func_node *e, *n;

    usb_composite_unregister(&hidg_driver);

    list_for_each_entry_safe(e, n, &hidg_func_list, node) {
        list_del(&e->node);
        kfree(e);
    }

    return 0;
}

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("Fabien Chouteau, Peter Korsgaard");
MODULE_LICENSE("GPL");

int hidg_init(void)
{   
    return 0;
}

void hidg_cleanup(void)
{
}
