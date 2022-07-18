/**
   Very cheap little bitcoin HWW for use with lilygo TDisplay
   although with little tinkering any ESP32 will work

   Join us!
   https://t.me/lnbits
   https://t.me/makerbits

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
String restore = "";

bool waitBool = true;
bool psbtCollect = false;
bool passwordEntered = false;
bool fileCheck = true;
bool keepLooping = true;

const int DELAY_MS = 5;

SHA256 h;
TFT_eSPI tft = TFT_eSPI();

String mnemonic = "";
String password = "";
String fetchedEncrytptedSeed = "";
String fetchedHash = "";
String keyHash = "";

PSBT psbt;
const String COMMAND_RESTORE = "/restore";
const String COMMAND_WIPE = "/wipe";
const String COMMAND_PASSWORD = "/password";
const String COMMAND_SEED = "/seed";
const String COMMAND_SIGN_PSBT = "/sign";
const String COMMAND_HELP = "/help";


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

bool authenticated = false;
String command = "";
String commandData = "";

String message = "Welcome";
String subMessage = "Enter password";

void loop() {
  if (checkFiles() == false) {
    message = "Failed opening files";
    subMessage = "Reset or 'help'";
  }

  if (message != "" || subMessage != "")
    showMessage(message, subMessage);

  serialData = awaitSerialData();
  int spacePos = serialData.indexOf(" ");
  command = serialData.substring(0, spacePos);
  if (spacePos == -1) {
    commandData = "";
  } else {
    commandData = serialData.substring(spacePos + 1, serialData.length());
  }

  executeCommand(command, commandData);

  delay(DELAY_MS);
}


//========================================================================//
//================================COMMANDS================================//
//========================================================================//

void executeCommand(String command, String commandData) {
  if (command == COMMAND_HELP) {
    executeHelp(commandData);
  } else if (command == COMMAND_WIPE) {
    executeWipeHww(commandData);
  } else if (command == COMMAND_PASSWORD) {
    executePasswordCheck(commandData);
  } else if (command == COMMAND_SEED) {
    executeShowSeed(commandData);
  } else if (command == COMMAND_SIGN_PSBT) {
    executeSignPsbt(commandData);
  } else if (command == COMMAND_RESTORE) {
    executeRestore(commandData);
  } else {
    executeUnknown(commandData);
  }
}

void executeHelp(String commandData) {
  help();
}

void executePasswordCheck(String commandData) {
  if (commandData == "") {
    showMessage("Enter password", "8 numbers/letters");
    commandData = awaitSerialData();
  }
  String passwordHash = hashPassword(commandData);
  if (passwordHash == keyHash) {
    authenticated = true;
    message = "Password correct!";
    subMessage = "Ready to sign sir!";
  } else {
    authenticated = false;
    message = "Wrong password, try again";
    subMessage = "8 numbers/letters";
  }
}

void executeWipeHww(String commandData) {
  showMessage("Resetting...", "");
  delay(2000);

  if (commandData == "") {
    showMessage("Enter new password", "8 numbers/letters");
    commandData = awaitSerialData();
  }

  authenticated = wipeHww(commandData);
  if (authenticated == true) {
    message = "Successfully wiped!";
    subMessage = "Every new beginning comes from some other beginning's end.";
  } else {
    message = "Error, try again";
    subMessage = "8 numbers/letters";
  }
}

void executeShowSeed(String commandData) {
  if (authenticated == false) {
    message = "Enter password!";
    subMessage = "8 numbers/letters";
  } else {
    message = "";
    subMessage = "";
    printMnemonic(fetchedEncrytptedSeed);
  }
}

void executeRestore(String commandData) {
  if (commandData == "") {
    showMessage("Enter seed words", "Separated by spaces");
    commandData = awaitSerialData();
  }
  int size = getMnemonicBytes(commandData);
  if (size == 0) {
    message = "Wrong word count!";
    subMessage = "Must be 12, 15, 18, 21 or 24";
  } else {
    uint8_t out[size];
    size_t len = mnemonicToEntropy(commandData, out, sizeof(out));
    String mn = mnemonicFromEntropy(out, sizeof(out));
    deleteFile(SPIFFS, "/mn.txt");
    writeFile(SPIFFS, "/mn.txt", mn);
    printMnemonic(mn);

    message = "Restore successfull";
    subMessage = "Use `/seed` to view word list";
  }
}

void executeSignPsbt(String commandData) {
  if (authenticated == false) {
    message = "Enter password!";
    subMessage = "8 numbers/letters";
  } else {
    message = "PSBT";
    subMessage = commandData;
  }
}

void executeUnknown(String commandData) {
  message = "Unknonw command";
  subMessage = "Use `/help` for details";
}
//========================================================================//
//================================HELPERS=================================//
//========================================================================//

String awaitSerialData() {
  while (Serial.available() == 0) {
    // just loop and wait
  }
  return Serial.readStringUntil('\n');
}

bool checkFiles() {
  bool isFileOK = readFile(SPIFFS, "/mn.txt", true);
  if (isFileOK == false) return false;
  return readFile(SPIFFS, "/hash.txt", false);
}

int getMnemonicBytes(String menmonicSentence) {
  int wc = wordCount(menmonicSentence);
  switch (wc)
  {
    case 12:
      return 16;
    case 15:
      return 20;
    case 18:
      return 24;
    case 21:
      return 28;
    case 24:
      return 32;
    default:
      return 0;
  }
}

int wordCount(String s) {
  int count = 1;
  for (int i = 0; i < s.length(); i++)
    if (s[i] == ' ') count++;
  return count;
}

bool wipeHww(String password) {
  if (isAlphaNumeric(password) == false)
    return false;

  deleteFile(SPIFFS, "/mn.txt");
  deleteFile(SPIFFS, "/hash.txt");
  createMn();
  keyHash = hashPassword(password); // todo: rename var
  Serial.println("wipeHww mnemonic: " + mnemonic);
  Serial.println("wipeHww keyHash: " + keyHash);
  writeFile(SPIFFS, "/mn.txt", mnemonic);
  writeFile(SPIFFS, "/hash.txt", keyHash);

  delay(DELAY_MS);
  return true;
}



void parseSignPsbt() {
  HDPrivateKey hd(mnemonic, password);
  psbt.parseBase64(psbtStr);
  // check parsing is ok
  if (!psbt) {
    showMessage("Failed parsing", "Send PSBT again");
    return;
  }
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(0, 30);
  tft.println("Transactions details:");
  // going through all outputs
  tft.println("Outputs:");
  for (int i = 0; i < psbt.tx.outputsNumber; i++) {
    // print addresses
    tft.print(psbt.tx.txOuts[i].address(&Testnet));
    if (psbt.txOutsMeta[i].derivationsLen > 0) { // there is derivation path
      // considering only single key for simplicity
      PSBTDerivation der = psbt.txOutsMeta[i].derivations[0];
      HDPublicKey pub = hd.derive(der.derivation, der.derivationLen).xpub();
      if (pub.address() == psbt.tx.txOuts[i].address()) {
        tft.print(" (change) ");
      }
    }
    tft.print(" -> ");
    tft.print(psbt.tx.txOuts[i].btcAmount() * 1e3);
    tft.println(" mBTC");
  }
  tft.print("Fee: ");
  tft.print(float(psbt.fee()) / 100); // Arduino can't print 64-bit ints
  tft.println(" bits");

  //wait for confirm
  bool waitToConfirm = true;
  showMessage("Pass 'sign' to sign the psbt", "");
  while (waitToConfirm) {
    if (Serial.available() > 0) {
      // If we're here, then serial data has been received
      serialData = Serial.readStringUntil('\n');
    }
    if (serialData == "sign") {
      if (!hd) { // check if it is valid
        Serial.println("Invalid xpub");
        waitToConfirm = false;
        return;
      }
      else {
        psbt.sign(hd);
        Serial.println(psbt.toBase64()); // now you can combine and finalize PSBTs in Bitcoin Core
        waitToConfirm = false;
      }
    }
    delay(DELAY_MS);
  }
}

String hashPassword(String key) {
  byte hash[64] = { 0 };
  int hashLen = 0;
  hashLen = sha256(key, hash);
  return toHex(hash, hashLen);
}

void createMn() {
  if (restore != "") {
    // be nice to add a check here
    mnemonic = restore;
  }
  else {
    // entropy bytes to mnemonic
    uint8_t arr[] = {'1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1'};
    String mn = mnemonicFromEntropy(arr, sizeof(arr));
    Serial.println("mnemonicFromEntropy: " + mn);
    uint8_t out[16];
    size_t len = mnemonicToEntropy(mn, out, sizeof(out));
    mnemonic = toHex(out, len);
  }
}


String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  const int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

bool isAlphaNumeric(String instr) {
  if (instr.length() < 8) {
    return false;
  }
  for (int i = 0; i < instr.length(); i++) {
    if (isalpha(instr[i])) {
      continue;
    }
    if (isdigit(instr[i])) {
      continue;
    }
    return false;
  }
  return true;
}

//========================================================================//
//===============================UI STUFF=================================//
//========================================================================//

void logo() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(4);
  tft.setCursor(0, 30);
  tft.print("BabyBowser");
  tft.setTextSize(2);
  tft.setCursor(0, 80);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("LNbits/ubitcoin HWW");
  Serial.println("BabyBowser LNbits/ubitcoin HWW");
}

void showMessage(String message, String additional)
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

void help()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.println("BabyBowser Commands");
  tft.setCursor(0, 20);
  tft.setTextSize(1);
  tft.println("'wipe' will completely wipe device and creat a new wallet");
  tft.println("'restore <BIP39 seed words seperated by space>' will restore from seed");
  tft.println("'seed' will send seed to serial output");
  tft.println("'cHNid...' Will parse valid psbt");
  tft.println("'sign' Will sign valid psbt");
  Serial.println("BabyBowser Commands");
  Serial.println("'wipe' will completely wipe device and creat a new wallet");
  Serial.println("'restore <BIP39 seed words seperated by space>' will restore from seed");
  Serial.println("'seed' will send seed to serial output");
  Serial.println("'cHNid...' Will parse valid psbt");
  Serial.println("'sign' Will sign valid psbt");
  delay(10000);
}

void printMnemonic(String mn) {
  Serial.println("printMnemonic mn: " + mn);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.println("Word list:");
  tft.setCursor(0, 20);
  tft.setTextSize(1);
  tft.println(mn);
}

//========================================================================//
//=============================SPIFFS STUFF===============================//
//========================================================================//

bool readFile(fs::FS &fs, const char * path, bool seed) {
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return false;
  }

  if (seed == false) {
    fetchedHash = "";
  }
  else {
    fetchedEncrytptedSeed = "";
  }
  while (file.available()) {
    if (seed == false) {
      fetchedHash += (char)file.read();
    }
    else {
      fetchedEncrytptedSeed += (char)file.read();
    }
  }
  file.close();
  return true;
}

void writeFile(fs::FS &fs, const char * path, String message) {
  Serial.printf("Writing file: %s\r\n", path);
  Serial.println("writeFile message: " + message);
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void deleteFile(fs::FS &fs, const char * path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}
