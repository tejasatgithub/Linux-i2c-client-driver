/*
Linux i2c client driver implementation.
Current code is not specific to any chipset or vendor.
Code needs modification as per target client device
and is not expected to work as is for any chipset.
Below code provides a working and tested template to someone 
who would like to reuse this code and make changes to fit his/her needs.
*/
/*
1) Registers as i2c client driver
2) Probe routine is called by Linux subsystem when interested device Ids 
are enumerated.
3) sysfs interface - exposing something to user space, critical data
4) Kobject read write attributes, shown as files
5) 
*/
#include<linux/module.h> // included for all kernel modules
#include <linux/kernel.h> // included for KERN_INFO
#include <linux/init.h> // included for __init and __exit macros
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/list.h>

// Sample examples
//cpld slave address
#define  CPLD_SLAVE_ADD   0x3e
//cpld ver register
#define  CPLD_SLAVE_VER   0x00
//qsfp reset cntrl reg on each iom
#define  QSFP_RST_CRTL_REG0  0x10 
#define  QSFP_RST_CRTL_REG1  0x11 
//qsfp lp mode reg on each iom
#define  QSFP_LPMODE_REG0 0x12 
#define  QSFP_LPMODE_REG1 0x13
//qsfp mod presence reg on each iom
#define  QSFP_MOD_PRS_REG0 0x16 
#define  QSFP_MOD_PRS_REG1 0x17 

//TODO:: Enhancement -- support probing of multiple client devices
// and store them into list.

// Stores probed CPLD device data
struct cpld_data {
    struct i2c_client *client;
};


static void i2c_cpld_add_client(struct i2c_client *client)
{
    struct cpld_data *data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_dbg(&client->dev, "Can't allocate cpld_client_node (0x%x)\n", client->addr);
        return;
    }
    data->client = client;
    i2c_set_clientdata(client, data);
}

static void i2c_cpld_remove_client(struct i2c_client *client)
{
    struct cpld_data *data = i2c_get_clientdata(client);
    kfree(data);
    return;
}

static int i2c_cpld_probe(struct i2c_client *client,
        const struct i2c_device_id *dev_id)
{
    int status;

  	// Check for platform i2c device capabilities here
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_dbg(&client->dev, "i2c_check_functionality failed (0x%x)\n", client->addr);
        status = -EIO;
        goto exit;
    }

    dev_info(&client->dev, "chip probed - adding to client store\n");
	  // adding to list here
    i2c_cpld_add_client(client);

    /* TODO -- Register sysfs hooks */
    return 0;

exit:
    return status;
}

static int i2c_cpld_remove(struct i2c_client *client)
{
    i2c_cpld_remove_client(client);
    return 0;
}

static const struct i2c_device_id i2c_cpld_id[] = {
    { "plat_i2c_cpld", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, i2c_cpld_id);

static struct i2c_driver i2c_cpld_driver = {
    .driver = {
        .name     = "i2c_cpld",
    },
    .probe        = i2c_cpld_probe,
    .remove       = i2c_cpld_remove,
    .id_table     = i2c_cpld_id,
};

static int __init i2c_cpld_init(void)
{
    return i2c_add_driver(&i2c_cpld_driver);
}

static void __exit i2c_cpld_exit(void)
{
    i2c_del_driver(&i2c_cpld_driver);
}

MODULE_AUTHOR("Tejas Trivedi  <tejas.trivedi.er@gmail.com>");
MODULE_DESCRIPTION("i2c client driver");

module_init(i2c_cpld_init);
module_exit(i2c_cpld_exit);
