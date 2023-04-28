#!/usr/bin/env bash

ssh -L 59000:localhost:5901 -C -N -l armcca 192.33.93.245
