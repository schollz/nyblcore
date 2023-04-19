# nyblcore

![image](https://user-images.githubusercontent.com/6550035/233191162-b6a9d175-3314-4aa9-abc1-d86e2248c9ee.png)


nyblcore is...

- a tiny **ATtiny85-based sample player** device for diy enthusiasts.
- powered by a single AAA for **~5-8 hours of battery life**.
- holding 8 kB of storage for up to **1.2 seconds of 8-bit audio sampled @ 4.3 kHz**.
- controlled with three multi-functional knobs that modulate tempo, volume, distortion, and fx.
- manipulated with **three fx knobs that "control" jump, retrig, stutter and stretch**.
- able to **load custom firmware / audio** using an [Arduino](https://www.amazon.com/Arduino-A000066-ARDUINO-UNO-R3/dp/B008GRTSV6/?tag=scholl-20&th=1) or an [AVR usb programmer](https://www.amazon.com/whiteeeen-Tiny-AVR-Programmer/dp/B09921SC7Z/?tag=scholl-20&th=1).
- entirely open-source, wonderfully hackable.

nyblcore is available for purchase. The assembled version is ready to play, handmade by me. The kit version comes with all the components (battery included!) and instructions on how to solder it together. more information at my website: [infinitedigits.co/nyblcore](https://infinitedigits.co/nyblcore).

## schematic

![schematic](https://user-images.githubusercontent.com/6550035/233191046-8b34f5eb-3aad-4488-ab2e-3506ba86dc4e.png)


## Uploading firmware

See below for the tools nessecary to upload the firmware (you will need a USB ATtiny85 programmer or an Arduino) and you will need the Arduino IDE. Skip to that section if you don't have .

To upload firmware, goto [infinitedigits.com/nyblcore#upload](https://infinitedigits.com/nyblcore#upload) and upload the audio you want. After uploading, you should receive a text box with a new firmware. 

![Uploaded audio](https://user-images.githubusercontent.com/6550035/233196926-6b05b1a4-4ea0-4eb0-8786-a3b743f84ae5.png)


You can copy and paste this directly into the Arduino IDE. 

![Arduino IDE](https://user-images.githubusercontent.com/6550035/233197400-03ea760a-eabb-4eca-8b45-8541af3ff5dc.png)

Then take out the ATtiny85 from the nyblcore board and put it in your programmer and plug your programmer into your computer. Now just upload the new firmware and voila! You can take out the programmer and put the newly loaded ATtiny85 back into the nyblcore board to enjoy the new sounds.


## Uploading firmware tools

Changing the audio on the ATtiny85 requires uploading new firmware. Uploading new firmware is easy but it requires  [a USB ATtiny85 programmer](https://www.amazon.com/Programmer-Tools-ATTiny85-ATTiny-Arduino/dp/B0BK9P76BH/). (You can also use an Arduino if you have one handy, I'll mention instructions for that below). 

![programmer](https://cdn.sparkfun.com/r/600-600/assets/1/5/f/9/9/527132e1757b7f632a8b4567.png)

Windows users: you will also need drivers for the USB ATtiny85 programmer which can be found [here](https://github.com/adafruit/Adafruit_Windows_Drivers/releases/tag/2.5.0.0).


To start, make sure you [install the Arduino IDE](https://www.arduino.cc/en/software). Open the software and goto `File -> Preferences` and where it says `Additional Boards Manager URLs` you can click a button to add an additional line. Add the following line:


```
https://raw.githubusercontent.com/damellis/attiny/ide-1.6.x-boards-manager/package_damellis_attiny_index.json
```

![Adding board](
https://user-images.githubusercontent.com/6550035/233195008-9a765f9c-c96e-4c49-9728-c7ebb7b6dbb1.png
))

Now restart the Arduino IDE. Now you can goto `Tools -> Board: -> ATtiny Microcontrollers` and select `ATtiny25/45/85`.

![Attiny selection](https://user-images.githubusercontent.com/6550035/233195515-e8d1367b-203c-4c53-bcbc-61b3ae34015a.png)




Before you begin you will need one of the following pieces of hardware. You can either purchase a USB ATtiny85 programmer (like [this one](https://www.amazon.com/Programmer-Tools-ATTiny85-ATTiny-Arduino/dp/B0BK9P76BH/)) OR

To upload new firmware, first make sure that you [install the Arduino IDE](https://www.arduino.cc/en/software). Any version will work.

Next, you will need to select `Tools -> Clock -> Internal 16 Mhz`.

![Clock setting](https://user-images.githubusercontent.com/6550035/233195852-3319e177-6096-41b0-a7ab-ef0f34e5fdba.png)

Finally, you will need to select `Tools -> Programmer -> USBtinyISP`.

![USBtinyISP](https://user-images.githubusercontent.com/6550035/233196149-7f6d4c16-921b-4cee-9243-9a2a6342318a.png)
