     .ORIG x1234
     ADD R1, R1, R1
     STB R7, R6 ,#0
     STB R7, R6 ,#0
     STB R7, R6 ,#0
     BRp DONE
     ADD R2, R2, R2
DONE ADD R1, R1, R1
     .END
