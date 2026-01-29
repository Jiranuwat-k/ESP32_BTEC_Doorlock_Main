#ifndef FILEHELPER_H
#define FILEHELPER_H
#include <Arduino.h>
#include <FS.h>

// Write
inline int writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open file for writing");
    return FILE_ERR_OPEN;
  }
  if(file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
  return FILE_OK;
}

// Append
inline int appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file) {
    Serial.println("Failed to open file for appending");
    return FILE_ERR_OPEN;
  }

  struct tm tmstruct;
  if(!getLocalTime(&tmstruct, 100)){
    // If failed to get local time, try to use file time or default
     time_t t = file.getLastWrite();
     struct tm *tmp = localtime(&t);
     tmstruct = *tmp;
     Serial.println("Failed to obtain time, using file time");
  }

  char bufferDate[50];
  snprintf(bufferDate, sizeof(bufferDate), "%d-%02d-%02d", 
          (tmstruct.tm_year) + 1900, 
          (tmstruct.tm_mon) + 1, 
          tmstruct.tm_mday);
  char bufferTime[50];
  snprintf(bufferTime, sizeof(bufferTime), "%02d:%02d:%02d", 
          tmstruct.tm_hour, 
          tmstruct.tm_min, 
          tmstruct.tm_sec);
          
  String finalString = String(bufferDate) + "," + String(bufferTime) + "," + String(message) + "\n";
  
  if(file.print(finalString.c_str())) {
    Serial.println("Message appended");
    file.flush();
  } else {
    Serial.println("Append failed");
  }
  file.close();
  return FILE_OK;
}

inline int appendUserFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file) {
    Serial.println("Failed to open file for appending");
    return FILE_ERR_OPEN;
  }

  String finalString = String(message) + "\n";

  if(file.print(finalString.c_str())) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
    file.close();
    return FILE_ERR_APPEND;
  }
  file.close();
  return FILE_OK;
}

inline int deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
    // Header re-creation logic should be handled by the caller or init function
    // But for safety/legacy retention:
    if(strcmp(path, "/readerlogs.txt") == 0){ // Changed from log.txt
       // Header will be recreated by initLittleFS check
    }else if(strcmp(path, "/users.txt") == 0){
       // Header will be recreated by initLittleFS check
    }
    return FILE_OK;
  } else {
    Serial.println("Delete failed");
    return FILE_ERR_DELETE;
  }
}

inline int deleteLineFromFile(fs::FS &fs, const char* filename, int lineNumber) {
  File file = fs.open(filename);
  if (!file) {
    Serial.println("Failed to open file for reading.");
    return FILE_ERR_OPEN;
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
    return FILE_ERR_OPEN;
  }

  file.print(lines);
  file.close();
  Serial.println("Line deleted successfully.");
  return FILE_OK;
}

// Function to check file size and trim if too large
// Strategy: When full, drop oldest 20%, replace keeping newest 80%
inline void maintainLogFile(fs::FS &fs, const char *path, const char *header) {
  const size_t MAX_LOG_SIZE = 500 * 1024; // 500KB Limit (Holds ~10,000 logs)

  File file = fs.open(path, "r");
  if (!file) return;
  size_t size = file.size();
  file.close();

  if (size > MAX_LOG_SIZE) {
    Serial.printf("Log %s too large (%d). Trimming oldest 20%...\n", path, size);
    
    // 1. Rename original to temp
    String tempPath = "/temp.txt";
    fs.remove(tempPath); 
    fs.rename(path, tempPath); 

    // 2. Open temp to read, original to write
    File inFile = fs.open(tempPath, "r");
    File outFile = fs.open(path, "w");

    if (inFile && outFile) {
        // Write Header
        outFile.print(header);
        if (String(header).indexOf('\n') == -1) outFile.print("\n");

        // Skip first 20% (Oldest data)
        size_t cutoff = size / 5; // Drop 1/5th
        inFile.seek(cutoff);
        inFile.readStringUntil('\n'); // Discard partial line

        // Copy rest
        while (inFile.available()) {
            outFile.write(inFile.read());
        }
        Serial.println("Trimmed successfully.");
    }

    if(inFile) inFile.close();
    if(outFile) outFile.close();
    
    // 3. Delete temp
    fs.remove(tempPath);
  }
}

// Update or Insert User
inline int upsertUser(fs::FS &fs, const char* filename, String uid, String newLineData) {
  File file = fs.open(filename);
  if (!file) {
    return FILE_ERR_OPEN;
  }

  String lines = "";
  bool found = false;
  
  // Keep Header
  if (file.available()) {
     lines += file.readStringUntil('\n') + "\n";
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if(line.length() == 0) continue;

    int commaIndex = line.indexOf(',');
    String fileUID = (commaIndex > 0) ? line.substring(0, commaIndex) : line;

    if (fileUID.equals(uid)) {
      lines += newLineData + "\n"; // Replace
      found = true;
    } else {
      lines += line + "\n"; // Keep
    }
  }
  file.close();

  if (!found) {
    lines += newLineData + "\n"; // Append
  }

  file = fs.open(filename, FILE_WRITE);
  if (!file) return FILE_ERR_WRITE;
  
  file.print(lines);
  file.close();
  Serial.println("User upserted.");
  return FILE_OK;
}

inline String getRoleFromFile(fs::FS &fs, const char *filename, String uid)
{
    File file = fs.open(filename);
    if (!file) {
        return String(FILE_ERR_OPEN);
    }
    // Skip header
    file.readStringUntil('\n');

    while (file.available())
    {
        String line = file.readStringUntil('\n');
        line.trim();
        if(line.length() == 0) continue;

        int firstComma = line.indexOf(',');
        if (firstComma > 0)
        {
            String fileUID = line.substring(0, firstComma);
            if (fileUID == uid)
            {
                int secondComma = line.indexOf(',', firstComma + 1);
                String role;
                if(secondComma > 0) {
                    role = line.substring(firstComma + 1, secondComma);
                } else {
                    role = line.substring(firstComma + 1);
                }
                file.close();
                role.trim(); 
                return role;
            }
        }
    }
    file.close();
    return String(FILE_ERR_NOTFOUND); 
}

inline String getUserDataFromFile(fs::FS &fs, const char *filename, String uid)
{
    File file = fs.open(filename);
    if (!file) return String(FILE_ERR_OPEN);
    
    file.readStringUntil('\n');
    while (file.available())
    {
        String line = file.readStringUntil('\n');
        int commaIndex = line.indexOf(',');
        if (commaIndex > 0)
        {
            String fileUID = line.substring(0, commaIndex);
            if (fileUID == uid)
            {
                file.close();
                line.trim(); 
                return line;
            }
        }
    }
    file.close();
    return String(FILE_ERR_NOTFOUND);
}

// Delete user by UID
inline int deleteUserByUID(fs::FS &fs, const char* filename, String uid) {
  File file = fs.open(filename);
  if (!file) {
    return FILE_ERR_OPEN;
  }

  String lines = "";
  bool deleted = false;
  
  // Keep Header
  if (file.available()) {
     lines += file.readStringUntil('\n') + "\n";
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if(line.length() == 0) continue;

    int commaIndex = line.indexOf(',');
    String fileUID = (commaIndex > 0) ? line.substring(0, commaIndex) : line;

    if (fileUID.equals(uid)) {
      deleted = true; // Skip this line
    } else {
      lines += line + "\n"; // Keep
    }
  }
  file.close();

  if (deleted) {
    file = fs.open(filename, FILE_WRITE);
    if (!file) return FILE_ERR_WRITE;
    file.print(lines);
    file.close();
    Serial.println("User deleted successfully.");
    return FILE_OK;
  }
  
  return FILE_ERR_NOTFOUND;
}

// Update Usage Stats (UID,Count,LastAccess)
inline void updateUsageStats(fs::FS &fs, const char *path, String uid) {
    File file = fs.open(path, "r");
    if (!file) return;

    String lines = "";
    bool found = false;
    
    if(file.available()) {
         String header = file.readStringUntil('\n');
         String tempHeader = header;
         tempHeader.trim();
         if(header.startsWith("/*") || tempHeader.length() == 0) header = "UID,Count,LastAccess"; // Sanity check
         lines += header + "\n";
    } else {
         lines += "UID,Count,LastAccess\n";
    }

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if(line.length() == 0 || line.startsWith("/*")) continue; // Skip empty or comments
        
        int comma = line.indexOf(',');
        if(comma == -1) continue;

        String fUID = line.substring(0, comma);
        
        if (fUID == uid) {
             int secondComma = line.indexOf(',', comma + 1);
             if(secondComma == -1) secondComma = line.length();
             
             String countStr = line.substring(comma + 1, secondComma);
             int count = countStr.toInt() + 1;
             
             struct tm tmstruct;
             if(!getLocalTime(&tmstruct, 100)) {
                 // Zero out if time sync failed
                 tmstruct.tm_year = 0; tmstruct.tm_mon = 0; tmstruct.tm_mday = 1;
                 tmstruct.tm_hour = 0; tmstruct.tm_min = 0;
                 tmstruct.tm_sec = 0;
             }
             char buffer[30];
             snprintf(buffer, sizeof(buffer), "%d-%02d-%02d %02d:%02d", tmstruct.tm_year+1900, tmstruct.tm_mon+1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min);
             
             lines += uid + "," + String(count) + "," + String(buffer) + "\n";
             found = true;
        } else {
             lines += line + "\n";
        }
    }
    file.close();

    if (!found) {
        struct tm tmstruct;
        getLocalTime(&tmstruct, 100);
        char buffer[30];
        snprintf(buffer, sizeof(buffer), "%d-%02d-%02d %02d:%02d", tmstruct.tm_year+1900, tmstruct.tm_mon+1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min);
        lines += uid + ",1," + String(buffer) + "\n";
    }

    file = fs.open(path, "w");
    if (file) {
        file.print(lines);
        file.flush();
        file.close();
        Serial.println("Usage stats updated.");
    }
}

#endif