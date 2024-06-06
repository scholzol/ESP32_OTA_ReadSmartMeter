# ESP32_OTA_Template

This is a repo for a ESP32 OTA Template project structure for VSCode and PlatformIO. It is based on the BasicOTA sketch and the idea from [SensorIOT](https://github.com/SensorsIot/ESP32-OTA).
I adapted it to my needs.

## Installation

In order to use the code provided in this repository you need to provide the credentials of your access point. Or you can comment out the `#include <credentials.h>` and define the ``ssid`` and ``password`` in the main.cpp.

### Provide your credentials

Create a `credentials.h` file (in the sketch folder directly or in the PlatformIO packages folder).
For Windows, I stored the file in `C:\Users\[UserName]\.platformio\packages\framework-arduinoespressif32\libraries\WiFi\src`.

The text file `credentials.h` looks like:

```c++
#pragma once
const char* ssid = "SSID of your AP";
const char* password = "Password of your AP";
```

## additional measures for platformio.ini - extra_scripts

place a file `mysql_credentials.py`

## access MariaDB

I installed the MySQL_MariaDB_Generic library in my .platformio/lib folder, to serve all projects with that library. To access the database you need an additional file `MySQL_credentials.h`.

The text file `MySQL_credentials.h` looks like:

```c++
char user[] = "DB_User"; 
char passwd[] = "DB_User_password";
```

