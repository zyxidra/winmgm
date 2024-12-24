#!/bin/bash

# Exit on errors
set -e

# Detect user and computer name
USER_NAME=$(whoami)
COMPUTER_NAME=$(hostname)

INSTALL_DIR="/usr/local/bin"
SERVICE_FILE="/etc/systemd/system/winmgm.service"

# Function to check and install dependencies
install_dependencies() {
  echo "Updating package lists..."

  # List of dependencies
  local dependencies="libx11-dev cmake gcc make"

  for dep in $dependencies; do
    if dpkg-query -l "$dep" &>/dev/null; then
      echo "$dep is already installed."
    else
      echo "Installing $dep..."
      sudo apt install -y "$dep"
    fi
  done
}

# Function to build and install the project
build_and_install() {
  echo "Building the project..."
  cmake .
  make

  echo "Installing winmgm to $INSTALL_DIR..."
  sudo cp winmgm "$INSTALL_DIR/"
  sudo chmod +x "$INSTALL_DIR/winmgm"
}

# Function to create and enable the systemd service
create_systemd_service() {
  echo "Creating systemd service for winmgm..."

  # Check if service already exists and delete it
  if [ -f "$SERVICE_FILE" ]; then
    echo "Service already exists. Removing old service..."
    sudo systemctl stop winmgm.service
    sudo systemctl disable winmgm.service
    sudo rm "$SERVICE_FILE"
  fi

  # Create new service file
  sudo bash -c "cat > $SERVICE_FILE" <<EOL
[Unit]
Description=winmgm window manager
After=graphical.target

[Service]
ExecStart=/usr/local/bin/winmgm
User=$USER_NAME
Environment=DISPLAY=:1
Environment=XDG_SESSION_TYPE=$XDG_SESSION_TYPE   # Pass the session type environment variable
Restart=always
RestartSec=3

[Install]
WantedBy=default.target
EOL

  # Reload systemd to apply changes
  sudo systemctl daemon-reload
  sudo systemctl enable winmgm.service
  sudo systemctl start winmgm.service
}

# Main installation process
echo "Starting installation of winmgm..."

install_dependencies
build_and_install
create_systemd_service

echo "Installation complete! winmgm is set to run at login."
