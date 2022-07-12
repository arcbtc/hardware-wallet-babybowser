![image](https://user-images.githubusercontent.com/33088785/178449323-e2bd30e2-9c2a-4704-90ab-79d55a4187e6.png)
# BabyBowser 
## ESP32 Bitcoin Hardware Wallet (powered by uBitcoin)

This very cheap off the shelf hardware wallet is designed to work with Lilygos Tdisplay, but you can easily make work with any ESP32.


The Lilygo <a href="https://www.aliexpress.com/item/33048962331.html">Tdisplay</a> is a cheap amd small ESP32/screen developement board. You can either attach a <a href="https://www.aliexpress.com/item/32993999306.html">keypad membrane</a>, or use the <a href="https://www.aliexpress.com/item/1005003589706292.html">breakout board</a> Lilygo specifically made for the LNURLPoS!

- Install <a href="https://www.arduino.cc/en/software">Arduino IDE 1.8.19</a>
- Install ESP32 boards, using <a href="https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html#installing-using-boards-manager">boards manager</a>
- Role back ESP32 boards to 2.0.1 in boards manager
![image](https://user-images.githubusercontent.com/33088785/161862832-1269a12e-16ce-427c-9a92-df3ee573a1fb.png)

- Download this repo
- Copy these <a href="libraries">libraries</a> into your Arduino install "libraries" folder
- Open this <a href="LNPoSTdisplay.ino">LNPoSTdisplay.ino</a> file in the Arduino IDE
- Select "TTGO-LoRa32-OLED-V1" from tools>board
- Upload to device

> Press/hold any button on startup to launch portal.
> Default password is "ToTheMoon1"

> _Note: If using MacOS, you will need the CP210x USB to UART Bridge VCP Drivers available here https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers_
> If you are using **MacOS Big Sur or an Mac with M1 chip**, you might encounter the issue `A fatal error occurred: Failed to write to target RAM (result was 0107)`, this is related to the chipsest used by TTGO, you can find the correct driver and more info in this <a href="https://github.com/Xinyuan-LilyGO/LilyGo-T-Call-SIM800/issues/139#issuecomment-904390716">GitHub issue</a>
