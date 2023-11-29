# sBITX-log2adi
C program to query sBITX log db and export data in an ADIF format file for use in another logger, upload to LOTW ...
promts user for start and end QSO IDs to process
 
written by Bob Benedict, KD8CGH, November 2023 

 released under creative commons license BY
 This license enables reusers to distribute, remix, adapt, and build upon the material in any medium or format, 
 so long as attribution is given to the creator. The license allows for commercial use. 
 CC BY includes the following elements:
 BY: credit must be given to the creator.
 
compile with sqlite3 library
   gcc -Wall -o "log2adi" "log2adi.c" -lsqlite3 
    yes - you get warnings you can ignore

 execute by tying ./log2adi from terminal
 creates and fills export.adi file and echos exports to terminal
 
 note: I made some guesses in the db to ADIF mapping
   exch_sent -> STX_String  transmit exchange information
   exch_recv -> STR_String  receive exchange information
   tx_id -> STX  transmit serial number or exchange
also arbitrary 90 character limit on fields including comments
