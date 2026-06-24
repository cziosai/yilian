#!/bin/bash
sudo cp yilianvpn.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl start yilianvpn.service
sudo systemctl enable yilianvpn.service
