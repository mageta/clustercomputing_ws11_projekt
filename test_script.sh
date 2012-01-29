#!/bin/bash

PATTERN="patternx.txt"
PATTERN_SIZE=1024

function run_pixelgenerator() {
	OUTFILE=".comp.1.txt"

	if [ ${PATTERN_SIZE} -lt 21 ]; then
		echo "pattern_size to small" &>/dev/stderr
		return 1
	fi

	PMOD=$((${PATTERN_SIZE} - 20))

	DIM_M=`expr $(expr $(head -c 200 /dev/urandom | tr -dc '[:digit:]' | sed 's/^0*//') % ${PMOD}) + 20`
	if [[ -z "${DIM_M}" ]]; then
		DIM_M=2;
	fi

	DIM_N=`expr $(expr $(head -c 200 /dev/urandom | tr -dc '[:digit:]' | sed 's/^0*//') % ${PMOD}) + 20`
	if [[ -z "${DIM_N}" ]]; then
		DIM_N=2;
	fi

	echo pixelpattern ${DIM_M} ${DIM_N}
	time ./pixelpattern ${DIM_M} ${DIM_N} 2>"${PATTERN}" 1>.comp.1.txt || return 1
	grep '^[0-9]' "${OUTFILE}" > "${OUTFILE}.tmp" || return 1
	mv "${OUTFILE}.tmp" "${OUTFILE}" || return 1
	sort -fn "${OUTFILE}" > "${OUTFILE}.tmp" || return 1
	mv "${OUTFILE}.tmp" "${OUTFILE}" || return 1
	echo "components: `wc -l .comp.1.txt`"
}

function run_findcomp() {
	INFILE="${1}"
	OUTFILE=".comp.${2}.txt"

	echo "find_components '${INFILE}' '${OUTFILE}'"
	time mpirun -np "${2}" ./find_components "${INFILE}" 2>/dev/null 1>"${OUTFILE}" || return 1
	cut -d ':' -f3 "${OUTFILE}" | sed 's/[ ]*//' > "${OUTFILE}.tmp" || return 1
	mv "${OUTFILE}.tmp" "${OUTFILE}" || return 1
	sort -fn "${OUTFILE}" > "${OUTFILE}.tmp" || return 1
	mv "${OUTFILE}.tmp" "${OUTFILE}" || return 1
}

CPUS="6"

run_pixelgenerator || { echo "pixelpattern failed"; exit 1; }
run_findcomp "${PATTERN}" "${CPUS}" || { echo "find_components failed"; exit 1; }

# run_findcomp "pattern_bigc_8192.txt" "1" || { echo "find_components failed"; exit 1; }
# run_findcomp "pattern_bigc_8192.txt" "${CPUS}" || { echo "find_components failed"; exit 1; }

if ! (diff .comp.1.txt .comp.${CPUS}.txt &>/dev/null)
then
	echo "the components doesn't match! something bad happened" &>/dev/stderr
	exit 1
fi

# rm -f .comp.1.txt &>/dev/null
# rm -f .comp."${CPUS}".txt &>/dev/null

echo "finished successful"

exit 0
