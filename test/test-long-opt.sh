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
${M202MD10B} "--path=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" "--break"


# Hello world.
${M202MD10B} "--path=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" "Hello world."
${M202MD10B} "--dev=${Port}"  "--baud=${Baud}" "--rtscts=${RtsCts}" "--goto=0,1" "Test long options."
sleep 1

# Min bright
${M202MD10B} "--path=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" --goto-position=0,0 --bright=1 "Brightness minimum"
sleep 1

# level 2
${M202MD10B} "--dev=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" --goto=0,0 --bright=2 "Brightness level 2"
sleep 1

# level 3
${M202MD10B} "--dev=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" --goto-position=0,0 --bright=3 "Brightness level 3"
sleep 1

# Max bright
${M202MD10B} "--path=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" --goto=0,0 --bright=4 "Brightness maximum"
sleep 1

# Clear line                                                                              01234567890123456789
${M202MD10B} "--dev=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" --goto-position=0,0 "                    "
sleep 1

# Back cursor 0
${M202MD10B} "--path=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" "--goto=0,0" "Back cursor [ ]" --back --back=1 "0]"
sleep 1

# Back cursor 1
${M202MD10B} "--dev=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" --back=2 "1]"
sleep 1

# Back cursor 2
${M202MD10B} "--dev=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" --back=2 "2]"
sleep 1

# Show Reverse Cursor and Blink
${M202MD10B} "--path=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" --reverse --reset-blink-show-cursor --blink --goto=0,0 "                    "
${M202MD10B} "--path=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" "--goto=0,0" "Cursor runs."
# Use wait time @ one character
${M202MD10B} "--dev=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" "--goto=0,0" "--wait=100" --forward --forward=11
${M202MD10B} "--path=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" --wait=100 --back=12
${M202MD10B} "--path=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" --wait=100 --forward --forward=11
${M202MD10B} "--path=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" --wait=100 --back=6 --back=6
sleep 1
${M202MD10B} "--path=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" --goto=0,0 "            "

# Clear all
${M202MD10B} "--path=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" --hide --goto=0,0 "Clear all."
sleep 1
${M202MD10B} "--dev=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" --clear-all
sleep 1
${M202MD10B} "--dev=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" "Continue."
sleep 1

# Clear all and goto home.
${M202MD10B} "--dev=${Port}" --baud=${Baud} --rtscts=${RtsCts} --hide-cursor --goto=0,1 "Clear all and home."
sleep 1
${M202MD10B} "--path=${Port}" --baud=${Baud} --rtscts=${RtsCts} --clear-all-goto-home
sleep 1
${M202MD10B} "--port=${Port}" --baud=${Baud} --rtscts=${RtsCts} "Continue."
sleep 1

# Goto home.
${M202MD10B} "--dev=${Port}" --baud=${Baud} --rtscts=${RtsCts} --goto=0,1 "Goto Home."
sleep 1
${M202MD10B} "--path=${Port}" --baud=${Baud} --rtscts=${RtsCts} --goto-home "Hello world."
sleep 1
${M202MD10B} "--path=${Port}" --baud=${Baud} --rtscts=${RtsCts} --home "Good night world."
sleep 1

# Round trip mode.
${M202MD10B} "--path=${Port}" --baud=${Baud} --rtscts=${RtsCts} --goto=0,0 "Round trip mode 1" --round-trip-mode
sleep 1
i=0
while (( ${i} < 20))
do
	seq 0x41 0x5a | gawk '{printf("%c", $1);}' | ./m202md10b "--dev=${Port}" --baud=${Baud} --rtscts=${RtsCts} -
	i=$(( ${i} + 1 ))
done
sleep 1

# Scroll mode.
${M202MD10B} "--port=${Port}" --baud=${Baud} --rtscts=${RtsCts} --cls "Scroll mode 1" --scroll-mode
sleep 1

i=0
while (( ${i} < 20))
do
	seq 0x41 0x5a | gawk '{printf("%c", $1);}' | ./m202md10b "--port=${Port}" --baud=${Baud} --rtscts=${RtsCts} -
	i=$(( ${i} + 1 ))
done
sleep 1

# Round trip mode.
${M202MD10B} "--path=${Port}" --baud=${Baud} --rtscts=${RtsCts} --cls "Round trip mode 2" --round
sleep 1
i=0
while (( ${i} < 20))
do
	seq 0x61 0x7a | gawk '{printf("%c", $1);}' | ./m202md10b "--dev=${Port}" --baud=${Baud} --rtscts=${RtsCts} -
	i=$(( ${i} + 1 ))
done
sleep 1

# Scroll mode.
${M202MD10B} "--port=${Port}" --baud=${Baud} --rtscts=${RtsCts} --cls "Scroll mode 2" --scroll
sleep 1

i=0
while (( ${i} < 20))
do
	seq 0x61 0x7a | gawk '{printf("%c", $1);}' | ./m202md10b "--port=${Port}" --baud=${Baud} --rtscts=${RtsCts} -
	i=$(( ${i} + 1 ))
done
sleep 1


# Under line Cursor
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --clear-all-goto-home "Under line Cursor 1" --goto=0,1 "[X]" --back=2
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --under-line-cursor --reset-blink-show-cursor
sleep 2

# Under line Cursor, blink
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} "X] blink" --back=8
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --blink-cursor
sleep 2

# Block Cursor
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --clear-all-goto-home "Block Cursor 1" -[0,1 "[X]" --back=2
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --block-cursor --show
sleep 2

# Block Cursor, blink
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} "X] blink" --back=8
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --blink-cursor
sleep 2

# Reverse Cursor
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --clear-all-goto-home "Reverse Cursor 1" -[0,1 "[X]" --back=2
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --reverse-cursor --show
sleep 2

# Reverse Cursor, blink
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} "X] blink" --back=8
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --blink-cursor
sleep 2

# Hide Cursor
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --clear-all-goto-home "Hide Cursor 1" -[0,1 "[X]" --back=2
sleep 2
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --hide-cursor
sleep 2

# Under line Cursor
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --cls "Under line Cursor 2" --goto=0,1 "[X]" --back=2
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --under --show
sleep 2

# Under line Cursor, blink
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} "X] blink" --back=8
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --blink
sleep 2

# Block Cursor
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --cls "Block Cursor 2" -[0,1 "[X]" --back=2
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --block --show
sleep 2

# Block Cursor, blink
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} "X] blink" --back=8
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --blink
sleep 2

# Reverse Cursor
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --cls "Reverse Cursor 2" -[0,1 "[X]" --back=2
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --reverse --show
sleep 2

# Reverse Cursor, blink
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} "X] blink" --back=8
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --blink
sleep 2

# Hide Cursor
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --cls "Hide Cursor 2" -[0,1 "[X]" --back=2 --show
sleep 2
${M202MD10B} -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --hide
sleep 2


# Define Character font
./m202md10b -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --clear-all-goto-home "Define Character" --goto=0,1 "[" $'\x5c' "]"
sleep 1
./m202md10b -F "${Port}" --baud=${Baud} --rtscts=${RtsCts} --define=5c:00:01:02:04:08:10:00
sleep 1

i=0
while (( ${i} < 10 ))
do
	for c in  "|" "/" "-" $'\\'
	do
		echo -n "${c}" | ./m202md10b "--dev=${Port}" --baud=${Baud} --rtscts=${RtsCts} - --back
	done
	i=$(( ${i} + 1 ))
done

# Define Character font on live
./m202md10b "--port=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" --goto-position=0,0 --round
(
	i=0
	while (( ${i} < 40 ))
	do
		echo -n $'\xfd'
		i=$(( ${i} + 1 ))
	done
) | ./m202md10b "--dev=${Port}" "--baud=${Baud}" "--rtscts=${RtsCts}" -

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

	./m202md10b "--path=${Port}" --baud=${Baud} --rtscts=${RtsCts} --define-char=fd:04:0a:11:04:0a:11:04

#  01234
# 0 * * 
# 1*   *
# 2  *  
# 3 * * 
# 4*   *
# 5  *  
# 6 * * 

	./m202md10b "--dev=${Port}" --baud=${Baud} --rtscts=${RtsCts} --define=fd=0a-11-04-0a-11-04-0a

#  01234
# 0*   *
# 1  *  
# 2 * * 
# 3*   *
# 4  *  
# 5 * * 
# 6*   *

	./m202md10b "--port=${Port}" --baud=${Baud} --rtscts=${RtsCts} --define-char=fd11040a11040a11
	i=$(( ${i} + 1 ))
done

./m202md10b "--port=${Port}" --baud=${Baud} --rtscts=${RtsCts} --define=fd=04.0A.11.04.0A.11.04

sleep 2

# Send break signal.
${M202MD10B} "--dev=${Port}" --baud=${Baud} --rtscts=${RtsCts} --hard-reset
