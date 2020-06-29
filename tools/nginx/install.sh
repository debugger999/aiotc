#!/bin/sh

sudo cp -rfa nginx/ /usr/local/
sudo chown -R ubuntu:ubuntu /usr/local/nginx
mkdir -p /home/ubuntu/webdir/video
mkdir -p /home/ubuntu/webdir/webpages
sudo chown -R ubuntu:ubuntu /home/ubuntu/webdir

