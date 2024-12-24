#include <embedded.h>
#include <microcuts.h>
#include <lang_assertions.h>

void setup() {
  Serial.begin(9600);
}

String data_input = "";
bool code_input = false;

void loop() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    data_input += c;
    if (data_input.endsWith("run\n")) {
      data_input = "";
      Serial.printf("start\n");
      print_info();
      set_cleanup(clean);
      set_target(validate);
      int r = run_target();
      Serial.printf("%d\n", r);
      Serial.printf("end\n");
      data_input = "";
    } else if (data_input.endsWith("_code_start_")) {
      data_input = "";
      code_input = true;
    } else if (code_input && data_input.endsWith("_code_end_")) {
      code_input = false;
      data_input = data_input.substring(0, data_input.length() - 10);
      Serial.printf("start\n");
      Serial.printf("code: %s\n", data_input.c_str());
      int r = run((char*)data_input.c_str());
      Serial.printf("output: %s\n", getBuffer());
      Serial.printf("%d\n", r);
      clean();
      Serial.printf("end\n");
    } else if (!code_input && data_input.endsWith("\n")) {
      data_input = "";
    }
  }
  delay(1000);
  Serial.printf("ready\n");
}
