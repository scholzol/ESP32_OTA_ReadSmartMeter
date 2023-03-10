/**
 * @file updateBoardTable.cpp
 * @author Olaf Scholz (olaf.scholz@online.de)
 * @brief 
 * @version 0.1
 * @date 2023-03-08
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <MySQL_Generic.h>
#include <MySQL_credentials.h>
#include <algorithm>

// define MySQL variables
#define USING_HOST_NAME     false

#if USING_HOST_NAME
  // Optional using hostname, and Ethernet built-in DNS lookup
  char MySQLserver[] = "your_account.ddns.net"; // change to your server's hostname/URL
#else
  IPAddress MySQLserver(192, 168, 178, 49);
#endif

uint16_t server_port = 3306;                  //port for the access of the MySQL database (standard:3306);

char default_database[] = "esp32";            // database for the ESP32 information1
char default_table[]    = "boards";           // table for ESP32 Hardware information

MySQL_Connection conn((Client *)&client);
MySQL_Query *query_mem;
time_t timer;

void updateBoardTable(char ssid32[13])
{
    // insert data into MySQL database
    
    // Sample query
    String SELECT_SQL = String("SELECT ChipID FROM ") + default_database + "." + default_table;
    String INSERT_SQL = String("INSERT INTO ") + default_database + "." + default_table  + " (ChipModel, MAC_Address, IP_Address, ChipID) VALUES ('" + String(ESP.getChipModel()) + "','" + String(WiFi.macAddress()) + "','" + WiFi.localIP().toString() + "','" + ssid32 + "')";
    String UPDATE_SQL = String("UPDATE ") + default_database + "." + default_table  + " SET ChipModel='" + String(ESP.getChipModel()) + "', MAC_Address='" + String(WiFi.macAddress()) + "', IP_Address='" + WiFi.localIP().toString() + "' WHERE ChipID='" + ssid32 + "'";
    String rowValues[20]= {" "};

      MySQL_Query query_mem = MySQL_Query(&conn);

    if (conn.connectNonBlocking(MySQLserver, server_port, user, passwd) != RESULT_FAIL)
    {
        delay(500);
        if (conn.connected())
        {
        MYSQL_DISPLAY(SELECT_SQL);
        
        // Execute the query
        // KH, check if valid before fetching
        if ( !query_mem.execute(SELECT_SQL.c_str()) )
        {
            MYSQL_DISPLAY("Select error");
            return;
        }
        else
        {
            // Fetch the columns and print them
            column_names *cols = query_mem.get_columns();

            for (int f = 0; f < cols->num_fields; f++)
            {
                MYSQL_DISPLAY0(cols->fields[f]->name);

                if (f < cols->num_fields - 1)
                {
                MYSQL_DISPLAY0(", ");
                }
            }
            
            MYSQL_DISPLAY();
            
            // Read the rows and print them
            row_values *row = NULL;
            int i = 1;
            do
            {
                row = query_mem.get_next_row();

                if (row != NULL)
                {
                for (int f = 0; f < cols->num_fields; f++)
                {
                    MYSQL_DISPLAY0(row->values[f]);
                    rowValues[i] = row->values[f];
                    if (f < cols->num_fields - 1)
                    {
                    MYSQL_DISPLAY0(", ");
                    }
                }
                
                MYSQL_DISPLAY();
                i++;
                }
            } while (row != NULL);
        }
        // now check whether the ChipID already exists
        bool isInArray = false;
        isInArray = std::find(std::begin(rowValues), std::end(rowValues), ssid32) != std::end(rowValues);

        if (isInArray)
        {
            MYSQL_DISPLAY(UPDATE_SQL);
            if ( !query_mem.execute(UPDATE_SQL.c_str()) )
            {
            MYSQL_DISPLAY("UPDATE error");
            return;
            }
        }
        else
        {
            MYSQL_DISPLAY(INSERT_SQL);
            if ( !query_mem.execute(INSERT_SQL.c_str()) )
            {
            MYSQL_DISPLAY("INSERT error");
            return;
            }

        }
    }
    else
        {
        MYSQL_DISPLAY("Disconnected from Server. Can't insert.");
        }
        conn.close();
    }
    else 
    {
        MYSQL_DISPLAY("\nConnect failed. Trying again on next iteration.");
    }
}
