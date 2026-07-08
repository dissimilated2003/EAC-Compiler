#ifndef EAC_UTILS
#define EAC_UTILS


#include <cmath>
#include <string>
#include <stdexcept>
#include <cstdlib>


// функция, которая нужна для получения более светлого цвета из тематичного цвета рассылки
inline std::string GetBrighterColorByBase(const std::string& baseColor, double factor = 0.10)
{
    int r = std::stoi(baseColor.substr(1, 2), nullptr, 16);
    int g = std::stoi(baseColor.substr(3, 2), nullptr, 16);
    int b = std::stoi(baseColor.substr(5, 2), nullptr, 16);

    int rBrighter = std::round(r * factor + 255 * (1 - factor));
    int gBrighter = std::round(g * factor + 255 * (1 - factor));
    int bBrighter = std::round(b * factor + 255 * (1 - factor));

    char colorBuffer[8];
    snprintf(colorBuffer, sizeof(colorBuffer), "#%02X%02X%02X", rBrighter, gBrighter, bBrighter);
    return std::string(colorBuffer);
} 


// функция, запускающая фронтенд компилятора - LexicalStreamParser (парсер .docx на Python)
void LaunchLexicalStreamParser(const std::string& parser, const std::string& docxFile, const std::string& themeColor)
{
    std::string launchCommand = "python " + parser + " \"" + docxFile + "\" " + themeColor;
    int result = std::system(launchCommand.c_str());

    if (result != 0)
    {
        throw std::runtime_error("Parsing .docx failed with code: " + std::to_string(result));
    }
}


#endif