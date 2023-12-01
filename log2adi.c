/*
* C program to query sBITX log db and export data in an ADIF format file for use in another logger, upload to LOTW ...
* promts user for start and end QSO IDs to process
* 
* written by Bob Benedict, KD8CGH, November 2023 
* 
* version 1.0
* 
* released under creative commons license BY
* This license enables reusers to distribute, remix, adapt, and build upon the material in any medium or format, 
* so long as attribution is given to the creator. The license allows for commercial use. 
* CC BY includes the following elements:
* BY: credit must be given to the creator.
* 
*  compile with sqlite3 library
* gcc -Wall -o "log2adi" "log2adi.c" -lsqlite3 
* execute by tying ./log2adi from terminal
* creates and fills export.adi file and echos exports to terminal
*
* note: I made some guesses in the db to ADIF mapping
*   exch_sent -> STX_String  transmit exchange information
*   exch_recv -> STR_String  receive exchange information
*   tx_id -> STX  transmit serial number or exchange
* also arbitrary 90 character limit on fields including comments
* 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h> 

// ADIF field headers, see note above
char names[13][12]={"ID","MODE","FREQ","QSO_DATE","TIME_ON","OPERATOR","RST_SENT","STX_String","CALL","RST_RCVD","SRX_String","STX","COMMENTS"};

// Function to assign band names
// although the sBITX knows the band at time of QSO, it chooses to forget it
// so we must recover the band from the frequency
char bands[9][5] = {"80M", "60M", "40M", "30M", "20M", "17M", "15M", "12M", "10M"};

void bandNames(char *fstring, char *band) {  // assign band names to string for export
	
	float freq=0;
	int ifreq=0;
		
	freq=atof(fstring);
	ifreq=freq/1000; // truncate to get MHz 
	
	switch(ifreq) {  // switch on frequency as int in MHz
		
		case 3:  // 80M
			strcpy(band,bands[0]);
			break;
			
		case 5:  // 60M
			strcpy(band,bands[1]);
			break;
			
		case 7:  // 40M
			strcpy(band,bands[2]);
			break;
			
		case 10:  // 30M
			strcpy(band,bands[3]);
			break;
			
		case 14:  // 20M
			strcpy(band,bands[4]);
			break;
			
		case 18:  //17M
			strcpy(band,bands[5]);
			break;
			
		case 21:  // 15M
			strcpy(band,bands[6]);
			break;
			
		case 24:  // 12M
			strcpy(band,bands[7]);
			break;			

		case 28:  // 10M
			strcpy(band,bands[8]);
			break;			
		
	default:  // handle band not found
		strcpy(band,"ERR");
		printf(" error \n");	
		
	}	
}

void removeAll(char * str, const char toRemove) // remove all char from string
{
    int i, j;
    int len = strlen(str);

    for(i=0; i<len; i++)
    {
/* If the character to remove is found then shift all characters to one
 * place left and decrement the length of string by 1.
 */
        if(str[i] == toRemove)
        {
            for(j=i; j<len; j++)
            {
                str[j] = str[j+1];
            }

            len--;
            i--;
        }
    }
}

// called by sqlite3 for each row selected in db
static int callback(void *export, int argc, char **argv, char **azColName){
	int i;
	int lenadd;
	char add[91];	
	char band[5];
	char rm='-';
	char rm1=' ';
	char end[3]="00";
	float freq;
	
// loop over columns, start at i=1 to skip ID number   
	for(i = 1; i<argc; i++){  
	  sprintf(add, "%s ", argv[i]); // write to add buff
	  lenadd=strlen(add);
	  if (lenadd > 1) {   // not null case
		  if (i==2) { // at frequency, assign band and write out
			bandNames(add, band); // write band, add has freq
			fprintf(export,"<BAND:%d>%s ", strlen(band), band); 
			// now do freq
			freq=atof(add)/1000.0;  // convert kHz to MHz
			sprintf(add,"%.3f",freq); // write out with 3 decimal digits
			// note db stores freq as kHz so no more than 3 valid
			lenadd=strlen(add);
		  }
		  if (i==3) {
			removeAll(add, rm);  // reformat date, remove "-"
			lenadd=strlen(add);
		  }
		  if (i==4) {
			removeAll(add, rm1); // remove trailing space
			strcat(add,end);  // reformat time to add 00 seconds
			lenadd=strlen(add);
		  }
	     printf("<%s:%d>%s ", names[i], lenadd-1, add);
	     fprintf(export,"<%s:%d>%s ", names[i], lenadd-1, add);

		}
	}  

	fprintf(export,"<EOR>\n");
	printf("\n");
	return 0;
}

int main(int argc, char* argv[]) {
	sqlite3 *db;
	char sqlstr[60] = {'\0'};
	char *zErrMsg = 0;
	int rc;
	char *sql;
	int startID=0;
	int endID=0;
	int startScanned =0;
	int endScanned=0;
	
// Open database 
   rc = sqlite3_open("/home/pi/sbitx/data/sbitx.db", &db);
   
   if( rc ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(0);
   } else {
      fprintf(stderr, "Opened database successfully\n");
   }

// Prompt user for start ID (with validation)
	do {
		printf("Enter start ID number: ");
		startScanned=scanf("%d", &startID);
		while(getchar() != '\n');
		if (startScanned != 1) {
			printf("Invalid ID. Please enter the ID number\n");
			}
		} while (startScanned != 1);

// Prompt user for end ID (with validation)
	do {
		printf("Enter end ID: ");
		endScanned=scanf("%d", &endID);
		while(getchar() != '\n');
		if (endScanned != 1) {
			printf("Invalid ID. Please enter the ID number\n");
		} else if (endID < startID) {
			printf("End ID must be equal to or after the start ID.\n");
		}
	} while ((endScanned != 1) || (endID < startID));

	endID++; // include endID
 
 // open output file  
   	FILE *export = fopen("export.adi", "w");
	if (!export) { fprintf(stderr, "Can't open output file.\n");
	return 1;
	} 
	 
// write ADIF file header	
	fprintf(export, "ADIF file\n");
	fprintf(export, "generated from sBITX log db by Log2ADIF program\n");	
	fprintf(export, "<adif version:5>3.1.4\n");	
	fprintf(export, "<EOH>\n");	

// Create SQL statement 
   sprintf(sqlstr, "SELECT * from logbook WHERE ID BETWEEN %d AND %d", startID, endID);
   sql=sqlstr;
   
// Execute SQL statement 
   rc = sqlite3_exec(db, sql, callback, (void*)export, &zErrMsg);
   
   if( rc != SQLITE_OK ) {
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   } else {
      fprintf(stdout, "Operation done successfully\n");
   }
   
   sqlite3_close(db);  // close it down
   fclose(export);
   return 0;
   
}
