#!/bin/bash
echo "alias launch=\"sh ./launcher.sh\"" >> ~/.bashrc
echo "alias clean=\"sh ./cleanup.sh\"" >> ~/.bashrc
echo "alias show=\"cat ./dump_*\"" >> ~/.bashrc

source ~/.bashrc

