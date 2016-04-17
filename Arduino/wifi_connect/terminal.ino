

void handleTerminal(String line)
{
  int subIndex = line.indexOf(' ');
  if(subIndex > 0)
  {
    String cmd = line.substring(0, subIndex);
    String arg = line.substring(subIndex + 1);

    //Serial.println(cmd);
    //Serial.println(arg);

    if(cmd == "setip")
    {
      Serial.print("Set IP: "); Serial.println(arg);

      pumpFrame.setIP(arg, 0);

      //pumpFrame.crc = calc_crc((uint8_t*)&pumpFrame, (sizeof(PumpFrame) - 1));
    }
    else if(cmd == "setport")
    {
      Serial.print("Set PORT: "); Serial.println(arg);

      pumpFrame.setPort(arg ,0);
    }
    else if(cmd == "cfg")
    {
      pumpFrame.print();
    }
  }
}

