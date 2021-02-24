#pragma once

#include <string>
#include <variant>

class ConfigurationManager
{

public:
	ConfigurationManager(int argc, char* commandLine[]);

	enum class Mode
	{
		HIDE,
		EXTRACT,
		TEST,
		HELP
	};

	enum class Option {
		Mode,
		ImagePath,
		DataPath,
		Encryption,
		Password,
		DisableCompression
	};

    using variant_value = std::variant<ConfigurationManager::Mode, std::string, bool>;
    std::variant<ConfigurationManager::Mode, std::string, bool> getOption(ConfigurationManager::Option option) {

        switch (option) {
        case ConfigurationManager::Option::Mode:
            return variant_value(mode);
        case ConfigurationManager::Option::ImagePath:
            return variant_value(imagePath);
        case ConfigurationManager::Option::DataPath:
            return variant_value(dataPath);
        case ConfigurationManager::Option::Encryption:
            return variant_value(encryption);
        case ConfigurationManager::Option::Password:
            return variant_value(password);
        case ConfigurationManager::Option::DisableCompression:
            return variant_value(disableCompression);
        default:
            break;
        }
    };

private:
    ConfigurationManager::Mode mode;
    std::string imagePath;
    std::string dataPath;
    bool encryption;
    std::string password;
    bool disableCompression;

};