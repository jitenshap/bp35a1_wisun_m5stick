#define RXD2 36
#define TXD2 0
String BPWD = "YOUR_B_ROUTE_PASSWORD";
String BID = "YOUR_B_ROUTE_ID";

String addr = "NG";
String panId = "NG";
String channel = "NG";

void discard_buf()
{
  delay(1000);
  while(Serial2.available())
  {
    Serial2.read();
  }
}
 
bool read_res()
{
  Serial2.flush();
  int timeout = 0;
  while(timeout < 200)
  {
    String line = Serial2.readStringUntil('\n');
    Serial.println(line);
    if(line.indexOf("OK") > -1 )
    {
      discard_buf();
      return true;
    }
    delay(100);
    timeout ++;
  }
  discard_buf();
  return false;
}


bool get_scan_result(int dur)
{
  Serial2.flush();
  int timeout = 0;
  while(timeout < (dur + 10) * 10)
  {
    while(Serial2.available())
    {
      String line = Serial2.readStringUntil('\n');
      Serial.println(line);
      if(line.indexOf("Channel:") > -1)
      {
       int startPos = line.indexOf("Channel:") + 8;
        channel = line.substring(startPos);
        channel.trim();       
      }
      else if(line.indexOf("Pan ID:") > -1)
      {
       int startPos = line.indexOf("Pan ID:") + 7;
        panId = line.substring(startPos);
        panId.trim();       
      }
      else if(line.indexOf("Addr:") > -1)
      {
        int startPos = line.indexOf("Addr:") + 5;
        addr = line.substring(startPos);
        addr.trim();
      }

      else if(line.indexOf("EVENT 22 ") > -1)
      {
        discard_buf();
        if(addr == "NG")
          return false;
        else
          return true;
      }
    }
    timeout ++;
    delay(100);
  }
  discard_buf();
  return false;
}

String get_ipv6_addr()
{
  Serial2.flush();
  int timeout = 0;
  while(timeout < 200)
  {
    while(Serial2.available())
    {
      String line = Serial2.readStringUntil('\n');
      Serial.println(line);
      line.trim();
      discard_buf();
      return line;
    }
    timeout ++;
    delay(100);
  }
  discard_buf();
  return "NG";
}

bool get_connecting_status()
{
  Serial2.flush();
  int timeout = 0;
  while(timeout < 300)
  {
    while(Serial2.available())
    {
      String line = Serial2.readStringUntil('\n');
      Serial.println(line);
      if(line.indexOf("EVENT 25") > -1)
      {
        discard_buf();
        return true;
      }
      else if(line.indexOf("EVENT 24") > -1)
      {
        discard_buf();
        return false;
      }
    }
    timeout ++;
    delay(100);
  }
  discard_buf();
  return false;
}

int get_and_parse_inst_data()
{
  Serial2.flush();
  int timeout = 0;
  while(timeout < 100)
  {
    while(Serial2.available())
    {
      String line = Serial2.readStringUntil('\n');
      Serial.println(line);
      if(line.indexOf("ERXUDP ") > -1)
      {
        String power = line.substring(line.length() - 8);
        power.trim();
        char powerChar[9];
        power.toCharArray(powerChar, 8);
        int parsed = strtoul(powerChar, NULL, 16);
        discard_buf();
        return parsed;
      }
    }
    timeout ++;
    delay(100);
  }
  discard_buf();
  return -1;  
}

int get_inst_power()
{
  Serial.println("Send request");
  Serial2.print("SKSENDTO 1 " + addr + " 0E1A 1 000E ");
  char sendBuf[] = {0x10, 0x81, 0x00, 0x01, 0x05, 0xFF, 0x01, 0x02, 0x88, 0x01, 0x62, 0x01, 0xE7, 0x00};
  for(int i = 0; i < sizeof(sendBuf); i++)
  {
    Serial2.write(sendBuf[i]);  
  }
  Serial2.print("\r\n");
  Serial.print("SKSENDTO 1 " + addr + " 0E1A 1 000E ");
  for(int i = 0; i < sizeof(sendBuf); i++)
  {
    Serial.write(sendBuf[i]);  
  }
  Serial.print("\r\n");
  if(!read_res())
  {
    Serial.println("err");
  }
  int power = get_and_parse_inst_data();
  return power;
}

void setup() 
{
  delay(1000);
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  Serial.println("Hello");
  discard_buf();  
  delay(2000);
  Serial.println("Disable echo");
  Serial2.print("SKSREG SFE 0\r\n");
  Serial.print("SKSREG SFE 0\r\n");
  delay(500);
  Serial.println("Check connection");
  Serial2.print("SKVER\r\n");
  Serial.print("SKVER\r\n");
  if(!read_res())
  {
    Serial.println("err"); 
  }
  Serial.println("Force disconnect");
  Serial2.print("SKTERM\r\n");
  discard_buf();
  Serial.println("Set password");
  Serial2.print("SKSETPWD C " + BPWD + "\r\n");
  Serial.print("SKSETPWD C " + BPWD + "\r\n");
  if(!read_res())
  {
    Serial.println("err");
  }
  Serial.println("Set ID");
  Serial2.print("SKSETRBID " + BID + "\r\n");
  Serial.print("SKSETRBID " + BID + "\r\n");
  if(!read_res())
  {
    Serial.println("err");
  }
  Serial.println("Scan meter");
  Serial2.print("SKSCAN 3 FFFFFFFF 6\r\n");
  Serial.print("SKSCAN 3 FFFFFFFF 6\r\n");
  if(!read_res())
  {
    Serial.println("err");
    Serial.flush();
    esp_restart();
  }
  if(!get_scan_result(6))
  {
    Serial.println("err");
    Serial.flush();
    esp_restart();
  }
  Serial.println("Scan result\r\nAddr: " + addr + ", Channel: " + channel + ", Pan ID: " + panId);
  Serial.println("convert meter address format");  
  Serial2.print("SKLL64 " + addr + "\r\n");
  Serial.print("SKLL64 " + addr + "\r\n");
  addr = get_ipv6_addr();
  if(addr == "NG")
  {
    Serial.println("err");
    Serial.flush();
    esp_restart();
  }
  Serial.println("IPv6 Addr: " + addr);
  Serial.println("Set channel");
  Serial2.print("SKSREG S2 " + channel +"\r\n");
  Serial.print("SKSREG S2 " + channel +"\r\n");
  if(!read_res())
  {
    Serial.println("err");
    Serial.flush();
    esp_restart();
  }
  Serial.println("Set pan ID");
  Serial2.print("SKSREG S3 " + panId +"\r\n");
  Serial.print("SKSREG S3 " + panId +"\r\n");
  if(!read_res())
  {
    Serial.println("err");
    Serial.flush();
    esp_restart();
  } 
  Serial.println("Connect");
  Serial2.print("SKJOIN " + addr +"\r\n");
  Serial.print("SKJOIN " + addr +"\r\n");
  if(!read_res())
  {
    Serial.println("err");
    Serial.flush();
    esp_restart();
  } 
  if(!get_connecting_status())
  {
    Serial.println("err");
    Serial.flush();
    esp_restart();    
  }
}
 
void loop() 
{
  int power = get_inst_power();
  if(power == -1)
  {
    Serial.println("err");
    Serial.flush();
    esp_restart();        
  }
  Serial.println((String)power + "W now.");
  delay(10000);
}
