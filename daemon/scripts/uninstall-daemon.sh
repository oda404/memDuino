#!/bin/bash

sudo systemctl stop memduinod
sudo systemctl disable memduinod
sudo rm -f /usr/lib/systemd/system/memduinod.service /usr/bin/memduinod
sudo rm -rf /etc/memduino
