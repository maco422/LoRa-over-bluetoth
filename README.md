# LoRa-over-bluetoth
Small project to send and recieve messages over LoRa and bluetooth to devices like smartphones and PC.


This code should work with LILYGO® TTGO T-Beam V1.1 device. Connection map looks like this:

<smartphone/pc> -bluetooth connection- <LILYGO® TTGO T-Beam V1.1> -LoRa connection- <LILYGO® TTGO T-Beam V1.1> -bluetooth connection- <smartphone/pc>

I did not create an app for sending messages, but you can use Bluetooth Serial Terminal app for Android devices and Windows.

Usable command are bellow

* --set_name:<YOUR NAME> (for device name change)
* --get_name (for getting current name)
* --get_code (for getting current Encryption code)
* --set_code:<YOUR CODE> (For set a new Encryption code (must be 15 ASCII chars))
* --set_public (set public configuration, to hear and send unencrypted messages).
  
  
To send an encrypted message between two devices both devices must be connected to own LILYGO® TTGO T-Beam V1.1 and Encryption code must be set same on both devices. 

Please change this line `#define BAND    923E6` to BAND allowed in your country.


![20230109_105305](https://user-images.githubusercontent.com/118419975/211231372-377fa800-e6f7-41ac-9e1c-42967153eae0.jpg)
