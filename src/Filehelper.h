#ifndef FILEHEALPER_H
#define FILEHEALPER_H
#include <Arduino.h>
#include <FS.h>

// Write
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

// Append
void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file) {
    Serial.println("Failed to open file for appending");
    return;
  }

  time_t t = file.getLastWrite();
  struct tm *tmstruct = localtime(&t);

  char bufferDate[50]; // Adjust buffer size as needed
  snprintf(bufferDate, sizeof(bufferDate), "%d-%02d-%02d", 
          (tmstruct->tm_year) + 1900, 
          (tmstruct->tm_mon) + 1, 
          tmstruct->tm_mday);
  char bufferTime[50]; // Adjust buffer size as needed
  snprintf(bufferTime, sizeof(bufferTime), "%02d:%02d:%02d", 
          tmstruct->tm_hour, 
          tmstruct->tm_min, 
          tmstruct->tm_sec);
          
  String lastWriteTime = bufferDate;
  String finalString = String(bufferDate) + "," + String(bufferTime) + "," + String(message) + "\n";
  Serial.println(lastWriteTime);
  if(file.print(finalString.c_str())) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void appendUserFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file) {
    Serial.println("Failed to open file for appending");
    return;
  }

  String finalString = String(message) + "\n";

  if(file.print(finalString.c_str())) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void deleteLineFromFile(fs::FS &fs, const char* filename, int lineNumber) {
  File file = fs.open(filename);
  if (!file) {
    Serial.println("Failed to open file for reading.");
    return;
  }

  // Read all lines except the one to delete
  String lines = "";
  int currentLine = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (currentLine != lineNumber) {
      lines += line + "\n";
    }
    currentLine++;
  }
  file.close();

  // Write back all lines except the deleted one
  file = fs.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing.");
    return;
  }

  file.print(lines);
  file.close();
  Serial.println("Line deleted successfully.");
}

String getRoleFromFile(fs::FS &fs, const char *filename, String uid)
{
    File file = fs.open(filename);
    if (!file)
    {
        Serial.println("Failed to open file for reading.");
        return "";
    }

    // Skip the header line
    file.readStringUntil('\n');

    // Read each line and check for UID
    while (file.available())
    {
        String line = file.readStringUntil('\n');

        int commaIndex = line.indexOf(',');
        if (commaIndex > 0)
        {
            String fileUID = line.substring(0, commaIndex);
            String role = line.substring(commaIndex + 1);

            // Compare UID
            if (fileUID == uid)
            {
                file.close();
                role.trim(); // Remove any extra spaces or newline characters
                return role;
            }
        }
    }
    file.close();
    return ""; // Return empty string if UID not found
}


#endif