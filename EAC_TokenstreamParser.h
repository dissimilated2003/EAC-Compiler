#ifndef EAC_TOKENSTREAM_PARSER
#define EAC_TOKENSTREAM_PARSER


#include "EAC_CampaignBuilder.h"


#include <string>
#include <regex>
#include <fstream>
#include <sstream>


// класс TokenstreamParser, отвечающий за разбор промежуточного представления рассылки tokenstream.txt
class TokenstreamParser
{
public:
    TokenstreamParser() = default;
    TokenstreamParser(
        ICampaignBuilder& builder, 
        const std::string& themeColor
    )
        : m_campaignBuilder(builder)
        , m_themeColor(themeColor)
    {}

    void ParseTokenstream(const std::string& tokenstream)
    {
        std::ifstream file{tokenstream};
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open tokenstream.txt");
        }

        std::string line{};
        std::vector<std::string> lines{};
        bool inMultiLineLexeme{false};
        std::string multilineBuffer{};
        std::string lexemeType{};

        std::string currentParagraph{};
        auto flushParagraph = [&]() {
            if (!currentParagraph.empty())
            {
                ProcessTextLine(currentParagraph);
                currentParagraph.clear();
            }
        };

        while (std::getline(file, line))
        {
            if (inMultiLineLexeme)
            {
                multilineBuffer += line + "\n";
                if (line.find(']') != std::string::npos)
                {
                    inMultiLineLexeme = false;
                    flushParagraph();
                    ProcessMultilineLexeme(lexemeType, multilineBuffer);
                }

                continue;
            }

            if (line.find("[LL:") == 0 || line.find("[DL:") == 0)
            {
                flushParagraph();
                inMultiLineLexeme = true;
                lexemeType = line.substr(1, 2);
                multilineBuffer = line + "\n";
                continue;
            }

            if (line.find("[BT:") == 0 || 
                line.find("[BR:") == 0 || 
                line.find("[VC:") == 0 || 
                line.find("[RV:") == 0 || 
                line.find("[SN:") == 0)
            {
                flushParagraph();
                ProcessSingleLineLexeme(line);
                continue;
            }

            if (line.empty())
            {
                flushParagraph();
                continue;
            }

            if (!currentParagraph.empty())
            {
                currentParagraph += "\n";
            }
                
            currentParagraph += line;
        }
        flushParagraph();
        file.close();
    }

private:
    ICampaignBuilder& m_campaignBuilder;
    std::string m_themeColor{};


    std::string ExtractFirstUtf8Char(const std::string& str)
    {
        if (str.empty()) return "";

        size_t len = 1;
        unsigned char c = static_cast<unsigned char>(str[0]);

        if ((c & 0x80) == 0) len = 1; // ASCII
        else if ((c & 0xE0) == 0xC0) len = 2; 
        else if ((c & 0xF0) == 0xE0) len = 3; 
        else if ((c & 0xF8) == 0xF0) len = 4;
        else len = 1;

        return str.substr(0, len);
    }


    std::string ReplaceVariablesInText(const std::string& text)
    {
        std::string result = text;
        std::regex vcPattern(R"(\[VC:([^\]]+)\])");
        std::smatch match;

        while (std::regex_search(result, match, vcPattern))
        {
            std::string synonym = match[1].str();
            VariableFromCRMNode tempNode(synonym);
            std::string replacement = tempNode.GetHTMLRepresentation();

            result.replace(match.position(0), match.length(0), replacement);
        }

        return result;
    }


    void ProcessSingleLineLexeme(const std::string& line)
    {
        // BT lexeme parsing
        if (line.find("[BT:") == 0)
        {
            std::string content = line.substr(4, line.length() - 5);
            size_t colonPos = std::string::npos;
            
            size_t protocolPos = content.find("://");
            if (protocolPos == std::string::npos)
            {
                protocolPos = content.find("//");
            }
            
            if (protocolPos != std::string::npos)
            {
                colonPos = content.rfind(':', protocolPos - 1);
                if (colonPos == std::string::npos)
                {
                    if (content[protocolPos] == '/')
                    {
                        colonPos = content.find(':', protocolPos - 1);
                        if (colonPos == std::string::npos) 
                        {
                            colonPos = protocolPos;
                        }
                    }
                }
            }
            
            if (colonPos != std::string::npos && content[colonPos] == ':')
            {
                std::string text = content.substr(0, colonPos);
                std::string url = content.substr(colonPos + 1);
                AddNode(std::make_unique<ButtonNode>(text, m_themeColor, url));
            }
            else
            {
                AddNode(std::make_unique<ButtonNode>(content, m_themeColor, ""));
            }
            return;
        }

        // BR lexeme parsing
        if (line.find("[BR:") == 0)
        {
            std::string content = line.substr(4, line.length() - 5);
            std::vector<std::string> buttonsTexts{};
            std::vector<std::string> buttonsLinks{};

            std::stringstream ss(content);
            std::string segment{};
            while (std::getline(ss, segment, '|'))
            {
                size_t colonPos = std::string::npos;
                size_t protocolPos = segment.find("://");
                if (protocolPos == std::string::npos)
                {
                    protocolPos = segment.find("//");
                }
                if (protocolPos != std::string::npos)
                {
                    colonPos = segment.rfind(':', protocolPos - 1);
                    if (colonPos == std::string::npos)
                    {
                        colonPos = segment.find(':', protocolPos - 1);
                        if (colonPos == std::string::npos) colonPos = protocolPos;
                    }
                }

                if (colonPos != std::string::npos && segment[colonPos] == ':')
                {
                    buttonsTexts.push_back(segment.substr(0, colonPos));
                    buttonsLinks.push_back(segment.substr(colonPos + 1));
                }
                else
                {
                    buttonsTexts.push_back(segment);
                    buttonsLinks.push_back("");
                }
            }
        
            AddNode(std::make_unique<ButtonsRowNode>(buttonsTexts, buttonsLinks, m_themeColor));
            return;
        }

        // VC lexeme parsing
        if (line.find("[VC:") == 0)
        {
            ProcessTextLine(line);
            return;
        }

        // SN lexeme parsing
        if (line.find("[SN:") == 0)
        {
            std::string content = line.substr(4, line.length() - 5);
            AddNode(std::make_unique<SenderNode>(content));
            return;
        }

        // RV lexeme parsing
        if (line.find("[RV:") == 0)
        {
            std::string content = line.substr(4, line.length() - 5);
            size_t pipePos = content.find_last_of(':');
            if (pipePos != std::string::npos)
            {
                std::string reviewText = content.substr(0, pipePos);
                std::string author = content.substr(pipePos + 1);
                AddNode(std::make_unique<ReviewNode>(reviewText, author, m_themeColor));
            }
            return;
        }
    }


    void ProcessMultilineLexeme(const std::string& type, const std::string& buffer)
    {
        if (type == "LL")
        {
            ProcessLabeledList(buffer);
        }
        else if (type == "DL")
        {
            ProcessDotList(buffer);
        }
    }


    void ProcessLabeledList(const std::string& buffer)
    {
        std::vector<std::string> emojies{};
        std::vector<std::string> texts{};

        std::stringstream ss(buffer);
        std::string line{};
        bool firstLine{true};

        while (std::getline(ss, line))
        {
            if (firstLine)
            {
                firstLine = false;
                continue;
            }

            if (line.empty())
            {
                continue;
            }

            if (line.find(']') != std::string::npos)
            {
                break;
            }

            if (line.length() > 0)
            {
                std::string emoji = ExtractFirstUtf8Char(line);
                std::string text = line.substr(emoji.length());
                
                if (!text.empty() && text[0] == ' ')
                {
                    text = text.substr(1);
                }
                emojies.push_back(emoji);
                texts.push_back(text);
            }
        }

        if (!emojies.empty() && !texts.empty())
        {
            AddNode(std::make_unique<LabeledListNode>(emojies, texts));
        }
    }


    void ProcessDotList(const std::string& buffer)
    {
        std::vector<std::string> texts{};
        std::stringstream ss(buffer);
        std::string line{};
        bool firstLine{true};

        while (std::getline(ss, line))
        {
            if (firstLine)
            {
                firstLine = false;
                continue;
            }

            if (line.empty())
            {
                continue;
            }

            if (line.find(']') != std::string::npos)
            {
                break;
            }

            texts.push_back(line);
        }

        if (!texts.empty())
        {
            AddNode(std::make_unique<DotListNode>(texts));
        }
    }


    void ProcessTextLine(const std::string& line)
    {
        std::string openedVarsText = ReplaceVariablesInText(line);
        std::string htmlText = openedVarsText;
        size_t pos = 0;
        while ((pos = htmlText.find('\n', pos)) != std::string::npos)
        {
            htmlText.replace(pos, 1, "<br>");
            pos += 4;
        }
        AddNode(std::make_unique<ParagraphNode>(htmlText));
    }


    void AddNode(std::unique_ptr<ICampaignNode> node)
    {
        m_campaignBuilder.AddNode(std::move(node));
    }
};


#endif