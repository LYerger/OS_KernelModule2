// Operating Systems (COP4600)
// Programming Assignment #2: Character Device Kernal Module
// Submission Date: 3/23/18
// Submission By: Brandon Cuevas, Jacquelyn Law, Lorraine Yerger
// File: moddymod.c
// Reference Used: http://derekmolloy.ie/writing-a-linux-kernel-module-part-2-a-character-device/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define BUFFER_SIZE 1024
#define DEVICE_NAME "moddymod"
#define CLASS_NAME "mod"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Brandon Jacquelyn Lorraine");
MODULE_DESCRIPTION("Programming Assignment #2: Character Device Kernal Module");
MODULE_VERSION("0.1");


static int majorNumber;
static char mainBuffer[BUFFER_SIZE]= {0};
static int bufferOccupation = 0;
static int bufferReadIndex = 0;
static int bufferWriteIndex = 0;
static struct class *modClass = NULL;
static struct device *modDevice = NULL;

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
 
static struct file_operations fops = {
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

/** @brief This function is called whenever the device is being written to from user space
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
	int bytesToReceive = len;
	int receiveIndex = 0;

	// While there is still room in the buffer and bytes to recieve
	while (bytesToReceive > 0 && bufferOccupation < BUFFER_SIZE)
	{
		// Put byte in main buffer at current write index
		sprintf(mainBuffer + bufferWriteIndex, "%c", buffer[receiveIndex++]);
		bufferOccupation++;
		bufferWriteIndex++;
		bytesToReceive--;

		if (bufferWriteIndex > BUFFER_SIZE - 1)
		{
			bufferWriteIndex = 0;
		}
	}

	printk(KERN_INFO "moddymod: Sent %d characters to the user\n", len - bytesToReceive);

	return len - bytesToReceive;
}

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
		printk(KERN_INFO "moddymod: Sent %d characters to the user\n", sendCount);
		return sendCount;
	}
	else
	{
		printk(KERN_INFO "moddymod: Failed to send %d characters to the user\n", sendCount);
		return -EFAULT;
	}
}

/** @brief The device open function that is called each time the device is opened
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep) {
   printk(KERN_INFO "moddymod: Device has been opened\n");
   return 0;
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep) {
   printk(KERN_INFO "moddymod: Device successfully closed\n");
   return 0;
}

int init_module(void) {
	printk(KERN_INFO "moddymod: Installing moddymod.\n");
	
	// Try to dynamically allocate a major number for the device
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	if (majorNumber < 0) {
		printk(KERN_ALERT "moddymod failed to register a major number \n");
		return majorNumber;
	}
	printk(KERN_INFO "moddymod: Registered correctly with major number %d\n", majorNumber);

	// Register the device class
   	modClass = class_create(THIS_MODULE, CLASS_NAME);
   	if (IS_ERR(modClass))
	{                
		// Check for error and clean up if there is
      		unregister_chrdev(majorNumber, DEVICE_NAME);
      		printk(KERN_ALERT "Failed to register device class\n");
      		return PTR_ERR(modClass);          // Return pointer error
   	}
  	printk(KERN_INFO "moddymod: Device class registered correctly\n");
 
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

   	printk(KERN_INFO "moddymod: Device class created correctly\n");

	return 0;
}

void cleanup_module(void) {
	printk(KERN_INFO "moddymod: Removing moddymod\n");

	device_destroy(modClass, MKDEV(majorNumber, 0));     // remove the device
	class_unregister(modClass);                          // unregister the device class
	class_destroy(modClass);                             // remove the device class
   	unregister_chrdev(majorNumber, DEVICE_NAME);         // unregister the major number
}
