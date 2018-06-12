#!/bin/bash
ping -c 5 server 
apt-get install -y telnet
telnet server 4097
