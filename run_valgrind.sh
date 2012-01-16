#!/bin/bash

# valgrind -v --leak-check=full --track-origins=yes --leak-resolution=high $* &> mpi_output.${LAMRANK}
# valgrind --tool=exp-sgcheck $* &> mpi_output.${LAMRANK}

if [[ "$LAMRANK" == "0" ]]; then
	ddd $* &> mpi_output.${LAMRANK}
else
	$* &> mpi_output.${LAMRANK}
fi

exit 0
