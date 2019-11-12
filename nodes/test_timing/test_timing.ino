
unsigned long startTime;  //some global variables available anywhere in the program
unsigned long currentTime;
const unsigned long interval = 20000UL; // 4 seconds


void setup()
{
  Serial.begin(9600);  //start Serial in case we need to print debugging info
  startTime = millis();  //initial start time
}

void loop()
{
  currentTime = millis();  //get the current "time" (actually the number of milliseconds since the program started)
  if (currentTime - startTime < interval)  //test whether the period has elapsed
  {
    Serial.println("Relay");
    delay(500);
    //startTime = currentTime;  //IMPORTANT to save the start time of the current LED state.
  }
  else{
    Serial.println("Invio");
    delay(1000);

    Serial.println("\n reset");
    delay(1000);
    startTime = millis();
  }

}
