#!/bin/bash

cp lc3bsim test/lc3bsim
cp ucode test/ucode

cd test

# ---------------------------------------------------------------
#  ----- If test fails, comment others and run only that test
#  ----- If you get "Dumpsim not found" ignore the 100/100 score
# ---------------------------------------------------------------


arr=(1	ADD 	5
2	AND 	5
4	NOT 	5
5	ADD 	5
7	ADD 	5
8	AND 	5
9	XOR 	5
10	LSHF  	5
11	RSHFL 	5
12	RSHFA 	5
13	RSHFA 	5
14	STB   	5 
15	STB   	5 
16	STW   	5 
17	LDB   	5
18	LDB   	5
19	LDW   	5
20	LEA   	5 
21	JMP   	6
24	JMP   	9
25	JSRR  	6
26	JSRR  	7
27	JSRR  	9
29	RET	5
30	TRAPx25	2
dep11	dep.stall	4
dep12	dep.stall	7
dep14	dep.stall	11
br11	br.stall	4
br12	br.stall	7
br21	br.stall	6
br23	br.stall	9
br31	br.stall	4
br33	br.stall	10
ist1	icache.r	13 
ist2	icache.r	14
istb2	icache_br	14
mst1	mem.stall	9
mst2	mem.stall	10
mstd1	mem.stall_dep	9
mstd2	mem.stall_dep	10
loop1	simple_loop	25
mem1	stores_loads	55
mem2	stores_loads	103
mem3	memory_jsr	28
sub1	subroutine	46
mem4	stores	55
mem5	stores	60
log2	misc	82
swap	misc	110
)
score=100
score_per=2
for ((i = 0; i < ${#arr[@]}; i = i + 3))
do
	printf "Running test ${arr[i]}.hex -- "
	./run ./lc3bsim ./ucode state_data_in/${arr[i]}.hex ${arr[i+2]} > /dev/null 2>&1
	diff dumpsim state_data_out/${arr[i]}.dump > out
	if [ -s ./out ]
	then
		echo "failed"
		score=$(echo "${score}-${score_per}"|bc)
	else
		echo
	fi
done

echo "Your score is $score out of 100"

