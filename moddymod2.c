// Contains the Read module, moddymod.c contains Write

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define BUFFER_SIZE 1024
#define DEVICE_NAME "moddymod2"
#define CLASS_NAME "mod2"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brandon Jacquelyn Lorraine");
MODULE_DESCRIPTION("Programming Assignment #3: 2 Character Device Kernal Modules");
MODULE_VERSION("0.1");

static int majorNumber;

// Get mainBuffer & bufferOccupation from shared memory
extern char mainBuffer[BUFFER_SIZE];
extern int bufferOccupation;
extern struct mutex moddymod_mutex;

static int bufferReadIndex = 0;
static struct class *modClass = NULL;
static struct device *modDevice = NULL;

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);

static struct file_operations fops = {
	.open = dev_open,
	.read = dev_read,
	.release = dev_release,
};


/** @brief This function is called whenever the device is being read from user space
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
	int errorCount = 0;
	int sendCount = 0;

	// While there are things to send and the requested character count has not been met
	while (sendCount < len && bufferOccupation > 0)
	{
		// Keep track of errors and send things to the user
		errorCount += copy_to_user(buffer + sendCount, mainBuffer + bufferReadIndex, 1);
		bufferOccupation--;
		bufferReadIndex++;
		sendCount++;
		if (bufferReadIndex > BUFFER_SIZE - 1)
		{
			bufferReadIndex = 0;
		}
	}
	
	// If no errors then return the number of characters sent
	// otherwise, return a bad address message
	if (errorCount == 0)
	{
		printk(KERN_INFO "moddymod2: Sent %d characters to the user\n", sendCount);
		mutex_unlock(&moddymod_mutex);
		return sendCount;
	}
	else
	{
		printk(KERN_INFO "moddymod2: Failed to send %d characters to the user\n", sendCount);
		return -EFAULT;
	}
	
}

/** @brief The device open function that is called each time the device is opened
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep) {
   printk(KERN_INFO "moddymod2: Device has been opened\n");
   if(!mutex_trylock(&moddymod_mutex))
   {
	printk(KERN_ALERT "moddymod2: Device is in use by another process\n");
	return -EBUSY;
   }
   return 0;
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep) {
   printk(KERN_INFO "moddymod2: Device successfully closed\n");
   
   return 0;
}

int init_module(void) {
	printk(KERN_INFO "moddymod2: Installing moddymod2.\n");

	// Try to dynamically allocate a major number for the device
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	if (majorNumber < 0) {
		printk(KERN_ALERT "moddymod2 failed to register a major number \n");
		return majorNumber;
	}
	printk(KERN_INFO "moddymod2: Registered correctly with major number %d\n", majorNumber);

	// Register the device class
   	modClass = class_create(THIS_MODULE, CLASS_NAME);
   	if (IS_ERR(modClass))
	{
		// Check for error and clean up if there is
      		unregister_chrdev(majorNumber, DEVICE_NAME);
      		printk(KERN_ALERT "Failed to register device class\n");
      		return PTR_ERR(modClass);          // Return pointer error
   	}
  	printk(KERN_INFO "moddymod2: Device class registered correctly\n");

   	// Register the device driver
   	modDevice = device_create(modClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	if (IS_ERR(modDevice))
	{
		// Clean up if there is an error
		class_destroy(modClass);
		// Repeated code but the alternative is goto statements
      		unregister_chrdev(majorNumber, DEVICE_NAME);
      		printk(KERN_ALERT "Failed to create the device\n");

		return PTR_ERR(modDevice);
	}

   	printk(KERN_INFO "moddymod2: Device class created correctly\n");

	return 0;
}

void cleanup_module(void) {
	printk(KERN_INFO "moddymod2: Removing moddymod2\n");

	device_destroy(modClass, MKDEV(majorNumber, 0));     // remove the device
	class_unregister(modClass);                          // unregister the device class
	class_destroy(modClass);                             // remove the device class
   	unregister_chrdev(majorNumber, DEVICE_NAME);         // unregister the major number
}
