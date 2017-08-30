void programPages(uint32_t opCode, uint32_t pageLen) {
  uint8_t header = 0;
  uint8_t recvBuf[pageLen + 3];
  memset(recvBuf, 0, pageLen + 3);
  command cmd;

  Crc16 crc;
  crc.clearCrc();

  cmd.cmd32 = opCode;
  while ((header & 1 << 7) == 0) {
    int retries = RETRY_AMOUNT;
    
    while(retries){
      Serial.write("REQUEST\r\n");
      Serial.readBytes(recvBuf, pageLen + 1); //receive header + CRC + payload
      
      uint16_t value = crc.XModemCrc(&recvBuf[3], 0, 16);
      
      if (memcmp(&recvBuf[1], &value, 2) == 0) {
        break;
      }
      
      retries--;
    }
    
    if(retries == 0){
      Serial.write("FAILED\r\n");
      for(;;);
    }
    
    write_memory(cmd.cmd, &recvBuf[3], pageLen);

    wait_for_busy();

    header = recvBuf[0];
    if ((header & 1 << 7) == 0) //last payload will have stop bit on, do not ACK then
      Serial.write("ACK\r\n");
  }
}


void programPage(uint32_t opCode, uint32_t pageLen) {
  command cmd;
  cmd.cmd32 = opCode;
  
  uint8_t recvBuf[pageLen+2];
  memset(recvBuf, 0, pageLen+2);

  Crc16 crc;
  crc.clearCrc();

  int retries = RETRY_AMOUNT;
  while(retries){
    Serial.write("REQUEST\r\n");
    Serial.readBytes(recvBuf, pageLen+2); //receive CRC + payload
    uint16_t value = crc.XModemCrc(&recvBuf[2], 0, pageLen);
    
    if (memcmp(recvBuf, &value, 2) == 0) {
      break;
    }
    retries--;
  }
  
  if(retries == 0){
    Serial.write("FAILED\r\n");
    cleanup();
  }


  
  write_memory(cmd.cmd, &recvBuf[2], pageLen);
}


bool verifyMemory(uint8_t* opCode, uint32_t readLen) {
  uint8_t UARTRecvBuf[16];
  uint8_t I2CRecvBuf[16];
  memset(UARTRecvBuf, 0, 16);
  memset(I2CRecvBuf, 0, 16);

  Serial.readBytes(UARTRecvBuf, readLen);

  send_data(opCode, CMD_LEN);
  int idx = read_data_buff(I2CRecvBuf, readLen);

  for (int i = 0; i < idx; i++)
    print_hex(I2CRecvBuf[i]);

  Serial.write(" - ");

  for (int i = 0; i < idx; i++)
    print_hex(UARTRecvBuf[i]);

  return memcmp(UARTRecvBuf, I2CRecvBuf, readLen) == 0;
}


void verifyUserCode() {
  if (*operations != 3)
    return;

  command cmd;
  cmd.cmd32 = READ_USR_OP;
  
  if (verifyMemory(cmd.cmd, USR_LEN))
    Serial.write("Usercode verification ok\r\n");
  else
    Serial.write("Usercode verification failed\r\n");
}


void verifyFeatureRow() {
  if (*operations != 3)
    return;

  command cmd;
  cmd.cmd32 = READ_FTR_ROW_OP;
  
  if (verifyMemory(cmd.cmd, FTR_LEN))
    Serial.write("feature row verification ok\r\n");
  else
    Serial.write("feature row verification failed\r\n");
}


void verifyFeatureBits() {
  if (*operations != 3)
    return;

  command cmd;
  cmd.cmd32 = READ_FTR_BIT_OP;
  
  if (verifyMemory(cmd.cmd, BIT_LEN))
    Serial.write("feature bits verification ok\r\n");
  else
    Serial.write("feature bits verification failed\r\n");
}



