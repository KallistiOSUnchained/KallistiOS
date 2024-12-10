#!/bin/bash

# Source the environment setup script
if [ -f "/opt/toolchains/dc/kos/environ.sh.master" ]; then
    source /opt/toolchains/dc/kos/environ.sh.master
fi

# Execute the provided command or fall back to bash
if [ "$#" -eq 0 ]; then
    # No command provided, start an interactive shell
    exec /bin/bash
else
    # Execute the provided command
    exec "$@"
fi
