#!/bin/bash
echo "Testing moddymod!"

make

sudo insmod moddymod.ko
sudo insmod moddymod2.ko

sudo gcc modtester.c
sudo ./a.out

sudo rmmod moddymod2
sudo rmmod moddymod
