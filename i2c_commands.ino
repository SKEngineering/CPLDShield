
void send_data(uint8_t* cmd_buf, uint32_t len){
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write(cmd_buf,len);
  Wire.endTransmission();
}

void write_memory(uint8_t* cmd_buf, uint8_t* fuses, uint32_t len){
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write(cmd_buf, 4);
  Wire.write(fuses, len);
  Wire.endTransmission();
}

uint32_t read_data_from(uint8_t* cmd_buf, uint32_t len){
  send_data(cmd_buf, CMD_LEN);
  return read_data32(len);
}

uint32_t read_data32(uint32_t len){
  uint32_t x = 0;
  Wire.requestFrom(SLAVE_ADDR, len);
  while(Wire.available()){
    x <<= 8;
    x |= Wire.read();
  }
  return x;
}

uint32_t read_data_buff(uint8_t* buf, uint32_t len){
  uint32_t idx = 0;
  memset(buf, 0, len);
  Wire.requestFrom(SLAVE_ADDR, len);
  while(Wire.available()){
    buf[idx] = Wire.read();
    idx++;
  }
  return idx;
}

void wait_for_busy(){
  unsigned int busy = 0x80;
  command cmd;
  cmd.cmd32 = RD_BUSY_OP;
  while(busy & (1<<7))
    busy = read_data_from(cmd.cmd, 1);
}

bool check_status_ok(){
  command cmd;
  cmd.cmd32 = RD_STATUS_OP;
  int value = read_data_from(cmd.cmd, CMD_LEN) & (1<<12 | 1 << 13);
  return value == 0;
}

