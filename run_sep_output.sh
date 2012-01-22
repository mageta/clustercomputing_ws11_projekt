#!/bin/bash

#$* 1>/dev/null 2> mpi_output.${LAMRANK}
$* $> mpi_output.${LAMRANK}

exit 0
