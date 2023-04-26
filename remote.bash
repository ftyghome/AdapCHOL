#!/bin/bash

REMOTE_MACHINE=b3351-alder

rsync -ar pl Makefile -e ssh liuzesen@$REMOTE_MACHINE:adapchol > /dev/null
ssh liuzesen@$REMOTE_MACHINE "cd /home/liuzesen/adapchol && make $1"
rsync -ar -e ssh liuzesen@$REMOTE_MACHINE:adapchol/build/pl ./build > /dev/null
rsync -ar -e ssh liuzesen@$REMOTE_MACHINE:adapchol/pl/vitis_prj ./pl > /dev/null