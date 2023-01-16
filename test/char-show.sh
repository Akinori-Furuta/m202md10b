#!/bin/bash

export LANG=C

MyName="$0"
MyWhich=$(which "${MyName}")
MyPath=$(readlink -f "${MyWhich}")
MyDir=$(dirname "${MyPath}")
MyBase=$(basename "${MyPath}")
if which uuidgen
then
	Uuid=$(uuidgen)
else
	Uuid=$(uuid)
fi

function Help() {
	echo "$0: m202md10b show characters script."
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
${M202MD10B} "--path=${Port}" --baud=${Baud} --rtscts=${RtsCts} --break


PageTime=2
PageChars=16

c=0x20
while (( ${c} < 256))
do
	cs=${c}
	ce=$(( ${c} + ${PageChars}  - 1 ))
	printf "Code 0x%.2x - 0x%.2x" ${cs} ${ce} | ${M202MD10B} "--path=${Port}" --baud=${Baud} --rtscts=${RtsCts} -[ 0,0 -
	seq ${cs} ${ce} | gawk '{printf("%c", $1);}' | ${M202MD10B} -F "${Port}" -B ${Baud} -[ 0,1 -
	sleep ${PageTime}
	c=$(( ${c} + ${PageChars} ))
done
