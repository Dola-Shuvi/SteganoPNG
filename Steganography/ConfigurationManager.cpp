#include "ConfigurationManager.h"
#include <variant>
#include <iostream>

ConfigurationManager::ConfigurationManager(int argc, char *commandLine[])
{

    mode = ConfigurationManager::Mode::HELP;
    imagePath = "";
    dataPath = "";
    encryption = false;
    password = "";
    disableCompression = false;

    for (int i = 1; i < argc; i++) {


        switch (i) {

            case 1:
                switch (*commandLine[i]) {
                    case 'a':
                        mode = Mode::HIDE;
                        break;
                    case 'x':
                        mode = Mode::EXTRACT;
                        break;
                    case 't':
                        mode = Mode::TEST;
                        break;
                    case 'h':
                        mode = Mode::HELP;
                        break;
                    default:
                        mode = Mode::HELP;
                        imagePath = "";
                        dataPath = "";
                        encryption = false;
                        password = "";
                        disableCompression = false;
                        break;
                }
                break;
            case 2:
                imagePath = commandLine[2];
                break;
            case 3:
                if (strcmp(commandLine[i], "-p") == 0) {
                    encryption = true;
                    password = commandLine[i + 1];
                }
                else if(strcmp(commandLine[i], "--no-compression") == 0){
                    disableCompression = true;
                }
                else
                {
                    dataPath = commandLine[i];
                }
                break;
            case 4:
                if (strcmp(commandLine[i], "-p") == 0) {
                    encryption = true;
                    password = commandLine[i + 1];
                }
                else if (strcmp(commandLine[i], "--no-compression") == 0) {
                    disableCompression = true;
                }
                break;
            case 5:
                if (strcmp(commandLine[i], "--no-compression") == 0) {
                    disableCompression = true;
                }
                break;
            case 6:
                if (strcmp(commandLine[i], "--no-compression") == 0) {
                    disableCompression = true;
                }
                break;
        }

    }

    std::cout << "Mode: \t" << (int)std::get<Mode>(getOption(Option::Mode)) << std::endl;
    std::cout << "Image Path: \t" << std::get<std::string>(getOption(Option::ImagePath)) << std::endl;
    std::cout << "Data Path: \t" << std::get<std::string>(getOption(Option::DataPath)) << std::endl;
    std::cout << "Encryption: \t" << std::get<bool>(getOption(Option::Encryption)) << std::endl;
    std::cout << "Password: \t" << std::get<std::string>(getOption(Option::Password)) << std::endl;
    std::cout << "Disable Compression: \t" << std::get<bool>(getOption(Option::DisableCompression)) << std::endl << std::endl;

}
