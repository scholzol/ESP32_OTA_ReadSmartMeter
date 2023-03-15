/*
 * 2017 - blog.404.at
 *
 * Read values from IEC 62056-21 powermeter.
 * Working with RaspberryPi / BananaPi
 *
 * ToDo:
 * Needs some errorhandling.
 * Negotiation of higher baudrate not working on EMH ITZ powermeter.
 *
 * */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdbool.h>
#include <mysql.h>
#include <my_global.h>
#include <stdint.h>
#include <time.h>

#define SQL_HOST "homeAutomation"
#define SQL_USER "homeAutomation"
#define SQL_PASS "yourpasswordhere"
#define SQL_DB   "homeAutomation"


typedef struct values_t {
	uint32_t electricityID;
	uint8_t billingPeriod;
	uint32_t activeFirmware;
	uint32_t timeSwitchPrgmNo;
	uint32_t localTime;
	uint32_t localDate;
	float sumPower1;
	float sumPower2;
	float sumPower3;
	float power;
	float U_L1;
	float U_L2;
	float U_L3;
	uint32_t deviceNo;
	uint8_t freqL1;
	uint8_t freqL2;
	uint8_t freqL3;
} values_t;



int init_serial ( int baudrate )
  {

   int fd;
   struct termios options;

   // For termios options see http://linux.die.net/man/3/termios

   fd = open ( "/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY );     // For use on BPI-R1 change to /dev/ttyS2

   if ( fd >= 0 ) {
     // Get current options
     fcntl ( fd, F_SETFL, 0 );
     if ( tcgetattr ( fd, &options ) != 0 ) return (-1);
     memset ( &options, 0, sizeof ( options ) ); // Save options to possibly restore them later

     // Set baudrate
     if (baudrate == 300) {
	     cfsetispeed ( &options, B300 );
	     cfsetospeed ( &options, B300 );
     }

     if ( baudrate == 4800 ) {
	     cfsetispeed ( &options, B4800 );
	     cfsetospeed ( &options, B4800 );
     }


     options.c_cflag |= PARENB;          // Enable parity generation on output and parity checking on input
     options.c_cflag &= ~PARODD;         // Unset odd parity
     options.c_cflag &= ~CSTOPB;         // Delete Flag for 2 stop bits, rather than one
     options.c_cflag &= ~CSIZE;          // Unset character size mask
     options.c_cflag |= CS7;             // Character size mask (CS5, CS6, CS7 or CS8)

     options.c_cflag |= CLOCAL;          // Ignore modem control lines
     options.c_cflag |= CREAD;           // Enable receiver

     options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
     options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);

     options.c_iflag = IGNPAR;           // Ignore framing errors and parity errors
     options.c_oflag &= ~OPOST;          // Disable implementation-defined output processing, set to "raw" Input
     options.c_cc[VMIN]  = 0;            // Minimum number of characters for noncanonical read
     options.c_cc[VTIME] = 10;           // Timeout in deciseconds for noncanonical read
     tcflush ( fd,TCIOFLUSH );          // flushes both data received but not read, and data written but not transmitted

     // if (tcsetattr(fd, TCSAFLUSH, &options) != 0) return(-1); // the change occurs after all output written to the object referred by fd has been transmitted, and all input that has been received but not read will be discarded before the change is made.
     if ( tcsetattr ( fd, TCSANOW, &options ) != 0 ) return ( -1) ; // The change occurs immediately

   }
   return ( fd );
}

int sendbytes ( int fd, char * Buffer, int Count )

  {
  int sent;

  sent = write ( fd, Buffer, Count );
  if ( sent < 0 )
    {
    perror ( "sendbytes failed - error!" );
    return false;
    }
  if ( sent < Count )
    {
    perror ( "sendbytes failed - truncated!" );
    }
  return sent;
  }



int receiveBytes ( int fd, char * retBuffer ) {

	char buf[101], c;
	int count, i = 0;
	        
	do {

		count =	read ( fd, (void*)&c, 1 );
		if ( c == 0x3 ) {	// ETX
				return false;
		}

		if ( count > 0 ) {
			if ( c  != '\r' && c != '\n')
				buf[i++] = c;
		}
		
	} while ( c != '\n' && i < 100 && count >= 0 );

	if ( count < 0 ) perror ( "Read failed!" );
	else if ( i == 0 ) perror ( "No data!" );
	else {
	  buf[i] = '\0';
	  snprintf ( retBuffer, i + 1, buf );
	  //printf (" %i Bytes: %s", i, buf );
	}

	return true;

}



int main( void ) {

	MYSQL *con;
	char sql[1000] = "\0";

	my_bool my_true = TRUE;
	con = mysql_init ( NULL );

	if ( con == NULL )
		fprintf ( stderr, "%s\n", mysql_error ( con ) );
	if ( mysql_options ( con, MYSQL_OPT_RECONNECT, &my_true ) )
		fprintf ( stderr, "MySQL_options failed: unknown MYSQL_OPT_RECONNECT." );
	if ( mysql_real_connect ( con, SQL_HOST, SQL_USER, SQL_PASS, SQL_DB, 0, NULL, 0 ) == 0 )
		fprintf ( stderr, "Connect to DB failed." );

    int fd;

	char sendBuffer[100];
	char retBuffer[101];
	char *p_retBuffer = retBuffer;
	char tmpVal[9];	// 8 Char + \0

	values_t values;

	time_t systime;

	for ( ; ; ) {
		systime = time( NULL );

		if ( systime  % 30 == 0 ) {

			fd = init_serial ( 300 );

			printf ( "Sending Request...\r\n" );
			sprintf ( sendBuffer, "/?!\r\n" );
			sendbytes ( fd, sendBuffer, 5 );

			printf ( "Identification: " );
			receiveBytes ( fd, p_retBuffer );
			printf ( "%s", retBuffer );
			printf ( "\r\n" );

			usleep ( 3000000 );	// 300msec

			printf ( "Sending ACK...   ");
			sprintf ( sendBuffer, "%c040\r\n", 0x06 );
			sendbytes ( fd, sendBuffer, strlen ( sendBuffer ) );

		//	close ( fd );
		//	fd = init_serial ( 4800 );
		//	printf ( "New connection with 4800 Baud\r\n" );

			receiveBytes ( fd, p_retBuffer );
			printf ( "Response: %s\r\n", retBuffer );

			//usleep(300000);	// 300msec

			while ( receiveBytes ( fd, p_retBuffer ) ) {
				if ( strncasecmp ( retBuffer, "0.0.0", 5) == 0 ) {
					snprintf ( tmpVal, 9, "%.*s", 8, retBuffer + 6 );
					printf ( "Electricity id: %i  rawdata: %s\n", atoi( tmpVal ), retBuffer );
					values.electricityID = atoi ( tmpVal );
				}
				if ( strncasecmp ( retBuffer, "0.1.0", 5) == 0 ) {
					snprintf ( tmpVal, 3, "%.*s", 2, retBuffer + 6 );
					printf ( "Billing period: %i  rawdata: %s\n", atoi( tmpVal ), retBuffer );
					values.billingPeriod = atoi ( tmpVal );
				}
				if ( strncasecmp ( retBuffer, "0.2.0", 5) == 0 ) {
					snprintf ( tmpVal, 9, "%.*s", 8, retBuffer + 6 );
					printf ( "Active firmware: %i  rawdata: %s\n", atoi( tmpVal ), retBuffer );
					values.activeFirmware = atoi ( tmpVal );
				}
				if ( strncasecmp ( retBuffer, "0.2.2", 5) == 0 ) {
					snprintf ( tmpVal, 9, "%.*s", 8, retBuffer + 6 );
					printf ( "Time switch prgm no.: %i  rawdata: %s\n", atoi( tmpVal ), retBuffer );
					values.timeSwitchPrgmNo = atoi ( tmpVal );
				}
				if ( strncasecmp ( retBuffer, "0.9.1", 5) == 0 ) {
					snprintf ( tmpVal, 8, "%.*s", 7, retBuffer + 6 );
					printf ( "Time: %i  rawdata: %s\n", atoi( tmpVal ), retBuffer );
					values.localTime = atoi ( tmpVal );
				}
				if ( strncasecmp ( retBuffer, "0.9.2", 5) == 0 ) {
					snprintf ( tmpVal, 8, "%.*s", 7, retBuffer + 6 );
					printf ( "Date: %i  rawdata: %s\n", atoi( tmpVal ), retBuffer );
					values.localDate = atoi ( tmpVal );
				}
				if ( strncasecmp ( retBuffer, "1.8.0", 5) == 0 ) {
					snprintf ( tmpVal, 9, "%.*s", 8, retBuffer + 6 );
					printf ( "Sum Power 1: %08.1f  rawdata: %s\n", atof( tmpVal ), retBuffer );
					values.sumPower1 = atof ( tmpVal );
				}
				if ( strncasecmp ( retBuffer, "1.8.1", 5) == 0 ) {
					snprintf ( tmpVal, 9, "%.*s", 8, retBuffer + 6 );
					printf ( "Sum Power 2: %08.1f  rawdata: %s\n", atof( tmpVal ), retBuffer );
					values.sumPower2 = atof ( tmpVal );
				}
				if ( strncasecmp ( retBuffer, "1.8.2", 5) == 0 ) {
					snprintf ( tmpVal, 9, "%.*s", 8, retBuffer + 6 );
					printf ( "Sum Power 3: %08.1f  rawdata: %s\n", atof( tmpVal ), retBuffer );
					values.sumPower3 = atof ( tmpVal );
				}
				if ( strncasecmp ( retBuffer, "1.25", 4) == 0 )  {
					snprintf ( tmpVal, 6, "%.*s", 5, retBuffer + 5 );
					printf ( "Power: %05.2f  rawdata: %s\n", atof( tmpVal ), retBuffer );
					values.power = atof ( tmpVal );
				}
				if ( strncasecmp ( retBuffer, "32.25", 5) == 0 ) {
					snprintf ( tmpVal, 6, "%.*s", 5, retBuffer + 6 );
					printf ( "U L1: %05.1f  rawdata: %s\n", atof( tmpVal ), retBuffer );
					values.U_L1 = atof ( tmpVal );
				}
				if ( strncasecmp ( retBuffer, "52.25", 5) == 0 ) {
					snprintf ( tmpVal, 6, "%.*s", 5, retBuffer + 6 );
					printf ( "U L2: %05.1f  rawdata: %s\n", atof( tmpVal ), retBuffer );
					values.U_L2 = atof ( tmpVal );
				}
				if ( strncasecmp ( retBuffer, "72.25", 5) == 0 ) {
					snprintf ( tmpVal, 6, "%.*s", 5, retBuffer + 6 );
					printf ( "U L3: %05.1f  rawdata: %s\n", atof( tmpVal ), retBuffer );
					values.U_L3 = atof ( tmpVal );
				}
				if ( strncasecmp ( retBuffer, "C.1.0", 5) == 0 ) {
					snprintf ( tmpVal, 9, "%.*s", 8, retBuffer + 6 );
					printf ( "Device No.: %i  rawdata: %s\n", atoi( tmpVal ), retBuffer );
					values.deviceNo = atoi ( tmpVal );
				}
				if ( strncasecmp ( retBuffer, "C.7.1", 5) == 0 ) {
					snprintf ( tmpVal, 5, "%.*s", 4, retBuffer + 6 );
					printf ( "Freq. L1: %li  rawdata: %s\n", strtol( tmpVal, NULL, 16 ), retBuffer );
					values.freqL1 = strtol( tmpVal, NULL, 16 );
				}
				if ( strncasecmp ( retBuffer, "C.7.2", 5) == 0 ) {
					snprintf ( tmpVal, 5, "%.*s", 4, retBuffer + 6 );
					printf ( "Freq. L2: %li  rawdata: %s\n", strtol( tmpVal, NULL, 16 ), retBuffer );
					values.freqL2 = strtol( tmpVal, NULL, 16 );
				}
				if ( strncasecmp ( retBuffer, "C.7.3", 5) == 0 ) {
					snprintf ( tmpVal, 5, "%.*s", 4, retBuffer + 6 );
					printf ( "Freq. L3: %li  rawdata: %s\n", strtol( tmpVal, NULL, 16 ), retBuffer );
					values.freqL3 = strtol( tmpVal, NULL, 16 );
				}

			}

			close ( fd );

			sprintf ( sql, "INSERT INTO D0 (dbTime, electricityID, billingPeriod, activeFirmware, timeSwitchPrgmNo, `localTime`, localDate"\
							", sumPower1, sumPower2, sumPower3, power, U_L1, U_L2, U_L3, deviceNo, freqL1, freqL2, freqL3) "\
							"VALUES (now(), %i, %i, %i, %i, %i, %i, %8.1f, %8.1f, %8.1f, %5.2f, %5.1f, %5.1f, %5.1f, %i, %i, %i, %i)"
							,values.electricityID
							, values.billingPeriod
							, values.activeFirmware
							, values.timeSwitchPrgmNo
							, values.localTime
							, values.localDate
							, values.sumPower1
							, values.sumPower2
							, values.sumPower3
							, values.power
							, values.U_L1
							, values.U_L2
							, values.U_L3
							, values.deviceNo
							, values.freqL1
							, values.freqL2
							, values.freqL3);
			if ( mysql_query ( con, sql ) ) {
				fprintf ( stderr, "Error at statement: %s\n", mysql_error ( con ) );
			}
		}
	}

    return true;
}


