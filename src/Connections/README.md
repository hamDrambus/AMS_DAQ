# How to add custom connections.

NOTE: for a simple reference, look at the Dummy connection.

## Creating the folder.
First create a subfolder of the Connections folder. Give it the name you want to identify your connection.

## Creating the connection class
In the created folder add your sender and/or receiver class. These must be named according to the folder.
e.g. "<foldername>Recevier" and "<foldername>Sender"
These classes must be derived from the Sender/Receiver class in the daqling::core namespace.
Furthermore, they must implement the 'Send' or 'Receive' method as a minimum, since these contain the specific logic for sending/receiving. Setup of the connection can be done in the constructor.

## Creating the CMakeLists.txt
You must also provide a CMakeLists.txt file in the folder. This can be copied from the Dummy connection folder.
Remember to substitute the Dummy source files with your own source files.

## Register the connection
In the source files you must use the REGISTER_RECEIVER or REGISTER_SENDER macros. 
These must be used in the following way:
'REGISTER_RECEIVER(<class_name>,<name_of_folder>)

## Further options provided by the connection interface

There are a couple of functions that can be overwritten from the Receiver and Sender classes. Below these are listed and explained:
* start - will be called when the daqling 'start' command is invoked. Can be used to perform setup, that should be torn down by the 'stop' command.
* stop - will be called when the daqling 'stop' command is invoked. Used to perform teardown, that is required for a temporary pause of the connection.
* sleep_send/receive - a blocking send/receive with a timeout. Can be useful to call in modules to avoid throttling. By default this method calls send/receive, and in case of a false return value, sleeps for the prescribed duration. Can be overwritten in case the custom connection supports timed blocking methods.
* set_sleep_duration - Set maximum sleeping duration of a sleep_send/receive call in milliseconds. Can be overwritten in case your custom connection needs specific configuration for this purpose. Otherwise the sleep_send/receive can just use the 'm_sleep_duration'(sleep duration in milliseconds) attribute.