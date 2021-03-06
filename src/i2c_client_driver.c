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

// Support probing of multiple client devices
// and store them into list.
static LIST_HEAD(cpld_client_list);
static struct mutex  list_lock;

// Stores probed CPLD device data
struct cpld_data {
    struct i2c_client *client;
    struct list_head list;
};

static const unsigned short normal_i2c[] = { 0x3e, I2C_CLIENT_END };

static void i2c_cpld_add_client(struct i2c_client *client)
{
    struct cpld_data *data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);

    if (!data) {
        dev_dbg(&client->dev, "Can't allocate cpld_client_node (0x%x)\n", client->addr);
        return;
    }
    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_lock(&list_lock);
    list_add(&data->list,&cpld_client_list);
    mutex_unlock(&list_lock);
}

static void i2c_cpld_remove_client(struct i2c_client *client)
{
    struct list_head *list_node = NULL;
    struct cpld_data *data = NULL;
    int found = 0;

    mutex_lock(&list_lock);
    list_for_each(list_node, &cpld_client_list)
    {
        data = list_entry(list_node,struct cpld_data, list);
        if(data->client == client) {
            found = 1;
            break;
        }
    }
    if(found) {
        list_del(list_node);
        kfree(data);
    }
    mutex_unlock(&list_lock);
}

// Read and Write API for I2C access
int cpld_i2c_read(struct cpld_data *data, u8 reg)
{
    int ret = -EPERM;
    u8 high_reg =0x00;

    mutex_lock(&list_lock);
    ret = i2c_smbus_write_byte_data(data->client, high_reg,reg);
    ret = i2c_smbus_read_byte(data->client);
    mutex_unlock(&list_lock);

    return ret;
}

int cpld_i2c_write(struct cpld_data *data,u8 reg, u8 value)
{
    int ret = -EIO;
    u16 devdata=0;
    u8 high_reg =0x00;

    mutex_lock(&list_lock);
    devdata = (value << 8) | reg;
    i2c_smbus_write_word_data(data->client,high_reg,devdata);
    mutex_unlock(&list_lock);

    return ret;
}

static ssize_t get_lpmode(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    u16 devdata=0;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = cpld_i2c_read(data,QSFP_LPMODE_REG0);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata = (u16)ret & 0xff;

    ret = cpld_i2c_read(data,QSFP_LPMODE_REG1);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata |= (u16)(ret & 0xff) << 8;

    return sprintf(buf,"0x%04x\n",devdata);
}

static ssize_t get_reset(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    u16 devdata=0;
    struct cpld_data *data = dev_get_drvdata(dev);

    ret = cpld_i2c_read(data,QSFP_RST_CRTL_REG0);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata = (u16)ret & 0xff;

    ret = cpld_i2c_read(data,QSFP_RST_CRTL_REG1);
    if(ret < 0)
        return sprintf(buf, "read error");
    devdata |= (u16)(ret & 0xff) << 8;

    return sprintf(buf,"0x%04x\n",devdata);
}

// Device attributes defined
// User may read low power mode of the XCVR
// As well read reset state of the XCVR
static DEVICE_ATTR(qsfp_lpmode, S_IRUGO,get_lpmode,NULL);
static DEVICE_ATTR(qsfp_reset,  S_IRUGO,get_reset,NULL);

static struct attribute *i2c_cpld_attrs[] = {
    &dev_attr_qsfp_lpmode.attr,
    &dev_attr_qsfp_reset.attr,
    NULL,
};

static struct attribute_group i2c_cpld_attr_grp = {
    .attrs = i2c_cpld_attrs,
};


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

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &i2c_cpld_attr_grp);
    if (status) {
        printk(KERN_INFO "Cannot create sysfs\n");
    }

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
    .address_list = normal_i2c,
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
