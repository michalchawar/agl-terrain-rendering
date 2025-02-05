// ==========================================================================
// Config: class definition
//
// Michał Chawar
// ==========================================================================
// Config
//===========================================================================

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>

template <typename T>
class Option {
public:
    Option() {}
    Option(std::string k, T val) : key(k), value(val) {}

    std::string getKey()   { return key; }
    T           getValue() { return value; }
private:
    std::string key;
    T value;
};

class Config {
public:
    Config() {}
    Config(std::string const& fileName) {
        load(fileName);
    }

    void load(std::string const& fileName) {
        std::ifstream configFile(fileName);
        if (!configFile.is_open()) throw std::ios_base::failure("Nie udało się otworzyć pliku ustawień: " + fileName);

        std::string line;
        while (std::getline(configFile, line)) {
            // Ignoruj puste linie lub komentarze (zakładamy komentarze zaczynają się od '#')
            if (line.empty() || line[0] == '#') continue;

            std::istringstream iss(line);
            std::string key, equals, value;

            // Oczekiwany format: key = value
            if (!(iss >> key >> equals >> value) || equals != "=") {
                std::cerr << "Niepoprawny format opcji ustawień w linii " << line << ", pominięto.\n";
                continue;
            }

            // printf("%s: %s -> %d\n", key.c_str(), value.c_str(), (value == "true"));
            options.push_back( Option(key, (value == "true")) );
        }
        configFile.close();
    }

    bool getValue(std::string const& key) {
        for (int i = 0; i < options.size(); i++) {
            if (options[i].getKey() == key) return options[i].getValue();
        }

        std::cerr << "Nie znaleziono opcji o nazwie '" << key << "', zwrócono false.\n";
        return false;
    }
private:
    std::vector<Option<bool>> options;
};