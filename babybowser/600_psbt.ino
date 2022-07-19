PSBT psbt; // todo: revisit, required to be global?
String psbtStr = "";

String serialData1;
String password;

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
      serialData1 = Serial.readStringUntil('\n');
    }
    if (serialData1 == "sign") {
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