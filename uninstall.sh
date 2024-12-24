#!/bin/bash

# Exit on errors
set -e

USER_NAME=$(whoami)
COMPUTER_NAME=$(hostname)

INSTALL_DIR="/usr/local/bin"
SERVICE_FILE="/etc/systemd/system/winmgm.service"

# Function to remove the binary
remove_binary() {
  echo "Removing winmgm binary from $INSTALL_DIR..."
  sudo rm -f "$INSTALL_DIR/winmgm"
}

# Function to remove the systemd service
remove_service() {
  echo "Removing systemd service for winmgm..."

  # Stop and disable the service before removing it
  sudo systemctl stop winmgm.service
  sudo systemctl disable winmgm.service

  # Remove the service file
  sudo rm -f "$SERVICE_FILE"

  # Reload systemd to apply the changes
  sudo systemctl daemon-reload
}

# Main uninstallation process
echo "Starting uninstallation of winmgm..."

remove_binary
remove_service

echo "Uninstallation complete! winmgm has been removed from your system."
