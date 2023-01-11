#!/bin/bash

MyName="$0"
MyWhich=$(which "${MyName}")
MyPath=$(readlink -f "${MyWhich}")
MyDir=$(dirname "${MyPath}")
MyBase=$(basename "${MyPath}")
Uuid=$(uuidgen)


export LANG=C

function Help() {
	echo "$0: m202md10b test script."
	echo "Command line: -F SerialPortPath [-B baud] [-C {y|n}]  [-h]"
	echo "-F SerialPortPath  Device path to serial port."
	echo "-B baud            Set baud ratio (2400)."
	echo "-C {y|n}           Set RTS-CTS flow control, y: enable, n: disable (n)."
	echo "-h                 Print help."
}

M202MD10B="${MyDir}/../m202md10b"

Port=""
Baud=2400
RtsCts="n"
ExitCode=0

while getopts "F:B:C:h" opt
do
	case ${opt} in
	(F)
		Port="${OPTARG}"
	;;
	(B)
		Baud=${OPTARG}
	;;
	(C)
		RtsCts=${OPTARG}
	;;
	(h | ?)
		Help
		exit 1
	;;
	esac
done

if [ -z "${Port}" ]
then
	echo "$0: Specify port path with -F path."
	ExitCode=1
fi

if (( ${ExitCode} != 0 ))
then
	exit ${ExitCode}
fi

# Send break signal.
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -R


# Hello world.
${M202MD10B} "-F=${Port}" "-B${Baud}" -C ${RtsCts} "Hello world."
${M202MD10B} "-F${Port}"  "-B=${Baud}" -C ${RtsCts} -[ 0,1 "Test short options."
sleep 1

# Min bright
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -[0,0 -d "Brightness minimum"
sleep 1

# level 2
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -[0,0 -c "Brightness level 2"
sleep 1

# level 3
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -[0,0 -b "Brightness level 3"
sleep 1

# Max bright
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -[0,0 -a "Brightness maximum"
sleep 1

# Clear line                                             01234567890123456789
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -[0,0 "                    "
sleep 1

# Back cursor 0
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -[0,0 "Back cursor [ ]" -h -h "0]"
sleep 1

# Back cursor 1
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -h2 "1]"
sleep 1

# Back cursor 2
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -h=2 "2]"
sleep 1

# Show Reverse Cursor and Blink
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -x -s -u -[0,0 "                    "
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -[0,0 "Cursor runs."
# Use wait time @ one character
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -[0,0 -W=100 -i -i=11
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -W 100 -h=12
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -W=100 -i -i11
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -W100 -h=6 -h6
sleep 1
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -[0,0 "            "

# Clear all
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -t -[0,0 "Clear all."
sleep 1
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -j
sleep 1
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} "Continue."
sleep 1

# Clear all and goto home.
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -t -[0,1 "Clear all and home."
sleep 1
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -l
sleep 1
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} "Continue."
sleep 1

# Goto home.
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -[0,1 "Goto Home."
sleep 1
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -m "Hello world."
sleep 1
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -m "Good night world."
sleep 1

# Round trip mode.
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -[0,0 "Round trip mode." -q
sleep 1
i=0
while (( ${i} < 20))
do
	seq 0x41 0x5a | gawk '{printf("%c", $1);}' | ./m202md10b -F ${Port} -B ${Baud} -C ${RtsCts} -
	i=$(( ${i} + 1 ))
done
sleep 1

# Scroll mode.
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -l "Scroll mode." -r
sleep 1

i=0
while (( ${i} < 20))
do
	seq 0x41 0x5a | gawk '{printf("%c", $1);}' | ./m202md10b -F ${Port} -B ${Baud} -C ${RtsCts} -
	i=$(( ${i} + 1 ))
done
sleep 1

# Under line Cursor
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -l "Under line Cursor" -[0,1 "[X]" -h2
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -v -s
sleep 2

# Under line Cursor, blink
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} "X] blink" -h8
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -u
sleep 2

# Block Cursor
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -l "Block Cursor" -[0,1 "[X]" -h2
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -w -s
sleep 2

# Block Cursor, blink
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} "X] blink" -h8
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -u
sleep 2

# Reverse Cursor
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -l "Reverse Cursor" -[0,1 "[X]" -h2
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -x -s
sleep 2

# Reverse Cursor, blink
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} "X] blink" -h8
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -u
sleep 2

# Hide Cursor
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -l "Hide Cursor" -[0,1 "[X]" -h2
sleep 2
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -t
sleep 2

# Define Character font
./m202md10b -F "${Port}" -B ${Baud} -C ${RtsCts} -l "Define Character" -[0,1 "[" $'\x5c' "]"
sleep 1
./m202md10b -F "${Port}" -B ${Baud} -C ${RtsCts} -z 5c:00:01:02:04:08:10:00
sleep 1

i=0
while (( ${i} < 10 ))
do
	for c in  "|" "/" "-" $'\\'
	do
		echo -n "${c}" | ./m202md10b -F "${Port}" -B ${Baud} -C ${RtsCts} - -h
	done
	i=$(( ${i} + 1 ))
done

# Define Character font on live
./m202md10b -F "${Port}" -B ${Baud} -C ${RtsCts} -[0,0 -q
(
	i=0
	while (( ${i} < 40 ))
	do
		echo -n $'\xfd'
		i=$(( ${i} + 1 ))
	done
) | ./m202md10b -F "${Port}" -B ${Baud} -C ${RtsCts} -

i=0
while (( ${i} < 10 ))
do
#  01234
# 0  *  
# 1 * * 
# 2*   *
# 3  *  
# 4 * * 
# 5*   *
# 6  *  

	./m202md10b -F "${Port}" -B ${Baud} -C ${RtsCts} -z fd:04:0a:11:04:0a:11:04

#  01234
# 0 * * 
# 1*   *
# 2  *  
# 3 * * 
# 4*   *
# 5  *  
# 6 * * 

	./m202md10b -F "${Port}" -B ${Baud} -C ${RtsCts} -z fd=0a-11-04-0a-11-04-0a

#  01234
# 0*   *
# 1  *  
# 2 * * 
# 3*   *
# 4  *  
# 5 * * 
# 6*   *

	./m202md10b -F "${Port}" -B ${Baud} -C ${RtsCts} -z fd11040a11040a11
	i=$(( ${i} + 1 ))
done

./m202md10b -F "${Port}" -B ${Baud} -C ${RtsCts} -zfd=04.0A.11.04.0A.11.04

sleep 2

# Send break signal.
${M202MD10B} -F "${Port}" -B ${Baud} -C ${RtsCts} -R
