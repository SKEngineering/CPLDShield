#include <Crc16.h>

#include <Wire.h>

#define SLAVE_ADDR 0x40

/*OP CODES*/
#define READ_ID_OP      0xE0
#define OFFLINE_OP      0x08C6
#define RD_BUSY_OP      0xF0
#define RD_STATUS_OP    0x3C
#define ERS_FLASH_OP    0x0F0E
#define RST_PTR_OP      0x46
#define PRGM_FLASH_OP   0x010000070
#define RST_PTRU_OP     0x47 //UFM page pointer
#define PROG_USR_OP     0xC2
#define READ_USR_OP     0xC0
#define PROG_FTR_ROW_OP 0xE4
#define READ_FTR_ROW_OP 0xE7
#define PROG_FTR_BIT_OP 0xF8
#define READ_FTR_BIT_OP 0xFB
#define DONE_PROG_OP    0x5E
#define REFRESH_OP      0x79

/*WRITE LENS*/
#define SYNC_LEN 1
#define BIT_LEN 2
#define RST_LEN 3
#define CMD_LEN 4
#define RECV_LEN 4
#define USR_LEN 4
#define FTR_LEN 8
#define MEM_LEN 18

/*MISC*/
#define RETRY_AMOUNT 3
#define COM_SPEED 115200

union command
{
  uint32_t cmd32;
  uint8_t  cmd[CMD_LEN];
};

void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(COM_SPEED);
  //Serial.setTimeout(10);

}

void print_hex(uint32_t num){
  char buf[64];
  itoa(num,buf,16);
  Serial.write(buf);
}

char operations[SYNC_LEN];

void erase(){
  //-------------------------------------------------------------------------------
  //MachXO2ProgrammingandConfigurationUsageGuide.pdf pg 43
  //-------------------------------------------------------------------------------
  command cmd;
  uint8_t I2CRecvBuf[SYNC_LEN];
  
  cmd.cmd32 = READ_ID_OP;
  send_data(cmd.cmd, CMD_LEN);
  
  int idx = read_data_buff(I2CRecvBuf, RECV_LEN);

  print_hex(idx);
  Serial.write(" - ");
  
  for(int i = 0; i < idx; i++)
    print_hex(I2CRecvBuf[i]);

  Serial.write("\r\n");

  cmd.cmd32 = OFFLINE_OP;
  send_data(cmd.cmd, RST_LEN);
  
  wait_for_busy();

  if(!check_status_ok())
    cleanup();
  
  cmd.cmd32 = ERS_FLASH_OP;
  send_data(cmd.cmd, CMD_LEN);

  wait_for_busy();

  if(!check_status_ok())
    cleanup();
}

void program(){
  command cmd;
  uint8_t header = 0;
  uint8_t recvBuf[SYNC_LEN];

  //-------------------------------------------------------------------------------
  //MachXO2ProgrammingandConfigurationUsageGuide.pdf pg 44
  //program configuration flash
  //-------------------------------------------------------------------------------
  
  Serial.readBytes(recvBuf, SYNC_LEN); // receive bit indicating if configuration memory will be programmed
  if(*recvBuf) {
    
    cmd.cmd32 = RST_PTR_OP; //reset page pointer
    send_data(cmd.cmd, CMD_LEN);
    programPages(PRGM_FLASH_OP, MEM_LEN);
    
    Serial.write("DONE\r\n"); //NACK to signal completion of the program pages
  }
  
  //-------------------------------------------------------------------------------
  //UFM program sequence
  //-------------------------------------------------------------------------------

  Serial.readBytes(recvBuf, SYNC_LEN); // receive bit indicating if UFM will be programmed
  if(*recvBuf) {
  
    cmd.cmd32 = RST_PTRU_OP; //reset UFM page pointer
    send_data(cmd.cmd, MEM_LEN);
    programPages(PRGM_FLASH_OP, MEM_LEN);
    
    Serial.write("DONE\r\n");
  }

  //-------------------------------------------------------------------------------
  //MachXO2ProgrammingandConfigurationUsageGuide.pdf pg 45
  //Usercode program sequence
  //-------------------------------------------------------------------------------
  
  Serial.readBytes(recvBuf, SYNC_LEN); // receive bit indicating if usercode will be programmed
  if(*recvBuf) {
    programPage(PROG_USR_OP, USR_LEN);
    Serial.write("ACK\r\n");
  }

  if(!check_status_ok())
      cleanup();

  verifyUserCode();
  
  //-------------------------------------------------------------------------------
  //MachXO2ProgrammingandConfigurationUsageGuide.pdf pg 46
  //write feature row sequence
  //-------------------------------------------------------------------------------

  Serial.readBytes(recvBuf, SYNC_LEN); // receive bit indicating if feature row will be programmed
  if(*recvBuf) {
    programPage(PROG_FTR_ROW_OP, FTR_LEN);
    Serial.write("ACK\r\n");
  }
  
  wait_for_busy();
  verifyFeatureRow();
  
  //-------------------------------------------------------------------------------
  //MachXO2ProgrammingandConfigurationUsageGuide.pdf pg 46-47
  //write feature row bits sequence
  //-------------------------------------------------------------------------------

  Serial.readBytes(recvBuf, SYNC_LEN); // receive bit indicating if feature bits will be programmed
  if(*recvBuf) {
    programPage(PROG_FTR_BIT_OP,BIT_LEN);
    Serial.write("ACK\r\n");
  }

  wait_for_busy();
  verifyFeatureBits();

  Serial.write("done\r\n");  
  cmd.cmd32 = DONE_PROG_OP;
  send_data(cmd.cmd, CMD_LEN);
  
  wait_for_busy();
  if(!check_status_ok())
      cleanup();

  //-------------------------------------------------------------------------------
  //MachXO2ProgrammingandConfigurationUsageGuide.pdf pg 49
  //finish programming
  //-------------------------------------------------------------------------------
  int tries = 10;
  while( tries--){
    cmd.cmd32 = REFRESH_OP;
    send_data(cmd.cmd, CMD_LEN);
    wait_for_busy();
    if(check_status_ok())
      break;
  }
}

void cleanup(){
  //-------------------------------------------------------------------------------
  //MachXO2ProgrammingandConfigurationUsageGuide.pdf pg 50
  //cleanup on failed programming
  //-------------------------------------------------------------------------------
  command cmd;
  
  cmd.cmd32 = ERS_FLASH_OP;
  send_data(cmd.cmd, CMD_LEN);

  wait_for_busy();
  
  cmd.cmd32 = REFRESH_OP;
  send_data(cmd.cmd, CMD_LEN);
}

void loop() {  
  Serial.write("ok\r\n");

  Serial.readBytes(operations, SYNC_LEN);
  operations[0] -= '0';
  
  if(*operations > 0 && *operations < 4)
    erase();
  if(*operations > 1)
    program();

  for(;;);
}
