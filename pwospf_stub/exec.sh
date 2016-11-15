#!/bin/bash

if [ "$1" == 1 ]
then
./sr -t 113 -v vhost1 -r rtable.net -l logfile1
fi

if [ "$1" == 2 ]
then 
./sr -t 113 -v vhost2 -r rtable.empty -l logfile2
fi

if [ "$1" == 3 ]
then
./sr -t 113 -v vhost3 -r rtable.empty -l logfile3
fi

if [ "$1" == "logfile1" ]
then
tcpdump -r logfile1 -e -vvv -xx
fi

if [ "$1" == "logfile2" ]
then
tcpdump -r logfile2 -e -vvv -xx
fi

if [ "$1" == "logfile3" ]
then
tcpdump -r logfile3 -e -vvv -xx
fi

