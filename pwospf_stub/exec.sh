#!/bin/bash

if [ "$1" == 1 ]
then
#echo "Running the command ./sr -t 319"
./sr -t 113 -v vhost1 -r rtable.vhost1
fi

if [ "$1" == 2 ]
then 
#echo "Running the command ./sr -t 319 -l logfile"
./sr -t 113 -v vhost2 -r rtable.vhost2
fi

if [ "$1" == 3 ]
then
#echo "Running the command tcpdump tcpdump -r logfile -e -vvv -xx"
./sr -t 113 -v vhost3 -r rtable.vhost3
fi
