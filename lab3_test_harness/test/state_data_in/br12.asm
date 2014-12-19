     .ORIG x1234
     ADD R0, R0, R0
     AND R1, R1, #0
     BRz DONE
     ADD R2, R2, R2
DONE ADD R1, R1, R1
     .END
