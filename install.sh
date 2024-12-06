#!/bin/bash

# Update package list and install necessary packages
sudo apt-get update
sudo apt-get install -y build-essential libusb-1.0-0-dev libfftw3-dev

# Clone the PocketSDR repository
git clone https://github.com/tomojitakasu/PocketSDR

# Define the installation directory
INSTALL_DIR=$(pwd)/PocketSDR

# Move to the library directory
cd $INSTALL_DIR/lib

# Make the clone_lib.sh script executable and run it
chmod +x clone_lib.sh
./clone_lib.sh

# Move to the library build directory and build the libraries
cd $INSTALL_DIR/lib/build
make
sudo make install

# Move to the application program directory and build the utilities and APs
cd $INSTALL_DIR/app
make
sudo make install

echo "PocketSDR has been successfully installed and built."
