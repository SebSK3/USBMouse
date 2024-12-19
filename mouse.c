#include <linux/errno.h>
#include <linux/hid.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/usb.h>

MODULE_AUTHOR("Sebastian Kwaśniak");
MODULE_AUTHOR("Anna Berkowska");
MODULE_LICENSE("GPL");

static struct usb_device_id mouse_table[] = {
    { USB_DEVICE(0x1038, 0x1702) },
    {} // Terminating entry
};

MODULE_DEVICE_TABLE(usb, mouse_table);

struct usb_mouse {
    struct usb_device *usb_dev;
    struct input_dev *input_dev;
    struct urb *urb;
    signed char *data_buf;

    char phys[64];
};

static void mouse_irq(struct urb *urb) {
    struct usb_mouse *mouse_context = urb->context;
    signed char *data_buf = mouse_context->data_buf;

    printk("Mouse IRQ status=%d.\n", urb->status);
    printk("%d %d %d %d %d %d %d %d", data_buf[0], data_buf[1], data_buf[2], data_buf[3], data_buf[4], data_buf[5], data_buf[6], data_buf[7]);

    // Handle buttons
    input_report_key(mouse_context->input_dev, BTN_LEFT, data_buf[0] & 1);
    input_report_key(mouse_context->input_dev, BTN_RIGHT, data_buf[0] & 2);
    input_report_key(mouse_context->input_dev, BTN_MIDDLE, data_buf[0] & 4);
    input_report_key(mouse_context->input_dev, BTN_SIDE, data_buf[0] & 8);

    // Handle movement
    input_report_rel(mouse_context->input_dev, REL_X, data_buf[1]);
    input_report_rel(mouse_context->input_dev, REL_Y, data_buf[3]);
    // Handle scroll
    input_report_rel(mouse_context->input_dev, REL_WHEEL, data_buf[5]);

    input_sync(mouse_context->input_dev);

    // Submit urb
    usb_submit_urb(mouse_context->urb, GFP_ATOMIC);
}

static int mouse_prob(struct usb_interface *intf, const struct usb_device_id *id) {
    struct usb_interface_descriptor *intf_desc = &intf->cur_altsetting->desc;
    struct usb_device *udev = interface_to_usbdev(intf);
    struct usb_host_endpoint *endpoint;
    struct usb_mouse *mouse;

    // This interface supports the mouse protocol
    printk("Supported USB Mouse detected on probe.\n");

    // Get the endpoint
    endpoint = &intf->cur_altsetting->endpoint[0];

    // Allocate the mouse struct to be passed as context
    mouse = kzalloc(sizeof(struct usb_mouse), GFP_KERNEL);
    mouse->data_buf = kzalloc(8, GFP_KERNEL);
    mouse->usb_dev = udev;

    // Allocate input dev
    mouse->input_dev = input_allocate_device();
    mouse->input_dev->name = "Mouse";

    usb_make_path(mouse->usb_dev, mouse->phys, sizeof(mouse->phys));

    mouse->input_dev->dev.parent = &intf->dev;
    mouse->input_dev->phys = mouse->phys;
    mouse->input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
    mouse->input_dev->keybit[BIT_WORD(BTN_MOUSE)] = BIT_MASK(BTN_LEFT) | BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_MIDDLE) | BIT_MASK(BTN_SIDE);
    mouse->input_dev->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y) | BIT_MASK(REL_WHEEL);

    input_register_device(mouse->input_dev);

    input_set_drvdata(mouse->input_dev, mouse);

    // Establish urb for mouse
    mouse->urb = usb_alloc_urb(0, GFP_KERNEL); // 0 for interrupt
    unsigned int pipe = usb_rcvintpipe(mouse->usb_dev, endpoint->desc.bEndpointAddress);

    // Init the urb
    usb_fill_int_urb(mouse->urb,
        mouse->usb_dev,
        pipe,
        mouse->data_buf,
        8,
        mouse_irq,
        mouse,
        endpoint->desc.bInterval);
    usb_set_intfdata(intf, mouse);

	usb_submit_urb(mouse->urb, GFP_KERNEL);

    return 0;
}

static void mouse_disconnect(struct usb_interface *intf) {
    struct usb_mouse *mouse = usb_get_intfdata(intf);
    printk("Mouse disconnected.\n");
    usb_set_intfdata(intf, NULL);
    if (mouse) {
        usb_kill_urb(mouse->urb);
        input_unregister_device(mouse->input_dev);
        usb_free_urb(mouse->urb);
        kfree(mouse);
    }
}

static struct usb_driver mouse_driver = {
    .name = "mouse",
    .probe = mouse_prob,
    .disconnect = mouse_disconnect,
    .id_table = mouse_table,
};

int init_module(void) {
    int result;
    pr_info("Mouse driver initiating.\n");

    // Register driver against the USB subsystem
    result = usb_register(&mouse_driver);

    return 0;
}

void cleanup_module(void) {
    pr_info("Mouse driver clean up\n");
    usb_deregister(&mouse_driver);
}
