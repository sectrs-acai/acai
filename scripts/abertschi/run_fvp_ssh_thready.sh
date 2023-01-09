#!/usr/bin/env bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
source $(git rev-parse --show-toplevel)/env.sh

ssh -X bean@192.33.93.250 /home/bean/trusted-periph/scripts/run_fvp.sh
