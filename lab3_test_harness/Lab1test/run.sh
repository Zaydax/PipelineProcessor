#!/bin/bash

cp A test/lc3bsim
cp ucode test/ucode
cd test

# -----------------------------------------------
# -----------------------------------------------
# -----------------------------------------------
#  Part A: Testing single instruction at a time
# -----------------------------------------------
# -----------------------------------------------
# -----------------------------------------------

arr=(1 1
2 6
3 7
4 8
5 9)
score=100
score_per=20
for ((i = 0; i < ${#arr[@]}; i = i + 2))
do
	printf "Running test ${arr[i]}.hex -- "
	./run ./lc3bsim ./ucode state_data_in/${arr[i]}.hex ${arr[i+1]} > /dev/null 2>&1
	diff dumpsim state_data_out/${arr[i]}.dump > out
	if [ -s ./out ]
	then
		echo "failed"
		score=$(echo "scale=6; ${score}-${score_per}"|bc)
	else
		echo
	fi
done

echo "Your score is $score out of 100"
