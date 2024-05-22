#include <iostream>
#include <curl/curl.h>
#include <libconfig.h++>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <string>
#include <sstream>
#include <regex>
#include <vector>
#include <rconpp/rcon.h>
#include <dpp/dpp.h>
#include <random>
#include <tgbot/tgbot.h>

// GeoVersion 1.0.1
// g++ zombot.cpp -o zombot -I/usr/local/include -L/usr/local/lib -lTgBot -lboost_system -lssl -lcrypto -lpthread -lcurl -lrconpp -lconfig++ -ldpp

// variables
// FTP
std::string ftpUsername;
std::string ftpPassword;
std::string ftpAddress;
int ftpPort;
// rcon
std::string rconAddress;
int rconPort;
std::string rconPassword;
std::atomic<bool> connected{false};
// words
std::string wordsArray;
// file downloaded
std::string localLogName;
// discord
std::string discordToken;
// telegram
std::string telegramToken;
int chat_id;
// server
std::string serverAddress;
int serverPort;

std::string delCommands;
bool deleteCommands = false;
// other variables
bool needrestart = true;
int currentpass = 0;
int currenttime = 0;
int newtime = 0;
int result = 0;
int tailtime = 2; // parse the last X minutes of the log file
bool is_valid = false;
std::string paypal;
std::string steamLink;

rconpp::response response;
// get the variables from a .config file
void get_config()
{
    // check if config file exists
    std::ifstream file("config.cfg");
    if (!file)
    {
        std::cerr << "Error: Unable to open config file or file dont exist\n";
        exit(1);
    }

    std::cout << "Reading config file" << std::endl;
    libconfig::Config cfg;
    try
    {
        cfg.readFile("config.cfg");
    }
    catch (const libconfig::ParseException &pex)
    {
        std::cerr << "Parse error at " << pex.getLine() << " - " << pex.getError() << std::endl;
        exit(1);
    }
    // FTP
    std::cout << "Reading FTP variables" << std::endl;
    ftpUsername = cfg.lookup("ftp_username").c_str();
    ftpPassword = cfg.lookup("ftp_password").c_str();
    ftpAddress = cfg.lookup("ftp_addressnfile").c_str();
    ftpPort = cfg.lookup("ftp_port");
    // rcon
    std::cout << "Reading RCON variables" << std::endl;
    rconAddress = cfg.lookup("rcon_address").c_str();
    rconPort = cfg.lookup("rcon_port");
    rconPassword = cfg.lookup("rcon_password").c_str();
    // words
    std::cout << "Reading words array" << std::endl;
    wordsArray = cfg.lookup("words_array").c_str();
    // file name
    std::cout << "Reading local log file name" << std::endl;
    localLogName = cfg.lookup("local_log_file_name").c_str();
    // discord token
    std::cout << "Reading discord token" << std::endl;
    discordToken = cfg.lookup("discord_token").c_str();
    // telegram token
    std::cout << "Reading telegram token" << std::endl;
    telegramToken = cfg.lookup("telegram_token").c_str();
    chat_id = cfg.lookup("telegram_chat_id");
    // server
    std::cout << "Reading server variables" << std::endl;
    serverAddress = cfg.lookup("server_address").c_str();
    serverPort = cfg.lookup("server_port");
    // other variables
    std::cout << "Reading other variables" << std::endl;
    paypal = cfg.lookup("paypal").c_str();
    steamLink = cfg.lookup("steam_link").c_str();
    delCommands = cfg.lookup("delete_commands").c_str();
    if (delCommands == "true")
    {
        deleteCommands = true;
    }
    else
    {
        deleteCommands = false;
    }

    std::cout << "Config file read" << std::endl;
}

// no idea what this does
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t totalSize = size * nmemb;
    std::ofstream *file = static_cast<std::ofstream *>(userp);
    file->write(static_cast<char *>(contents), totalSize);
    return totalSize;
}

// Log Functions Get Log > check last lines > Check Log > Get text matches > Send message

// download log file
void get_log()
{
    std::cout << "Downloading log file" << std::endl;
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl)
    {
        // FTP username and password
        curl_easy_setopt(curl, CURLOPT_USERNAME, ftpUsername.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, ftpPassword.c_str());

        std::string outputFilePath = localLogName;

        std::ofstream outputFile(outputFilePath, std::ios::binary);
        if (!outputFile)
        {
            std::cerr << "Failed to open output file." << std::endl;
            exit(1);
        }
        // FTP address
        curl_easy_setopt(curl, CURLOPT_URL, ftpAddress.c_str());
        // FTP port
        curl_easy_setopt(curl, CURLOPT_PORT, ftpPort);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outputFile);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::cerr << "Failed to download file: " << curl_easy_strerror(res) << std::endl;
            exit(1);
        }

        curl_easy_cleanup(curl);
        outputFile.close();
    }

    curl_global_cleanup();
    std::cout << "Log file downloaded" << std::endl;
}

// Function to parse the timestamp from a log line
long long parseTimestamp(const std::string &logLine)
{
    size_t pos = logLine.find('>');
    if (pos != std::string::npos)
    {
        std::string timestampStr = logLine.substr(0, pos);
        timestampStr = timestampStr.substr(timestampStr.find_last_of(' ') + 1);
        return std::stoll(timestampStr);
    }
    throw std::invalid_argument("Invalid log line format: " + logLine);
}

// Function to check if a log line contains the specified text after the prefix
bool containsText(const std::string &logLine, const std::string &searchText)
{
    size_t pos = logLine.find("LOG  : General");
    if (pos != std::string::npos)
    {
        pos = logLine.find(">", pos);
        if (pos != std::string::npos)
        {
            pos = logLine.find(">", pos + 1);
            if (pos != std::string::npos)
            {
                std::string message = logLine.substr(pos + 1);
                return message.find(searchText) != std::string::npos;
            }
        }
    }
    return false;
}

// analize log file to check if the specified text is present
bool analazingLogFile()
{
    std::cout << "Opening log file" << std::endl;
    std::ifstream logFile(localLogName);
    if (!logFile)
    {
        std::cerr << "Error: Unable to open log file\n";
        return 1;
    }
    std::cout << "Done" << std::endl;
    std::cout << "Getting current time" << std::endl;
    // Get current time
    auto currentTime = std::chrono::system_clock::now();
    auto currentTimeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime.time_since_epoch()).count();

    std::cout << "Current time: " << currentTimeStamp << std::endl;
    // Vector to store log lines from 5 minutes ago
    std::vector<std::string> logLinesFiveMinutesAgo;

    std::cout << "Filtering log file" << std::endl;
    std::string line;
    while (std::getline(logFile, line))
    {
        if (line.find("LOG  : General") == 0)
        { // Check if the line starts with "LOG  : General"
            long long timestamp = parseTimestamp(line);
            // Check if the timestamp is within the last 5 minutes
            //
            if (currentTimeStamp - timestamp <= 5 * 60 * 1000)
            {
                logLinesFiveMinutesAgo.push_back(line);
            }
        }
    }
    // Define the text to match
    std::string searchText = wordsArray;
    std::cout << "Searching for text: " << searchText << std::endl;
    // Compare the last lines from 5 minutes ago with the specified text
    bool foundMatch = false;

    // Compare the last lines from 5 minutes ago with the specified text
    for (const auto &logLine : logLinesFiveMinutesAgo)
    {
        if (containsText(logLine, searchText))
        {
            std::cout << "Match found: " << logLine << std::endl;
            foundMatch = true;
            // If you need to take some action here, you can do it.
        }
    }

    if (foundMatch)
    {
        // Trigger the action if at least one match was found
        std::cout << "At least one match was found." << std::endl;
        return true;
    }
    else
    {
        // Do nothing if no matches were found
        std::cout << "No matches were found." << std::endl;
        return false;
    }
}
// dice roll function 1d100
int roll_dice(int sides, int times)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, sides);
    int result = 0;
    for (int n = 0; n < times; ++n)
    {
        result += dis(gen);
    }
    std::cout << "The result is: " << result << std::endl;
    return result;
}

// generate a 6-digit password and send it via Telegram
int generatePass()
{
    // this generates a random number between 100000 and 999999 as password
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);
    int currentpass = dis(gen);
    std::string message = "Tu codigo de autenticacion es: " + std::to_string(currentpass);
    return currentpass;
}
// generate unix timestamp
int generateTime()
{
    // this generates a timestamp
    std::time_t time = std::time(nullptr);

    // Convert the Unix timestamp to an integer
    int curenttime = static_cast<int>(time);
    return curenttime;
}

// validate the password and check if it is within 2 minutes of the generated time
bool validatePass(int newpass, int newtime, int currentpass, int currenttime)
{
    // this checks if the new pass is the same as the current pass and if the time is within 2 minutes
    if (newpass == currentpass && newtime - currenttime <= 120)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int main()
{
    // Get the variables from the config file
    get_config();

    dpp::cluster bot(discordToken);
    TgBot::Bot telebot(telegramToken);
    bot.on_log(dpp::utility::cout_logger());
    // attempt to connect to RCON
    std::cout << "Connecting to RCON with address: " << rconAddress << " port: " << rconPort << " password: " << rconPassword << std::endl;
    rconpp::rcon_client client(rconAddress, rconPort, rconPassword);

    client.on_log = [](const std::string_view &log)
    {
        std::cout << log << "\n";
    };

    client.start(true);
    bool isConnected = client.connected.load();
    if (isConnected == false)
    {
        std::cout << "Failed to connect to RCON server" << std::endl;
        exit(1);
    }
    bot.on_slashcommand([&](const dpp::slashcommand_t &event)
                        {
        auto cmd_data = event.command.get_command_interaction();

        if (event.command.get_command_name() == "players")
        {
            std::cout << "Checking ammount of players" << std::endl;
            response = client.send_data_sync("players", 3, rconpp::data_type::SERVERDATA_EXECCOMMAND, true);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << response.data << std::endl;
            event.reply(response.data);
        }
        if (event.command.get_command_name() == "mods")
        {
            event.reply("Suscribete a la lista: " + steamLink + " \npara precargar los mods antes de entrar al server");
        } 
        if (event.command.get_command_name() == "tengolag")
        {
            event.reply("Abre el directorio del juego y edita el archivo ProjectZomboid64.json y cambia las opciones a: -Xms4096m and -Xmx8192m para que tengas 8GB de ram asignados al juego");
        }
        if (event.command.get_command_name() == "mapa")
        {
            event.reply("Aqui tienes el Mapa: https://i.imgur.com/f7r2Upz.png");
            // alternatives are https://map.projectzomboid.com/ or create a custom map yourself
        }
        if (event.command.get_command_name() == "acceso")
        {
            event.reply("Para tener acceso porfavor deposita a esta cuenta\nCLABE:" + paypal + "\nBanco:      Banamex\nNombre: George\nPon tu user como referencia\n$75.00 Mxn\nUna vez hecho el deposito manda tu comprobante a George\nGracias por tu apoyo");
        }
        if (event.command.get_command_name() == "serverinfo")
        {
            event.reply(std::string("La ip del server es: " + serverAddress + " \nEl puerto es: " + std::to_string(serverPort) + " \nUsa Steam Relay\nObten tu contraseña de acceso con /acceso"));
        }
        /*
        //this is the old checkmods command it took more than 3 seconds to respond so i changed it to a message
        //i need to find a way to make it faster
        if (event.command.get_command_name() == "checkmods")
        {
            std::cout << "Checking if mods need update. sending command to server" << std::endl;
            response = client.send_data_sync("checkModsNeedUpdate", 3, rconpp::data_type::SERVERDATA_EXECCOMMAND, true);
            get_log();
            // check if the log file contains the specified text
            if (analazingLogFile())
            {
                telebot.getApi().sendMessage(chat_id, "Necesitamos actualizar los mods");
                event.reply("Los mods necesitan actualizarse\nAcabo de notificar a los admins");
            }
            else
            {
                event.reply("todo en orden");
            }
        }
        */
        if (event.command.get_command_name() == "checkmods")
        {
            std::cout << "Checking if mods need update. sending command to server" << std::endl;
            event.reply("Voy a checar si los mods necesitan actualizarse \nEn caso de que si, le avisare a George para que reinice el server");
            response = client.send_data_sync("checkModsNeedUpdate", 3, rconpp::data_type::SERVERDATA_EXECCOMMAND, true);
            get_log();
            // check if the log file contains the specified text
            if (analazingLogFile())
            {   
                std::cout << "Mods need update" << std::endl;
                telebot.getApi().sendMessage(chat_id, "Necesitamos actualizar los mods");
            }

        } 
        if (event.command.get_command_name() == "dados")
        {
            //std::string user = std::get<std::string>(event.get_parameter("user"));
            int dado = 0;
            dado = roll_dice(100, 1);
            if (dado == 100)
            {
                std::string message = "El resultado es: " + std::to_string(dado) + " CRITICO ten un taco :D :taco: ";
                event.reply("El resultado es: " + std::to_string(dado) + " CRITICO ten un taco :D :taco:");
            }
            else if (dado >= 77 && dado <= 99)
            {
                std::string message = "El resultado es: " + std::to_string(dado) + " Tienes suerte";
                event.reply("El resultado es: " + std::to_string(dado) + " Tienes suerte");
            }
            else if (dado >= 70 && dado <= 76)
            {
                std::string message = "El resultado es: " + std::to_string(dado) + " Tienes suerte, El clima ha mejorado";
                //response = client.send_data_sync("stopweather", 3, rconpp::data_type::SERVERDATA_EXECCOMMAND, true);
                event.reply("El resultado es: " + std::to_string(dado) + " Tienes suerte, El clima ha mejorado");
            }
            else if (dado >= 51 && dado <= 68)
            {
                std::string message = "El resultado es: " + std::to_string(dado) + " Tienes suerte, El clima ha mejorado";
                //response = client.send_data_sync("stopweather", 3, rconpp::data_type::SERVERDATA_EXECCOMMAND, true);
                event.reply("El resultado es: " + std::to_string(dado) + " Tienes suerte, El clima ha mejorado");
            }
            else if (dado >= 43 && dado <= 50)
            {
                std::string message = "El resultado es: " + std::to_string(dado) + " Se escuchan disparos a lo lejos";
                //response = client.send_data_sync("gunshot", 3, rconpp::data_type::SERVERDATA_EXECCOMMAND, true);
                event.reply("El resultado es: " + std::to_string(dado) + " Se escuchan disparos a lo lejos");
            }
            else if (dado >= 26 && dado <= 41)
            {
                std::string message = "El resultado es: " + std::to_string(dado) + " Se escuchan disparos a lo lejos";
                //response = client.send_data_sync("gunshot", 3, rconpp::data_type::SERVERDATA_EXECCOMMAND, true);
                event.reply("El resultado es: " + std::to_string(dado) + " Se escuchan disparos a lo lejos");
            }
            else if (dado == 69)
            {
                std::string message = "El resultado es: " + std::to_string(dado) + " nice";
                event.reply("El resultado es: " + std::to_string(dado) + " nice");
            }
            else if (dado == 42)
            {
                std::string message = "El resultado es: " + std::to_string(dado) + " La respuesta a la vida, el universo y todo lo demas";
                event.reply("El resultado es: " + std::to_string(dado) + " La respuesta a la vida, el universo y todo lo demas");
            }
            else if (dado >= 2 && dado <= 25)
            {
                std::string message = "El resultado es: " + std::to_string(dado) + " Helicoptero! Cuidado con los zombies";
                //response = client.send_data_sync("chopper", 3, rconpp::data_type::SERVERDATA_EXECCOMMAND, true);
                event.reply("El resultado es: " + std::to_string(dado) + " Helicoptero! Cuidado con los zombies");
            }
            else if (dado == 1)
            {
                std::string message = "El resultado es: " + std::to_string(dado) + " HORDA DE ZOMBIES! Corre por tu vida";
                event.reply("El resultado es: " + std::to_string(dado) + " HORDA DE ZOMBIES! Corre por tu vida");
            }
        }
        if (event.command.get_command_name() == "tfa")
        {
            currentpass = generatePass();
            currenttime = generateTime();
            std::cout << "Sending TFA via Telegram with code: " << currentpass << " and time: " << currenttime << "\n";
            telebot.getApi().sendMessage(chat_id, "Tu codigo de autenticacion es: " + std::to_string(currentpass));
            event.reply("TFA enviado");
        }
        if (event.command.get_command_name() == "resetserver")
            {
            // Check if options are present}
            if (currentpass == 0 || currenttime == 0)
            {
                event.reply("No hay un codigo de autenticacion activo");
            }
                int64_t newpass = std::get<int64_t>(event.get_parameter("passS"));
                newtime = generateTime();
                is_valid = validatePass(newpass, newtime, currentpass, currenttime);
                if (is_valid == true)
                {
                    // Respond to the command
                    event.reply("Restarting the server");
                    currentpass = 0;
                    currenttime = 0;
                    response = client.send_data_sync("save", 3, rconpp::data_type::SERVERDATA_EXECCOMMAND, true);
                    response = client.send_data_sync("quit", 3, rconpp::data_type::SERVERDATA_EXECCOMMAND, true);
                    exit(0);
                }
                else
                {
                    // Respond to the command
                    event.reply("Password is incorrect.");
                }
            }
        if (event.command.get_command_name() == "resetbot")
            {
            // Check if options are present}
            if (currentpass == 0 || currenttime == 0)
            {
                event.reply("No hay un codigo de autenticacion activo");
            }
                int64_t newpass = std::get<int64_t>(event.get_parameter("passB"));
                newtime = generateTime();
                is_valid = validatePass(newpass, newtime, currentpass, currenttime);
                if (is_valid == true)
                {
                    // Respond to the command
                    event.reply("Restarting bot");
                    currentpass = 0;
                    currenttime = 0;
                    exit(0);
                }
                else
                {
                    // Respond to the command
                    event.reply("Password is incorrect.");
                }
            } });

    bot.on_ready([&](const dpp::ready_t &event)
                 {
        if (dpp::run_once<struct register_bot_commands>())
        {

            if (deleteCommands)
            {
                bot.global_bulk_command_delete();
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
            bot.set_presence(dpp::presence(dpp::ps_online, dpp::at_game, "Project Zomboid"));
            bot.global_command_create(dpp::slashcommand("serverinfo", "informacion del servidor", bot.me.id));
            bot.global_command_create(dpp::slashcommand("players", "cuantos jugadores estan conectados actualmente", bot.me.id));
            bot.global_command_create(dpp::slashcommand("checkmods", "revisa si hay actualizaciones", bot.me.id));
            bot.global_command_create(dpp::slashcommand("tfa", "solo para admins", bot.me.id));
            bot.global_command_create(dpp::slashcommand("dados", "??? :) ", bot.me.id));
            bot.global_command_create(dpp::slashcommand("acceso", "acceso al servidor", bot.me.id));
            bot.global_command_create(dpp::slashcommand("tengolag", "ayuda con el lag", bot.me.id));
            bot.global_command_create(dpp::slashcommand("mods", "lista de mods", bot.me.id));
            bot.global_command_create(dpp::slashcommand("mapa", "mapa del server", bot.me.id));
            /*
            dpp::slashcommand("dados", "??? :)", bot.me.id);
            dados.add_option(dpp::command_option(dpp::co_string, "user", "tu user de zomboid", true));
            bot.global_command_create(dados);
            */
            dpp::slashcommand resetserver("resetserver", "solo para admins", bot.me.id);
            resetserver.add_option(
                dpp::command_option(dpp::co_integer, "passS", "password", true)
                    .set_min_value(100000)
                    .set_max_value(999999));
            bot.global_command_create(resetserver);

            dpp::slashcommand resetbot("resetbot", "solo para admins", bot.me.id);
            resetbot.add_option(
                dpp::command_option(dpp::co_integer, "passB", "password", true)
                    .set_min_value(100000)
                    .set_max_value(999999));
            bot.global_command_create(resetbot);
        } });
    bot.start(dpp::st_wait);
};
