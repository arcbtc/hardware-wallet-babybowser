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