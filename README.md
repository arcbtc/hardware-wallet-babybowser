![image](https://user-images.githubusercontent.com/33088785/178449323-e2bd30e2-9c2a-4704-90ab-79d55a4187e6.png)
# BabyBowser 
## ESP32 Bitcoin Hardware Wallet (powered by uBitcoin)

This very cheap off the shelf hardware wallet is designed to work with Lilygos <a href="https://www.aliexpress.com/item/33048962331.html">Tdisplay</a>, but you can easily make work with any ESP32.

Data is sent to/from BabyBowser over webdev Serial, not the most secure data transmission method, but fine for handling small-medium sized amounts of funds. You can use LNbits OnchainWallet extension, or any other serial monitor.

## Build instructions

- Buy a Lilygo <a href="https://www.aliexpress.com/item/33048962331.html">Tdisplay</a> (although with a little tinkering any ESP32 will do) 
- Install <a href="https://www.arduino.cc/en/software">Arduino IDE 1.8.19</a>
- Install ESP32 boards, using <a href="https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html#installing-using-boards-manager">boards manager</a>
- Download this repo
- Copy these <a href="libraries">libraries</a> into your Arduino install "libraries" folder
- Open this <a href="babybowser.ino">babybowser.ino</a> file in the Arduino IDE
- Select "TTGO-LoRa32-OLED-V1" from tools>board
- Upload to device

## How to use
// Guide to go here

> _Note: If using MacOS, you will need the CP210x USB to UART Bridge VCP Drivers available here https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers_
> If you are using **MacOS Big Sur or an Mac with M1 chip**, you might encounter the issue `A fatal error occurred: Failed to write to target RAM (result was 0107)`, this is related to the chipsest used by TTGO, you can find the correct driver and more info in this <a href="https://github.com/Xinyuan-LilyGO/LilyGo-T-Call-SIM800/issues/139#issuecomment-904390716">GitHub issue</a>
