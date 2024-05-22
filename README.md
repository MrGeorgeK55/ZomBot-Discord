# Project Zomboid Discord Bot

A Discord bot written in C++ designed to monitor a Project Zomboid server with various functionalities accessible via slash commands.
it uses Rcon Protocol to get all the data related. Feel free to modify it as you want  

"I love creating unnecessary and over-complicated tools that only used a handful of times, maybe less than that..." 
## Features

### Dice
- Roll 1d100 and face the consequences or your luck inside the Zomboid server.

### ServerStatus
- Get detailed information about the Project Zomboid server.

### Check Mods Updates
- Check for updates on installed mods.

### Map
- Display the world map of the Project Zomboid server.

### Access
- Request access to the server to the admins.

### Two-Factor Authentication (TFA)
- Generate a 2FA code valid for 2 minutes for administrators to manually restart the bot or the server.

### Reset Bot
- Administrators can trigger a bot reset by entering a 6-digit code.

### Reset Server
- Administrators can trigger a server reset by entering a 6-digit code.

## Usage

To use the bot, simply type the corresponding slash command followed by any additional parameters if required.

- `/dice`: Roll dice(1d100) and get the consequences in the server. (I still working to request the username and set a cooldown time to use this command)
- `/serverstatus`: Get server status.
- `/checkmods`: Check for mod updates.
- `/map`: Display the server's world map.
- `/access`: Request server access.
- `/tfa`: Generate a 2FA code.
- `/resetbot [6-digit code]`: Reset the bot (admin only).
- `/resetserver [6-digit code]`: Reset the server (admin only).
- `/tengolag`: Shows the users how to configure Project Zomboid to use more RAM.
- `/players`: How many players are currently connected to the server

## Installation

1. Clone the repository.
2. Install the required Libraries*.
3. Set up environment variables for your Discord bot token and other necessary configurations.
4. Compile the bot using g++.
5. Execute the bot.


## Libraries used:

[RCON++](https://github.com/Jaskowicz1/rconpp)
[Telegram C++](https://github.com/reo7sp/tgbot-cpp/tree/master)
[LibConfig++](https://github.com/hyperrealm/libconfig)
[D++](https://github.com/brainboxdotcc/DPP)
