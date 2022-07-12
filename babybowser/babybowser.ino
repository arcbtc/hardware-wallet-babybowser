/**
 * Very cheap little bitcoin HWW for use with lilygo TDisplay
 * although with little tinkering any ESP32 will work
 *
 * https://t.me/lnbits
 * https://t.me/makerbits
 * 
 */

// #include <SPI.h> // Comment out when using i2c

#include <FS.h>
#include <SPIFFS.h>

#include <Wire.h>
#include <TFT_eSPI.h>
#include <Hash.h>
#include <ArduinoJson.h>
#include "Bitcoin.h"


String rcvdSerialData = "";
String masterPubKey = "";
bool masterPubKeyCollect = false;
bool passwordEntered = false;
bool fileCheck = true;

const int DELAY_MS = 5;

SHA256 h;
TFT_eSPI tft = TFT_eSPI();

String mnemonic = "";
String password = "";
String fetchedEncrytptedSeed = "";
String fetchedHash = "";

PSBT psbt;

void setup() {
  Serial.begin(9600);

    // load screen
  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(true);
  logo();

  h.begin();
  FlashFS.begin(FORMAT_ON_FAIL);
  SPIFFS.begin(true);
}

void loop() {
  fileCheck = true;
  readFile(SPIFFS, "/mn.txt", true);
  readFile(SPIFFS, "/hash.txt");
  if (fileCheck == false){
    error("Failed opening files", "Try again or send 'reset'");
  }
  rcvdSerialData = "";
  // Check to see if there is any incoming serial data
  if(Serial.available() > 0){
    // If we're here, then serial data has been received
    rcvdSerialData = Serial.readStringUntil('\n'); 
  }
  if(rcvdSerialData == "reset"){
    resetHww();
  }
  // Make sure we're not collecting same data
  if(rcvdSerialData != oldRcvdSerialData){
    if(passwordEntered == false){
      hashPass(rcvdSerialData)
      if(keyHash == fetchedHash){
        passwordEntered == true;
      }
    }
    else{
      // Check if we are currently collecting psbt chunks
      if(masterPsbtCollect == true){
        masterPubKey = masterPubKey + rcvdSerialData;
      }
      // Check if we need to start collecting psbt
      if(rcvdSerialData.substring(0,3) == "cHNid"){
        masterPubKey = rcvdSerialData;
        masterPsbtCollect = true;
      }
      if(rcvdSerialData == "clear" && masterPubKeyCollect == true){
        // Check, decode, sign, and send back psbt
        parseSignPsbt();
        masterPubKeyCollect = false;
      }
      // Clear psbt
      if(rcvdSerialData == "clear" && masterPubKeyCollect == false){
        masterPubKey = "";
      }
    }
    
  }
  oldRcvdSerialData = rcvdSerialData;
  delay(DELAY_MS);
}

//========================================================================//
//================================HELPERS=================================//
//========================================================================//

void parseSignPsbt(){
  psbt.parseBase64(masterPubKey);
  // check parsing is ok
  if(!psbt){
    error("Failed parsing transaction", "Try sending again");
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
  serial.println("Write 'sign' to sign the psbt");
  while(waitToConfirm){
    if(Serial.available() > 0){
      // If we're here, then serial data has been received
      rcvdSerialData = Serial.readStringUntil('\n'); 
    }
    if(rcvdSerialData == "sign"){
      HDPrivateKey hd(mnemonic, password);
      if(!hd){ // check if it is valid
        Serial.println("Invalid xpub");
        waitToConfirm = false;
        return;
      }
      else{
        psbt.sign(hd);
        serial.println(psbt.toBase64()); // now you can combine and finalize PSBTs in Bitcoin Core
        waitToConfirm = false;
      }
    }
    delay(DELAY_MS);
  }
}

void hashPass(String key){
  uint8_t hmacresult[32];
  SHA256 h;
  h.beginHMAC(key, keylen);
  h.write((uint8_t *) "qwertyuiopasd", 13);
  h.endHMAC(hmacresult);
  keyHash = hmacresult;
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

void resetHww(){
  error("Resetting...", "");
  delay(2000);
  error("Enter password", "Must be 8 digits and alphanumeric");
    waitBool = true;
    while(waitBool){
      if(Serial.available() > 0){
        // If we're here, then serial data has been received
        rcvdSerialData = Serial.readStringUntil('\n');
        if (isAlphaNumeric(rcvdSerialData) == true){
          waitBool = false;
        }
      }
      delay(DELAY_MS);
    }
    deleteFile(SPIFFS, "/mn.txt");
    deleteFile(SPIFFS, "/hash.txt");
    createMn();
    hashPass(rcvdSerialData);
    writeFile(SPIFFS, "/mn.txt", mnemonic);
    writeFile(SPIFFS, "/hash.txt", keyHash);
}

bool isAlphaNumeric(String instr){
      for(int i = 0; i < strlen(instr); i++){
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
  serial.println("Baby Bowser LNbits/ubitcoin wallet");
}

void error(String message, String additional)
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(0, 30);
  tft.println(message);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(0, 120);
  tft.setTextSize(2);
  tft.println(additional);
  serial.println(message);
  serial.println(additional);
}

//========================================================================//
//=============================SPIFFS STUFF===============================//
//========================================================================//

void readFile(fs::FS &fs, const char * path, bool seed = false){

    Serial.printf("Reading file: %s\r\n", path);
    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        fileCheck = false;
        return;
    }
    while(file.available()){
      if(seed == true){
        fetchedEncrytptedSeed = file.read();
      }
      else{
        fetchedHash = file.read();
      }
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
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
