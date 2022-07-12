/**
 * Very cheap little bitcoin HWW for use with lilygo TDisplay
 * although with little tinkering any ESP32 will work
 *
 * Join us!
 * https://t.me/lnbits
 * https://t.me/makerbits
 * 
 */

#include <FS.h>
#include <SPIFFS.h>

#include <Wire.h>
#include <TFT_eSPI.h>
#include <Hash.h>
#include <ArduinoJson.h>
#include "Bitcoin.h"
#include "PSBT.h"

#include <FS.h>
#include <SPIFFS.h>
fs::SPIFFSFS &FlashFS = SPIFFS;
#define FORMAT_ON_FAIL true

String serialData = "";
String oldSerialData = "";
String psbtStr = "";

bool waitBool = true;
bool psbtCollect = false;
bool passwordEntered = false;
bool fileCheck = true;

const int DELAY_MS = 5;

SHA256 h;
TFT_eSPI tft = TFT_eSPI();

String mnemonic = "";
String password = "";
String fetchedEncrytptedSeed = "";
String fetchedHash = "";
String keyHash = "";

PSBT psbt;

void setup() {
  Serial.begin(9600);

    // load screen
  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(true);
  logo();
  delay(3000);
  h.begin();
  FlashFS.begin(FORMAT_ON_FAIL);
  SPIFFS.begin(true);
}

void loop() {
  serialData = "";
  // check files
  fileCheck = true;
  readFile(SPIFFS, "/mn.txt", true);
  readFile(SPIFFS, "/hash.txt", false);
  if (fileCheck == false){
    message("Failed opening files", "Reset or 'wipe'");
    while(true){
      if(Serial.available() > 0){
        serialData = Serial.readStringUntil('\n'); 
        if(serialData == "wipe"){
          wipeHww();
          return false;
        }
      }
    }
  }

  // check password
  while(passwordEntered == false){
    message("Welcome", "Enter password");
    while(true){
      if(Serial.available() > 0){
        serialData = Serial.readStringUntil('\n'); 
        hashPass(serialData);
        if(keyHash == fetchedHash){
          passwordEntered == true;
          return false;
        }
      }
    }
  }
  while(true){
    message("Bwahaha", "Send PSBT, 'seed' or 'reset'");
    if(Serial.available() > 0){
      serialData = Serial.readStringUntil('\n'); 
      if(serialData.substring(0,3) == "cHNid"){
        psbtStr = serialData;
        parseSignPsbt();
      }
      if(serialData == "seed"){
        Serial.print();
      }
      if(serialData == "wipe"){
        wipeHww();
        return false;
      }
  }
  
  if(Serial.available() > 0){
    serialData = Serial.readStringUntil('\n');
    // Make sure we're not collecting same data
    if(serialData != oldSerialData){
      // Check if we need to start collecting psbt chunks
      if(serialData.substring(0,3) == "cHNid"){
        psbtStr = serialData;
        parseSignPsbt();
      }
    }  
  }
  oldSerialData = serialData;
  delay(DELAY_MS);
}

//========================================================================//
//================================HELPERS=================================//
//========================================================================//

void parseSignPsbt(){
  HDPrivateKey hd(mnemonic, password);
  psbt.parseBase64(psbtStr);
  // check parsing is ok
  if(!psbt){
    message("Failed parsing", "Send PSBT again");
    return;
  }
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(0, 30);
  tft.println("Transactions details:");
  // going through all outputs
  tft.println("Outputs:");
  for(int i=0; i<psbt.tx.outputsNumber; i++){
    // print addresses
    tft.print(psbt.tx.txOuts[i].address(&Testnet));
    if(psbt.txOutsMeta[i].derivationsLen > 0){ // there is derivation path
      // considering only single key for simplicity
      PSBTDerivation der = psbt.txOutsMeta[i].derivations[0];
      HDPublicKey pub = hd.derive(der.derivation, der.derivationLen).xpub();
      if(pub.address() == psbt.tx.txOuts[i].address()){
        tft.print(" (change) ");
      }
    }
    tft.print(" -> ");
    tft.print(psbt.tx.txOuts[i].btcAmount()*1e3);
    tft.println(" mBTC");
  }
  tft.print("Fee: ");
  tft.print(float(psbt.fee())/100); // Arduino can't print 64-bit ints
  tft.println(" bits");

  //wait for confirm
  bool waitToConfirm = true;
  message("Pass 'sign' to sign the psbt", "");
  while(waitToConfirm){
    if(Serial.available() > 0){
      // If we're here, then serial data has been received
      serialData = Serial.readStringUntil('\n'); 
    }
    if(serialData == "sign"){
      if(!hd){ // check if it is valid
        Serial.println("Invalid xpub");
        waitToConfirm = false;
        return;
      }
      else{
        psbt.sign(hd);
        Serial.println(psbt.toBase64()); // now you can combine and finalize PSBTs in Bitcoin Core
        waitToConfirm = false;
      }
    }
    delay(DELAY_MS);
  }
}

void hashPass(String key){
  byte hash[64] = { 0 }; 
  int hashLen = 0;
  hashLen = sha256(key, hash);
  keyHash = toHex(hash, hashLen);
}

void createMn(){
  // entropy bytes to mnemonic
  uint8_t arr[] = {'1','1','1','1','1','1','1','1','1','1','1','1','1','1','1','1'};
  String mn = mnemonicFromEntropy(arr, sizeof(arr));
  Serial.println(mn);
  uint8_t out[16];
  size_t len = mnemonicToEntropy(mn, out, sizeof(out));
  mnemonic = toHex(out, len);
}

void wipeHww(){
  waitBool = true;
  message("Resetting...", "");
  delay(2000);
  message("Enter password", "8 digits and alphanumeric");
    waitBool = true;
    while(waitBool){
      if(Serial.available() > 0){
        // If we're here, then serial data has been received
        serialData = Serial.readStringUntil('\n');
        if (isAlphaNumeric(serialData) == true){
          waitBool = false;
        }
      }
      delay(DELAY_MS);
    }
    deleteFile(SPIFFS, "/mn.txt");
    deleteFile(SPIFFS, "/hash.txt");
    createMn();
    hashPass(serialData);
    writeFile(SPIFFS, "/mn.txt", mnemonic);
    writeFile(SPIFFS, "/hash.txt", keyHash);
}

bool isAlphaNumeric(String instr){
      for(int i = 0; i < instr.length(); i++){
           if(instr.length() > 7){
               continue;
           }
           if(isalpha(instr[i])){
               continue;
           }
           if(isdigit(instr[i])){
               continue;
           }
           return false;
      }
      return true;
}

//========================================================================//
//===============================UI STUFF=================================//
//========================================================================//

void logo(){
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(4);
  tft.setCursor(0, 30);
  tft.print("BabyBowser");
  tft.setTextSize(2);
  tft.setCursor(0, 80);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("LNbits/ubitcoin wallet");
  Serial.println("Baby Bowser LNbits/ubitcoin HWW");
}

void message(String message, String additional)
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 30);
  tft.println(message);
  tft.setCursor(0, 80);
  tft.setTextSize(2);
  tft.println(additional);
  Serial.println(message);
  Serial.println(additional);
}

//========================================================================//
//=============================SPIFFS STUFF===============================//
//========================================================================//

void readFile(fs::FS &fs, const char * path, bool seed){

    Serial.printf("Reading file: %s\r\n", path);
    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        fileCheck = false;
        return;
    }
    while(file.available()){
      if(seed == false){
        fetchedHash = file.read();
      }
      else{
        fetchedEncrytptedSeed = file.read();
      }
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, String message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}
