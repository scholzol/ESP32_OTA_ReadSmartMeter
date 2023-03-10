Import("env")

import os
import subprocess
import mysql_credentials
from getmac import get_mac_address

try:
    import mysql.connector
except ImportError:
    env.Execute("$PYTHONEXE -m pip install mysql-connector-python")
    import mysql.connector
try:
    import git
except ImportError:
    env.Execute("$PYTHONEXE -m pip install GitPython")
    import git

VERSION_FILE = "Version.h"
UPLOAD_PORT = env['UPLOAD_PORT']
cwd = os.path.abspath(os.getcwd())
print(cwd)

def is_pio_build():                                 # check, whether you are in a build process, to avoid extra_script execution during Platformio.ini changes or branch changes
    from SCons.Script import DefaultEnvironment
    env = DefaultEnvironment()
    if "IsCleanTarget" in dir(env) and env.IsCleanTarget(): return False
    return not env.IsIntegrationDump()

def getmacaddr(port):
    ip_mac_addr = "no valid MAC addr"
    if "192" in port:                               # used when upload via OTA
        ip_mac_addr = None
        while ip_mac_addr == None:
            ip_mac_addr = get_mac_address(ip=port)
        return ip_mac_addr
    elif "COM" in port:                             # used, if ESP32 is connected via COM - port
        ip_mac_addr = "20:00:00:00:00:00"
        read_mac = subprocess.run('esptool read_mac', capture_output=True, encoding="utf-8")
        ip_mac_addr = read_mac.stdout               # assign the output of 'esptool read_mac' to ip_mac_addr
        start = ip_mac_addr.find("MAC: ")           # find the MAC address in the "CompletedProcess" - object output
        ip_mac_addr = ip_mac_addr[start+5:start+23] # extract the MAC address string
        return ip_mac_addr
    else:
        return ip_mac_addr

def after_upload(source, target, env):
    print("--------------------- after_upload ----------------------------")
    ip_mac = getmacaddr(UPLOAD_PORT)

    # read the information from the Version.h file and write it into the boards_firmware - table in the esp32 database

    FILE = os.path.join(os.path.join(cwd, "include"), VERSION_FILE)
    if os.path.exists(FILE):
        f = open(FILE, "r")
    file_line = f.readline()
    while file_line:
        words = file_line.split()
        if words[1] == "SHA_short":
            sha_short = words[2]
        elif words[1] == "SemanticVersion":
            semanticversion = words[2]
        elif words[1] == "FullBuildMetaData":
            fullbuildmetadata = words[2]
        elif words[1] == "WorkingDirectory":
            workingdirectory = words[2]
        file_line = f.readline()
    f.close

    mydb = mysql.connector.connect(
    host=mysql_credentials.mysql_host,
    user=mysql_credentials.mysql_user,
    password=mysql_credentials.mysql_passwd,
    database=mysql_credentials.mysql_database
    )

    mycursor = mydb.cursor()

    sql = "INSERT INTO boards_firmware (UPLOAD_PORT,MAC_ADDRESS,WORKING_DIRECTORY,SEMANTIC_VERSION,FULL_DATA,SHA_SHORT) VALUES (%s,%s,%s,%s,%s,%s)"
    val = (UPLOAD_PORT,ip_mac,workingdirectory,semanticversion,fullbuildmetadata,sha_short)
    mycursor.execute(sql, val)

    mydb.commit()

    print(mycursor.rowcount, "record inserted.")
    print("--------------------- end after upload ------------------------")

if is_pio_build():                                  # modify Version.h during build process
    os.system("powershell -NonInteractive -NoLogo -NoProfile -File .\GetVersion.ps1 -ProjectDirectory . -OutputFile .\include\Version.h")

env.AddPostAction("upload", after_upload)
