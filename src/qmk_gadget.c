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
#include <linux/usb/otg.h>

#define DRIVER_DESC     "QMK HID Gadget"
#define DRIVER_VERSION      "2010/03/16"

#include "qmk.h"
#include "u_hid.h"
#include "u_os_desc.h"

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

struct usb_os_string {
    __u8    bLength;
    __u8    bDescriptorType;
    __u8    qwSignature[OS_STRING_QW_SIGN_LEN];
    __u8    bMS_VendorCode;
    __u8    bPad;
} __packed;

static LIST_HEAD(hidg_func_list);

/*-------------------------------------------------------------------------*/
USB_GADGET_COMPOSITE_OPTIONS();

static struct usb_device_descriptor device_desc = {
    .bLength =      sizeof device_desc,
    .bDescriptorType =  USB_DT_OTG,

    .bcdUSB = 0x0200,

    .bDeviceClass =     USB_CLASS_PER_INTERFACE,
    .bDeviceSubClass =  0,
    .bDeviceProtocol =  0,
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

/*
 * The code in this file is utility code, used to build a gadget driver
 * from one or more "function" drivers, one or more "configuration"
 * objects, and a "usb_composite_driver" by gluing them together along
 * with the relevant device-wide data.
 */

static struct usb_gadget_strings **get_containers_gs(
        struct usb_gadget_string_container *uc)
{
    return (struct usb_gadget_strings **)uc->stash;
}


static struct usb_descriptor_header **
function_descriptors(struct usb_function *f,
             enum usb_device_speed speed)
{
    struct usb_descriptor_header **descriptors;

    /*
     * NOTE: we try to help gadget drivers which might not be setting
     * max_speed appropriately.
     */

    switch (speed) {
    case USB_SPEED_SUPER_PLUS:
        descriptors = f->ssp_descriptors;
        if (descriptors)
            break;
        /* FALLTHROUGH */
    case USB_SPEED_SUPER:
        descriptors = f->ss_descriptors;
        if (descriptors)
            break;
        /* FALLTHROUGH */
    case USB_SPEED_HIGH:
        descriptors = f->hs_descriptors;
        if (descriptors)
            break;
        /* FALLTHROUGH */
    default:
        descriptors = f->fs_descriptors;
    }

    /*
     * if we can't find any descriptors at all, then this gadget deserves to
     * Oops with a NULL pointer dereference
     */

    return descriptors;
}


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

static int count_configs(struct usb_composite_dev *cdev, unsigned type)
{
    struct usb_gadget       *gadget = cdev->gadget;
    struct usb_configuration    *c;
    unsigned            count = 0;
    int             hs = 0;
    int             ss = 0;
    int             ssp = 0;

    if (gadget_is_dualspeed(gadget)) {
        if (gadget->speed == USB_SPEED_HIGH)
            hs = 1;
        if (gadget->speed == USB_SPEED_SUPER)
            ss = 1;
        if (gadget->speed == USB_SPEED_SUPER_PLUS)
            ssp = 1;
        if (type == USB_DT_DEVICE_QUALIFIER)
            hs = !hs;
    }
    list_for_each_entry(c, &cdev->configs, list) {
        /* ignore configs that won't work at this speed */
        if (ssp) {
            if (!c->superspeed_plus)
                continue;
        } else if (ss) {
            if (!c->superspeed)
                continue;
        } else if (hs) {
            if (!c->highspeed)
                continue;
        } else {
            if (!c->fullspeed)
                continue;
        }
        count++;
    }
    return count;
}

static void reset_config(struct usb_composite_dev *cdev)
{
    struct usb_function     *f;

    DBG(cdev, "reset config\n");

    list_for_each_entry(f, &cdev->config->functions, list) {
        if (f->disable)
            f->disable(f);

        bitmap_zero(f->endpoints, 32);
    }
    cdev->config = NULL;
    cdev->delayed_status = 0;
}

static int set_config(struct usb_composite_dev *cdev,
        const struct usb_ctrlrequest *ctrl, unsigned number)
{
    struct usb_gadget   *gadget = cdev->gadget;
    struct usb_configuration *c = NULL;
    int         result = -EINVAL;
    unsigned        power = gadget_is_otg(gadget) ? 8 : 100;
    int         tmp;

    if (number) {
        list_for_each_entry(c, &cdev->configs, list) {
            if (c->bConfigurationValue == number) {
                /*
                 * We disable the FDs of the previous
                 * configuration only if the new configuration
                 * is a valid one
                 */
                if (cdev->config)
                    reset_config(cdev);
                result = 0;
                break;
            }
        }
        if (result < 0)
            goto done;
    } else { /* Zero configuration value - need to reset the config */
        if (cdev->config)
            reset_config(cdev);
        result = 0;
    }

    INFO(cdev, "%s config #%d: %s\n",
         usb_speed_string(gadget->speed),
         number, c ? c->label : "unconfigured");

    if (!c)
        goto done;

    usb_gadget_set_state(gadget, USB_STATE_CONFIGURED);
    cdev->config = c;

    /* Initialize all interfaces by setting them to altsetting zero. */
    for (tmp = 0; tmp < MAX_CONFIG_INTERFACES; tmp++) {
        struct usb_function *f = c->interface[tmp];
        struct usb_descriptor_header **descriptors;

        if (!f)
            break;

        /*
         * Record which endpoints are used by the function. This is used
         * to dispatch control requests targeted at that endpoint to the
         * function's setup callback instead of the current
         * configuration's setup callback.
         */
        descriptors = function_descriptors(f, gadget->speed);

        for (; *descriptors; ++descriptors) {
            struct usb_endpoint_descriptor *ep;
            int addr;

            if ((*descriptors)->bDescriptorType != USB_DT_ENDPOINT)
                continue;

            ep = (struct usb_endpoint_descriptor *)*descriptors;
            addr = ((ep->bEndpointAddress & 0x80) >> 3)
                 |  (ep->bEndpointAddress & 0x0f);
            set_bit(addr, f->endpoints);
        }

        result = f->set_alt(f, tmp, 0);
        if (result < 0) {
            DBG(cdev, "interface %d (%s/%p) alt 0 --> %d\n",
                    tmp, f->name, f, result);

            reset_config(cdev);
            goto done;
        }

        if (result == USB_GADGET_DELAYED_STATUS) {
            DBG(cdev,
             "%s: interface %d (%s) requested delayed status\n",
                    __func__, tmp, f->name);
            cdev->delayed_status++;
            DBG(cdev, "delayed_status count %d\n",
                    cdev->delayed_status);
        }
    }

    /* when we return, be sure our power usage is valid */
    power = c->MaxPower ? c->MaxPower : CONFIG_USB_GADGET_VBUS_DRAW;
done:
    usb_gadget_vbus_draw(gadget, power);
    if (result >= 0 && cdev->delayed_status)
        result = USB_GADGET_DELAYED_STATUS;
    return result;
}

static void remove_config(struct usb_composite_dev *cdev,
                  struct usb_configuration *config)
{
    while (!list_empty(&config->functions)) {
        struct usb_function     *f;

        f = list_first_entry(&config->functions,
                struct usb_function, list);

        usb_remove_function(config, f);
    }
    list_del(&config->list);
    if (config->unbind) {
        DBG(cdev, "unbind config '%s'/%p\n", config->label, config);
        config->unbind(config);
            /* may free memory for "c" */
    }
}

/**
 * usb_remove_config() - remove a configuration from a device.
 * @cdev: wraps the USB gadget
 * @config: the configuration
 *
 * Drivers must call usb_gadget_disconnect before calling this function
 * to disconnect the device from the host and make sure the host will not
 * try to enumerate the device while we are changing the config list.
 */
void usb_remove_config(struct usb_composite_dev *cdev,
              struct usb_configuration *config)
{
    unsigned long flags;

    spin_lock_irqsave(&cdev->lock, flags);

    if (cdev->config == config)
        reset_config(cdev);

    spin_unlock_irqrestore(&cdev->lock, flags);

    remove_config(cdev, config);
}

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

static void composite_setup_complete(struct usb_ep *ep, struct usb_request *req)
{
    struct usb_composite_dev *cdev;

    if (req->status || req->actual != req->length)
        DBG((struct usb_composite_dev *) ep->driver_data,
                "setup complete --> %d, %d/%d\n",
                req->status, req->actual, req->length);

    /*
     * REVIST The same ep0 requests are shared with function drivers
     * so they don't have to maintain the same ->complete() stubs.
     *
     * Because of that, we need to check for the validity of ->context
     * here, even though we know we've set it to something useful.
     */
    if (!req->context)
        return;

    cdev = req->context;

    if (cdev->req == req)
        cdev->setup_pending = false;
    else if (cdev->os_desc_req == req)
        cdev->os_desc_pending = false;
    else
        WARN(1, "unknown request %p\n", req);
}

static int composite_ep0_queue(struct usb_composite_dev *cdev,
        struct usb_request *req, gfp_t gfp_flags)
{
    int ret;

    ret = usb_ep_queue(cdev->gadget->ep0, req, gfp_flags);
    if (ret == 0) {
        if (cdev->req == req)
            cdev->setup_pending = true;
        else if (cdev->os_desc_req == req)
            cdev->os_desc_pending = true;
        else
            WARN(1, "unknown request %p\n", req);
    }

    return ret;
}

static int count_ext_compat(struct usb_configuration *c)
{
    int i, res;

    res = 0;
    for (i = 0; i < c->next_interface_id; ++i) {
        struct usb_function *f;
        int j;

        f = c->interface[i];
        for (j = 0; j < f->os_desc_n; ++j) {
            struct usb_os_desc *d;

            if (i != f->os_desc_table[j].if_id)
                continue;
            d = f->os_desc_table[j].os_desc;
            if (d && d->ext_compat_id)
                ++res;
        }
    }
    BUG_ON(res > 255);
    return res;
}

static int fill_ext_compat(struct usb_configuration *c, u8 *buf)
{
    int i, count;

    count = 16;
    buf += 16;
    for (i = 0; i < c->next_interface_id; ++i) {
        struct usb_function *f;
        int j;

        f = c->interface[i];
        for (j = 0; j < f->os_desc_n; ++j) {
            struct usb_os_desc *d;

            if (i != f->os_desc_table[j].if_id)
                continue;
            d = f->os_desc_table[j].os_desc;
            if (d && d->ext_compat_id) {
                *buf++ = i;
                *buf++ = 0x01;
                memcpy(buf, d->ext_compat_id, 16);
                buf += 22;
            } else {
                ++buf;
                *buf = 0x01;
                buf += 23;
            }
            count += 24;
            if (count + 24 >= USB_COMP_EP0_OS_DESC_BUFSIZ)
                return count;
        }
    }

    return count;
}

static int count_ext_prop(struct usb_configuration *c, int interface)
{
    struct usb_function *f;
    int j;

    f = c->interface[interface];
    for (j = 0; j < f->os_desc_n; ++j) {
        struct usb_os_desc *d;

        if (interface != f->os_desc_table[j].if_id)
            continue;
        d = f->os_desc_table[j].os_desc;
        if (d && d->ext_compat_id)
            return d->ext_prop_count;
    }
    return 0;
}

static int len_ext_prop(struct usb_configuration *c, int interface)
{
    struct usb_function *f;
    struct usb_os_desc *d;
    int j, res;

    res = 10; /* header length */
    f = c->interface[interface];
    for (j = 0; j < f->os_desc_n; ++j) {
        if (interface != f->os_desc_table[j].if_id)
            continue;
        d = f->os_desc_table[j].os_desc;
        if (d)
            return min(res + d->ext_prop_len, 4096);
    }
    return res;
}

static int fill_ext_prop(struct usb_configuration *c, int interface, u8 *buf)
{
    struct usb_function *f;
    struct usb_os_desc *d;
    struct usb_os_desc_ext_prop *ext_prop;
    int j, count, n, ret;

    f = c->interface[interface];
    count = 10; /* header length */
    buf += 10;
    for (j = 0; j < f->os_desc_n; ++j) {
        if (interface != f->os_desc_table[j].if_id)
            continue;
        d = f->os_desc_table[j].os_desc;
        if (d)
            list_for_each_entry(ext_prop, &d->ext_prop, entry) {
                n = ext_prop->data_len +
                    ext_prop->name_len + 14;
                if (count + n >= USB_COMP_EP0_OS_DESC_BUFSIZ)
                    return count;
                usb_ext_prop_put_size(buf, n);
                usb_ext_prop_put_type(buf, ext_prop->type);
                ret = usb_ext_prop_put_name(buf, ext_prop->name,
                                ext_prop->name_len);
                if (ret < 0)
                    return ret;
                switch (ext_prop->type) {
                case USB_EXT_PROP_UNICODE:
                case USB_EXT_PROP_UNICODE_ENV:
                case USB_EXT_PROP_UNICODE_LINK:
                    usb_ext_prop_put_unicode(buf, ret,
                             ext_prop->data,
                             ext_prop->data_len);
                    break;
                case USB_EXT_PROP_BINARY:
                    usb_ext_prop_put_binary(buf, ret,
                            ext_prop->data,
                            ext_prop->data_len);
                    break;
                case USB_EXT_PROP_LE32:
                    /* not implemented */
                case USB_EXT_PROP_BE32:
                    /* not implemented */
                default:
                    return -EINVAL;
                }
                buf += n;
                count += n;
            }
    }

    return count;
}


int
composite_setup(struct usb_gadget *gadget, const struct usb_ctrlrequest *ctrl)
{
    struct usb_composite_dev    *cdev = get_gadget_data(gadget);
    struct usb_request      *req = cdev->req;
    int             value = -EOPNOTSUPP;
    int             status = 0;
    u16             w_index = le16_to_cpu(ctrl->wIndex);
    u8              intf = w_index & 0xFF;
    u16             w_value = le16_to_cpu(ctrl->wValue);
    u16             w_length = le16_to_cpu(ctrl->wLength);
    struct usb_function     *f = NULL;
    u8              endp;

    /* partial re-init of the response message; the function or the
     * gadget might need to intercept e.g. a control-OUT completion
     * when we delegate to it.
     */
    req->zero = 0;
    req->context = cdev;
    req->complete = composite_setup_complete;
    req->length = 0;
    gadget->ep0->driver_data = cdev;

    /*
     * Don't let non-standard requests match any of the cases below
     * by accident.
     */
    if ((ctrl->bRequestType & USB_TYPE_MASK) != USB_TYPE_STANDARD)
        goto unknown;

    switch (ctrl->bRequest) {

    /* we handle all standard USB descriptors */
    case USB_REQ_GET_DESCRIPTOR:
        if (ctrl->bRequestType != USB_DIR_IN)
            goto unknown;
        switch (w_value >> 8) {

        case USB_DT_DEVICE:
            cdev->desc.bNumConfigurations =
                count_configs(cdev, USB_DT_DEVICE);
            cdev->desc.bMaxPacketSize0 =
                cdev->gadget->ep0->maxpacket;
            if (gadget_is_superspeed(gadget)) {
                if (gadget->speed >= USB_SPEED_SUPER) {
                    cdev->desc.bcdUSB = cpu_to_le16(0x0320);
                    cdev->desc.bMaxPacketSize0 = 9;
                } else {
                    cdev->desc.bcdUSB = cpu_to_le16(0x0210);
                }
            } else {
                if (gadget->lpm_capable)
                    cdev->desc.bcdUSB = cpu_to_le16(0x0201);
                else
                    cdev->desc.bcdUSB = cpu_to_le16(0x0200);
            }

            value = min(w_length, (u16) sizeof cdev->desc);
            memcpy(req->buf, &cdev->desc, value);
            break;
        case USB_DT_OTG:
            if (gadget_is_otg(gadget)) {
                struct usb_configuration *config;
                int otg_desc_len = 0;

                if (cdev->config)
                    config = cdev->config;
                else
                    config = list_first_entry(
                            &cdev->configs,
                        struct usb_configuration, list);
                if (!config)
                    goto done;

                if (gadget->otg_caps &&
                    (gadget->otg_caps->otg_rev >= 0x0200))
                    otg_desc_len += sizeof(
                        struct usb_otg20_descriptor);
                else
                    otg_desc_len += sizeof(
                        struct usb_otg_descriptor);

                value = min_t(int, w_length, otg_desc_len);
                memcpy(req->buf, config->descriptors[0], value);
            }
            break;
        }
        break;

    /* any number of configs can work */
    case USB_REQ_SET_CONFIGURATION:
        if (ctrl->bRequestType != 0)
            goto unknown;
        if (gadget_is_otg(gadget)) {
            if (gadget->a_hnp_support)
                DBG(cdev, "HNP available\n");
            else if (gadget->a_alt_hnp_support)
                DBG(cdev, "HNP on another port\n");
            else
                VDBG(cdev, "HNP inactive\n");
        }
        spin_lock(&cdev->lock);
        value = set_config(cdev, ctrl, w_value);
        spin_unlock(&cdev->lock);
        break;
    case USB_REQ_GET_CONFIGURATION:
        if (ctrl->bRequestType != USB_DIR_IN)
            goto unknown;
        if (cdev->config)
            *(u8 *)req->buf = cdev->config->bConfigurationValue;
        else
            *(u8 *)req->buf = 0;
        value = min(w_length, (u16) 1);
        break;

    /* function drivers must handle get/set altsetting */
    case USB_REQ_SET_INTERFACE:
        if (ctrl->bRequestType != USB_RECIP_INTERFACE)
            goto unknown;
        if (!cdev->config || intf >= MAX_CONFIG_INTERFACES)
            break;
        f = cdev->config->interface[intf];
        if (!f)
            break;

        /*
         * If there's no get_alt() method, we know only altsetting zero
         * works. There is no need to check if set_alt() is not NULL
         * as we check this in usb_add_function().
         */
        if (w_value && !f->get_alt)
            break;

        spin_lock(&cdev->lock);
        value = f->set_alt(f, w_index, w_value);
        if (value == USB_GADGET_DELAYED_STATUS) {
            DBG(cdev,
             "%s: interface %d (%s) requested delayed status\n",
                    __func__, intf, f->name);
            cdev->delayed_status++;
            DBG(cdev, "delayed_status count %d\n",
                    cdev->delayed_status);
        }
        spin_unlock(&cdev->lock);
        break;
    case USB_REQ_GET_INTERFACE:
        if (ctrl->bRequestType != (USB_DIR_IN|USB_RECIP_INTERFACE))
            goto unknown;
        if (!cdev->config || intf >= MAX_CONFIG_INTERFACES)
            break;
        f = cdev->config->interface[intf];
        if (!f)
            break;
        /* lots of interfaces only need altsetting zero... */
        value = f->get_alt ? f->get_alt(f, w_index) : 0;
        if (value < 0)
            break;
        *((u8 *)req->buf) = value;
        value = min(w_length, (u16) 1);
        break;
    case USB_REQ_GET_STATUS:
        if (gadget_is_otg(gadget) && gadget->hnp_polling_support &&
                        (w_index == OTG_STS_SELECTOR)) {
            if (ctrl->bRequestType != (USB_DIR_IN |
                            USB_RECIP_DEVICE))
                goto unknown;
            *((u8 *)req->buf) = gadget->host_request_flag;
            value = 1;
            break;
        }

        /*
         * USB 3.0 additions:
         * Function driver should handle get_status request. If such cb
         * wasn't supplied we respond with default value = 0
         * Note: function driver should supply such cb only for the
         * first interface of the function
         */
        if (!gadget_is_superspeed(gadget))
            goto unknown;
        if (ctrl->bRequestType != (USB_DIR_IN | USB_RECIP_INTERFACE))
            goto unknown;
        value = 2;  /* This is the length of the get_status reply */
        put_unaligned_le16(0, req->buf);
        if (!cdev->config || intf >= MAX_CONFIG_INTERFACES)
            break;
        f = cdev->config->interface[intf];
        if (!f)
            break;
        status = f->get_status ? f->get_status(f) : 0;
        if (status < 0)
            break;
        put_unaligned_le16(status & 0x0000ffff, req->buf);
        break;
    /*
     * Function drivers should handle SetFeature/ClearFeature
     * (FUNCTION_SUSPEND) request. function_suspend cb should be supplied
     * only for the first interface of the function
     */
    case USB_REQ_CLEAR_FEATURE:
    case USB_REQ_SET_FEATURE:
        if (!gadget_is_superspeed(gadget))
            goto unknown;
        if (ctrl->bRequestType != (USB_DIR_OUT | USB_RECIP_INTERFACE))
            goto unknown;
        switch (w_value) {
        case USB_INTRF_FUNC_SUSPEND:
            if (!cdev->config || intf >= MAX_CONFIG_INTERFACES)
                break;
            f = cdev->config->interface[intf];
            if (!f)
                break;
            value = 0;
            if (f->func_suspend)
                value = f->func_suspend(f, w_index >> 8);
            if (value < 0) {
                ERROR(cdev,
                      "func_suspend() returned error %d\n",
                      value);
                value = 0;
            }
            break;
        }
        break;
    default:
unknown:
        /*
         * OS descriptors handling
         */
        if (cdev->use_os_string && cdev->os_desc_config &&
            (ctrl->bRequestType & USB_TYPE_VENDOR) &&
            ctrl->bRequest == cdev->b_vendor_code) {
            struct usb_configuration    *os_desc_cfg;
            u8              *buf;
            int             interface;
            int             count = 0;

            req = cdev->os_desc_req;
            req->context = cdev;
            req->complete = composite_setup_complete;
            buf = req->buf;
            os_desc_cfg = cdev->os_desc_config;
            w_length = min_t(u16, w_length, USB_COMP_EP0_OS_DESC_BUFSIZ);
            memset(buf, 0, w_length);
            buf[5] = 0x01;
            switch (ctrl->bRequestType & USB_RECIP_MASK) {
            case USB_RECIP_DEVICE:
                if (w_index != 0x4 || (w_value >> 8))
                    break;
                buf[6] = w_index;
                /* Number of ext compat interfaces */
                count = count_ext_compat(os_desc_cfg);
                buf[8] = count;
                count *= 24; /* 24 B/ext compat desc */
                count += 16; /* header */
                put_unaligned_le32(count, buf);
                value = w_length;
                if (w_length > 0x10) {
                    value = fill_ext_compat(os_desc_cfg, buf);
                    value = min_t(u16, w_length, value);
                }
                break;
            case USB_RECIP_INTERFACE:
                if (w_index != 0x5 || (w_value >> 8))
                    break;
                interface = w_value & 0xFF;
                buf[6] = w_index;
                count = count_ext_prop(os_desc_cfg,
                    interface);
                put_unaligned_le16(count, buf + 8);
                count = len_ext_prop(os_desc_cfg,
                    interface);
                put_unaligned_le32(count, buf);
                value = w_length;
                if (w_length > 0x0A) {
                    value = fill_ext_prop(os_desc_cfg,
                                  interface, buf);
                    if (value >= 0)
                        value = min_t(u16, w_length, value);
                }
                break;
            }

            goto check_value;
        }

        VDBG(cdev,
            "non-core control req%02x.%02x v%04x i%04x l%d\n",
            ctrl->bRequestType, ctrl->bRequest,
            w_value, w_index, w_length);

        /* functions always handle their interfaces and endpoints...
         * punt other recipients (other, WUSB, ...) to the current
         * configuration code.
         */
        if (cdev->config) {
            list_for_each_entry(f, &cdev->config->functions, list)
                if (f->req_match &&
                    f->req_match(f, ctrl, false))
                    goto try_fun_setup;
        } else {
            struct usb_configuration *c;
            list_for_each_entry(c, &cdev->configs, list)
                list_for_each_entry(f, &c->functions, list)
                    if (f->req_match &&
                        f->req_match(f, ctrl, true))
                        goto try_fun_setup;
        }
        f = NULL;

        switch (ctrl->bRequestType & USB_RECIP_MASK) {
        case USB_RECIP_INTERFACE:
            if (!cdev->config || intf >= MAX_CONFIG_INTERFACES)
                break;
            f = cdev->config->interface[intf];
            break;

        case USB_RECIP_ENDPOINT:
            if (!cdev->config)
                break;
            endp = ((w_index & 0x80) >> 3) | (w_index & 0x0f);
            list_for_each_entry(f, &cdev->config->functions, list) {
                if (test_bit(endp, f->endpoints))
                    break;
            }
            if (&f->list == &cdev->config->functions)
                f = NULL;
            break;
        }
try_fun_setup:
        if (f && f->setup)
            value = f->setup(f, ctrl);
        else {
            struct usb_configuration    *c;

            c = cdev->config;
            if (!c)
                goto done;

            /* try current config's setup */
            if (c->setup) {
                value = c->setup(c, ctrl);
                goto done;
            }

            /* try the only function in the current config */
            if (!list_is_singular(&c->functions))
                goto done;
            f = list_first_entry(&c->functions, struct usb_function,
                         list);
            if (f->setup)
                value = f->setup(f, ctrl);
        }

        goto done;
    }

check_value:
    /* respond with data transfer before status phase? */
    if (value >= 0 && value != USB_GADGET_DELAYED_STATUS) {
        req->length = value;
        req->context = cdev;
        req->zero = value < w_length;
        value = composite_ep0_queue(cdev, req, GFP_ATOMIC);
        if (value < 0) {
            DBG(cdev, "ep_queue --> %d\n", value);
            req->status = 0;
            composite_setup_complete(gadget->ep0, req);
        }
    } else if (value == USB_GADGET_DELAYED_STATUS && w_length != 0) {
        WARN(cdev,
            "%s: Delayed status not supported for w_length != 0",
            __func__);
    }

done:
    /* device either stalls (value < 0) or reports success */
    return value;
}

void composite_disconnect(struct usb_gadget *gadget)
{
    struct usb_composite_dev    *cdev = get_gadget_data(gadget);
    unsigned long           flags;

    /* REVISIT:  should we have config and device level
     * disconnect callbacks?
     */
    spin_lock_irqsave(&cdev->lock, flags);
    cdev->suspended = 0;
    if (cdev->config)
        reset_config(cdev);
    if (cdev->driver->disconnect)
        cdev->driver->disconnect(cdev);
    spin_unlock_irqrestore(&cdev->lock, flags);
}

static void __composite_unbind(struct usb_gadget *gadget, bool unbind_driver)
{
    struct usb_composite_dev    *cdev = get_gadget_data(gadget);
    struct usb_gadget_strings   *gstr = cdev->driver->strings[0];
    struct usb_string       *dev_str = gstr->strings;

    /* composite_disconnect() must already have been called
     * by the underlying peripheral controller driver!
     * so there's no i/o concurrency that could affect the
     * state protected by cdev->lock.
     */
    WARN_ON(cdev->config);

    while (!list_empty(&cdev->configs)) {
        struct usb_configuration    *c;
        c = list_first_entry(&cdev->configs,
                struct usb_configuration, list);
        remove_config(cdev, c);
    }
    if (cdev->driver->unbind && unbind_driver)
        cdev->driver->unbind(cdev);

    composite_dev_cleanup(cdev);

    if (dev_str[USB_GADGET_MANUFACTURER_IDX].s == cdev->def_manufacturer)
        dev_str[USB_GADGET_MANUFACTURER_IDX].s = "";

    kfree(cdev->def_manufacturer);
    kfree(cdev);
    set_gadget_data(gadget, NULL);
}

static void composite_unbind(struct usb_gadget *gadget)
{
    __composite_unbind(gadget, true);
}

static void update_unchanged_dev_desc(struct usb_device_descriptor *new,
        const struct usb_device_descriptor *old)
{
    __le16 idVendor;
    __le16 idProduct;
    __le16 bcdDevice;
    u8 iSerialNumber;
    u8 iManufacturer;
    u8 iProduct;

    /*
     * these variables may have been set in
     * usb_composite_overwrite_options()
     */
    idVendor = new->idVendor;
    idProduct = new->idProduct;
    bcdDevice = new->bcdDevice;
    iSerialNumber = new->iSerialNumber;
    iManufacturer = new->iManufacturer;
    iProduct = new->iProduct;

    *new = *old;
    if (idVendor)
        new->idVendor = idVendor;
    if (idProduct)
        new->idProduct = idProduct;
    if (bcdDevice)
        new->bcdDevice = bcdDevice;
    else
        new->bcdDevice = cpu_to_le16(get_default_bcdDevice());
    if (iSerialNumber)
        new->iSerialNumber = iSerialNumber;
    if (iManufacturer)
        new->iManufacturer = iManufacturer;
    if (iProduct)
        new->iProduct = iProduct;
}

int composite_dev_prepare(struct usb_composite_driver *composite,
        struct usb_composite_dev *cdev)
{
    struct usb_gadget *gadget = cdev->gadget;
    int ret = -ENOMEM;

    /* preallocate control response and buffer */
    cdev->req = usb_ep_alloc_request(gadget->ep0, GFP_KERNEL);
    if (!cdev->req)
        return -ENOMEM;

    cdev->req->buf = kmalloc(USB_COMP_EP0_BUFSIZ, GFP_KERNEL);
    if (!cdev->req->buf)
        goto fail;

    cdev->req->complete = composite_setup_complete;
    cdev->req->context = cdev;
    gadget->ep0->driver_data = cdev;

    cdev->driver = composite;

    /*
     * As per USB compliance update, a device that is actively drawing
     * more than 100mA from USB must report itself as bus-powered in
     * the GetStatus(DEVICE) call.
     */
    if (CONFIG_USB_GADGET_VBUS_DRAW <= USB_SELF_POWER_VBUS_MAX_DRAW)
        usb_gadget_set_selfpowered(gadget);

    /* interface and string IDs start at zero via kzalloc.
     * we force endpoints to start unassigned; few controller
     * drivers will zero ep->driver_data.
     */
    usb_ep_autoconfig_reset(gadget);
    return 0;
fail_dev:
    kfree(cdev->req->buf);
fail:
    usb_ep_free_request(gadget->ep0, cdev->req);
    cdev->req = NULL;
    return ret;
}

int composite_os_desc_req_prepare(struct usb_composite_dev *cdev,
                  struct usb_ep *ep0)
{
    int ret = 0;

    cdev->os_desc_req = usb_ep_alloc_request(ep0, GFP_KERNEL);
    if (!cdev->os_desc_req) {
        ret = -ENOMEM;
        goto end;
    }

    cdev->os_desc_req->buf = kmalloc(USB_COMP_EP0_OS_DESC_BUFSIZ,
                     GFP_KERNEL);
    if (!cdev->os_desc_req->buf) {
        ret = -ENOMEM;
        usb_ep_free_request(ep0, cdev->os_desc_req);
        goto end;
    }
    cdev->os_desc_req->context = cdev;
    cdev->os_desc_req->complete = composite_setup_complete;
end:
    return ret;
}

void composite_dev_cleanup(struct usb_composite_dev *cdev)
{
    struct usb_gadget_string_container *uc, *tmp;
    struct usb_ep              *ep, *tmp_ep;

    list_for_each_entry_safe(uc, tmp, &cdev->gstrings, list) {
        list_del(&uc->list);
        kfree(uc);
    }
    if (cdev->os_desc_req) {
        if (cdev->os_desc_pending)
            usb_ep_dequeue(cdev->gadget->ep0, cdev->os_desc_req);

        kfree(cdev->os_desc_req->buf);
        usb_ep_free_request(cdev->gadget->ep0, cdev->os_desc_req);
    }
    if (cdev->req) {
        if (cdev->setup_pending)
            usb_ep_dequeue(cdev->gadget->ep0, cdev->req);

        kfree(cdev->req->buf);
        usb_ep_free_request(cdev->gadget->ep0, cdev->req);
    }
    cdev->next_string_id = 0;

    /*
     * Some UDC backends have a dynamic EP allocation scheme.
     *
     * In that case, the dispose() callback is used to notify the
     * backend that the EPs are no longer in use.
     *
     * Note: The UDC backend can remove the EP from the ep_list as
     *   a result, so we need to use the _safe list iterator.
     */
    list_for_each_entry_safe(ep, tmp_ep,
                 &cdev->gadget->ep_list, ep_list) {
        if (ep->ops->dispose)
            ep->ops->dispose(ep);
    }
}

static int composite_bind(struct usb_gadget *gadget,
        struct usb_gadget_driver *gdriver)
{
    struct usb_composite_dev    *cdev;
    struct usb_composite_driver *composite = to_cdriver(gdriver);
    int             status = -ENOMEM;

    cdev = kzalloc(sizeof *cdev, GFP_KERNEL);
    if (!cdev)
        return status;

    spin_lock_init(&cdev->lock);
    cdev->gadget = gadget;
    set_gadget_data(gadget, cdev);
    INIT_LIST_HEAD(&cdev->configs);
    INIT_LIST_HEAD(&cdev->gstrings);

    status = composite_dev_prepare(composite, cdev);
    if (status)
        goto fail;

    /* composite gadget needs to assign strings for whole device (like
     * serial number), register function drivers, potentially update
     * power state and consumption, etc
     */
    status = hid_bind(cdev);
    if (status < 0)
        goto fail;

    if (cdev->use_os_string) {
        status = composite_os_desc_req_prepare(cdev, gadget->ep0);
        if (status)
            goto fail;
    }

    update_unchanged_dev_desc(&cdev->desc, composite->dev);

    /* has userspace failed to provide a serial number? */
    if (composite->needs_serial && !cdev->desc.iSerialNumber)
        WARNING(cdev, "userspace failed to provide iSerialNumber\n");

    INFO(cdev, "%s ready\n", composite->name);
    return 0;

fail:
    __composite_unbind(gadget, false);
    return status;
}

void composite_suspend(struct usb_gadget *gadget)
{
    struct usb_composite_dev    *cdev = get_gadget_data(gadget);
    struct usb_function     *f;

    /* REVISIT:  should we have config level
     * suspend/resume callbacks?
     */
    DBG(cdev, "suspend\n");
    if (cdev->config) {
        list_for_each_entry(f, &cdev->config->functions, list) {
            if (f->suspend)
                f->suspend(f);
        }
    }
    if (cdev->driver->suspend)
        cdev->driver->suspend(cdev);

    cdev->suspended = 1;

    usb_gadget_vbus_draw(gadget, 2);
}

void composite_resume(struct usb_gadget *gadget)
{
    struct usb_composite_dev    *cdev = get_gadget_data(gadget);
    struct usb_function     *f;
    u16             maxpower;

    /* REVISIT:  should we have config level
     * suspend/resume callbacks?
     */
    DBG(cdev, "resume\n");
    if (cdev->driver->resume)
        cdev->driver->resume(cdev);
    if (cdev->config) {
        list_for_each_entry(f, &cdev->config->functions, list) {
            if (f->resume)
                f->resume(f);
        }

        maxpower = cdev->config->MaxPower;

        usb_gadget_vbus_draw(gadget, maxpower ?
            maxpower : CONFIG_USB_GADGET_VBUS_DRAW);
    }

    cdev->suspended = 0;
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

static const struct usb_gadget_driver qmk_gadget_driver = {
    .function       = "qmk",
    .bind           = composite_bind,
    .unbind         = composite_unbind,

    .setup          = composite_setup,
    .reset          = composite_disconnect,
    .disconnect     = composite_disconnect,

    .suspend        = composite_suspend,
    .resume         = composite_resume,

    .max_speed      = USB_SPEED_SUPER,
    .driver = {
        .owner      = THIS_MODULE,
        .name       = "qmk",
    },
    .match_existing_only = 1,
};

static const struct usb_composite_driver hidg_driver = {
    .name           = "qmk",
    .dev            = &device_desc,
    .strings        = dev_strings,
    .max_speed      = USB_SPEED_HIGH,
    .bind           = hid_bind,
    .unbind         = hid_unbind,
    .gadget_driver  = qmk_gadget_driver,
};

int hidg_plat_driver_probe(struct platform_device *pdev)
{
    struct qmk_platform_data *pdata = dev_get_platdata(&pdev->dev);
    struct hidg_func_node *entry;
    int status;
    // struct usb_composite_driver *driver = &hidg_driver;
    char *name;

    status = usb_gadget_probe_driver(&qmk_gadget_driver);
    if (status < 0) {
        dev_err(&pdev->dev, "usb gadget probe failed\n");
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
    usb_composite_unregister(&hidg_driver);
    struct hidg_func_node *e, *n;

    list_for_each_entry_safe(e, n, &hidg_func_list, node) {
        list_del(&e->node);
        kfree(e);
    }

    return 0;
}

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("Fabien Chouteau, Peter Korsgaard");
MODULE_LICENSE("GPL");