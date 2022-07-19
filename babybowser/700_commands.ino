//========================================================================//
//================================COMMANDS================================//
//========================================================================//

String encrytptedMnemonic = "";
String passwordHash = "";

String keyHash = ""; // todo: revisit

bool authenticated = false;
String command = "";
String commandData = "";

String message = "Welcome";
String subMessage = "Enter password";

String serialData = "";

void listenForCommands() {
  if (loadFiles() == false) {
    message = "Failed opening files";
    subMessage = "Reset or 'help'";
  }

  if (message != "" || subMessage != "")
    showMessage(message, subMessage);

  serialData = awaitSerialData();


  Command c = extractCommand(serialData);
  executeCommand(c);

  delay(DELAY_MS);
}


void executeCommand(Command c) {
  if (c.cmd == COMMAND_HELP) {
    executeHelp(c.data);
  } else if (c.cmd == COMMAND_WIPE) {
    executeWipeHww(c.data);
  } else if (c.cmd == COMMAND_PASSWORD) {
    executePasswordCheck(c.data);
  } else if (c.cmd == COMMAND_SEED) {
    executeShowSeed(c.data);
  } else if (c.cmd == COMMAND_SEND_PSBT) {
    executeSignPsbt(c.data);
  } else if (c.cmd == COMMAND_RESTORE) {
    executeRestore(c.data);
  } else {
    executeUnknown(c.data);
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
    return;
  }
  message = "";
  subMessage = "";
  printMnemonic(encrytptedMnemonic);

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
    return;
  }
  uint8_t out[size];
  size_t len = mnemonicToEntropy(commandData, out, sizeof(out));
  String mn = mnemonicFromEntropy(out, sizeof(out));
  deleteFile(SPIFFS, "/mn.txt");
  writeFile(SPIFFS, "/mn.txt", mn);
  printMnemonic(mn);

  message = "Restore successfull";
  subMessage = "Use `/seed` to view word list";

}

void executeSignPsbt(String commandData) {
  if (authenticated == false) {
    message = "Enter password!";
    subMessage = "8 numbers/letters";
    return;
  }

  PSBT psbt = parseBase64Psbt(commandData);
  if (!psbt) {
    message = "Failed parsing";
    subMessage = "Send PSBT again";
    return;
  }

  HDPrivateKey hd(encrytptedMnemonic, ""); // todo: no passphrase yet
  if (!hd) { // check if it is valid
    message = "Invalid XXXXXXXX";
    return;
  }

  HDPrivateKey hd44 = hd.derive("m/44'/0'/0'"); // todo: 49', 84', 86'
  Serial.println("### hd44.xpub()" + hd44.xpub().xpub());
  

  printPsbtDetails(psbt, hd44);

  commandData = awaitSerialData();
  if (commandData == COMMAND_SIGN_PSBT) {
    int signedInputCount = psbt.sign(hd44);
    Serial.println(psbt.toBase64());
    message = "Signed inputs:";
    subMessage = String(signedInputCount);
  } else {
    message = "Welcome XXX";
  }

}

void executeUnknown(String commandData) {
  message = "Unknown command";
  subMessage = "`/help` for details";
}


bool loadFiles() {
  FileData mnFile = readFile(SPIFFS, "/mn.txt");
  encrytptedMnemonic = mnFile.data;

  FileData pwdFile = readFile(SPIFFS, "/hash.txt");
  passwordHash = pwdFile.data;

  return mnFile.success && pwdFile.success;
}

bool wipeHww(String password) {
  if (isAlphaNumeric(password) == false)
    return false;

  deleteFile(SPIFFS, "/mn.txt");
  deleteFile(SPIFFS, "/hash.txt");
  String mn = createMnemonic(24); // todo: allow 12 also
  keyHash = hashPassword(password); // todo: rename var
  Serial.println("wipeHww mnemonic: " + mn); // todo: remove
  Serial.println("wipeHww keyHash: " + keyHash);
  writeFile(SPIFFS, "/mn.txt", mn);
  writeFile(SPIFFS, "/hash.txt", keyHash);

  delay(DELAY_MS);
  return true;
}

Command extractCommand(String s) {
  int spacePos = s.indexOf(" ");
  command = s.substring(0, spacePos);
  if (spacePos == -1) {
    commandData = "";
  } else {
    commandData = s.substring(spacePos + 1, s.length());
  }
  return {command, commandData};
}
