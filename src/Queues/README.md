# How to add custom queues.

NOTE: for a simple reference, look at the FollyProducerConsumer queue.

## Creating the folder.
First create a subfolder of the Queues folder. Give it the name you want to identify your queue.

## Creating the queue class
In the created folder add your queue class. This must be named according to the folder.
- "<foldername>Queue" 
This class must be derived from the Queue class in the daqling::core namespace.
Furthermore, it must implement the 'read','write','sizeGuess' and 'capacity' methods as a minimum, since these contain the specific logic for reading/writing to the queue. Setup of the queue can be done in the constructor.

## Creating the CMakeLists.txt
You must also provide a CMakeLists.txt file in the folder. This can be copied from the FollyProducerConsumer queue folder.
Remember to substitute the FollyProducerConsumerQueue source files with your own source files.

## Register the queue
In the source files you must use the REGISTER_QUEUE macro. 
This must be used in the following way:
'REGISTER_QUEUE(<class_name>,<name_of_folder>)'

## Further options provided by the queue interface

There are a couple of functions that can be overwritten from the Receiver and Sender classes. Below these are listed and explained:

* sleep_read/write - a blocking read/write with a timeout. Can be useful to call in modules to avoid throttling. By default this method calls read/write, and in case of a false return value, sleeps for the prescribed duration. Can be overwritten in case the custom queue supports timed blocking methods.
* set_sleep_duration - Set maximum sleeping duration of a sleep_send/receive call in milliseconds. Can be overwritten in case your custom queue needs specific configuration for this purpose. Otherwise the sleep_read/write can just use the 'm_sleep_duration'(sleep duration in milliseconds) attribute.