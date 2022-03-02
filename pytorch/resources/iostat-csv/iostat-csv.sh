#!/bin/sh
## iostat csv generator ##
# This script converts iostat output every second in one line and generates a csv file.

# Settings
COMMAND_OUTPUT=`iostat -t -x`
CONCAT_LINE_OFFSET=3
TITLE_LINE_OFFSET=6
TITLE_LINE_HEADER="Date Time"


# Count command output line number
COMMAND_OUTPUT_LINE_NUMBER=`wc -l <<EOF
${COMMAND_OUTPUT}
EOF`

# Calcurate how many lines to concatenate in each second
CONCAT_LINE_NUMBER=`expr ${COMMAND_OUTPUT_LINE_NUMBER} - ${CONCAT_LINE_OFFSET}`

# Print "N;" for ${CONCAT_LINE_NUBER} times to concatenate output lines by using sed command
CONCAT_LINE_N=`seq -s'N;' ${CONCAT_LINE_NUMBER} | tr -d '[:digit:]'`

# Generate title line for csv
TITLE_AVG_CPU=`grep avg-cpu <<EOF
${COMMAND_OUTPUT}
EOF`
TITLE_EACH_DEVICE=`grep "Device" <<EOF
${COMMAND_OUTPUT}
EOF`
DEVICE_LINE_NUMBER=`expr ${COMMAND_OUTPUT_LINE_NUMBER} - ${TITLE_LINE_OFFSET}`
TITLE_DEVICES=`seq -s"${TITLE_EACH_DEVICE} " ${DEVICE_LINE_NUMBER} | tr -d '[:digit:]'`

echo "${TITLE_LINE_HEADER} ${TITLE_AVG_CPU} ${TITLE_DEVICES}" \
 | awk 'BEGIN {OFS=","} {$1=$1;print $0}' | sed 's/avg-cpu//g;s/://g;s/,,/,/g'


# Main part
LANG=C; iostat -t -x 1 | grep --line-buffered -v -e avg-cpu -e Device -e Linux \
 | sed --unbuffered "${CONCAT_LINE_N}s/\n/,/g;s/\s\s*/,/g;s/,,*/,/g;s/^,//g"

# grep -v -e avg-cpu -e Device -e Linux
#  => Exclude title columns

# sed --unbuffered
#  '${CONCAT_LINE_N}s/\n/,/g'
#    => Read ${CONCAT_LINE_NUMBER} lines and replace newline characters to ","

#  's/\s\s*/,/g'
#    => Replace adjacent blank symbols for ","

#  's/,,*/,/g'
#    => Replace adjacent commas for single comma

#  's/^,//g'
#    => Remove comma symbol placed at the beginning of the line
