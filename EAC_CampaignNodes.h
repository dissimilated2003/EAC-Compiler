#ifndef EAC_CAMPAIGN_NODES
#define EAC_CAMPAIGN_NODES


#include "EAC_Utils.h"


#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <sstream>


// перечисление "Тип узла", нужно для конечной обработки билдером по расстановке отступов
enum class NodeType
{
    PARAGRAPH_NODE,
    BUTTON_NODE,
    BUTTONS_ROW_NODE,
    LABELED_LIST_NODE,
    DOT_LIST_NODE,
    REVIEW_NODE,
    VARIABLE_FROM_CRM_NODE,
    SENDER_NODE,
    SPACING_NODE
};


// интерфейс "Узел кампании"
// задаёт контракт между всеми узлами на получение их представления в HTML 
struct ICampaignNode
{
    virtual std::string GetHTMLRepresentation() = 0;
    virtual NodeType GetNodeType() const = 0;

    virtual ~ICampaignNode() = default;
};


// класс "Узел Абзац", представляет собой обыкновенный текстовый абзац
// по умолчанию компилятор считывает весь текст как абзац, если не встречает []
class ParagraphNode final : public ICampaignNode
{
public:
    ParagraphNode() = default;
    ParagraphNode(const std::string text) : m_text(text) {} 

    std::string GetHTMLRepresentation() override
    {
        std::string formattedText = m_text;
        return R"(
                    <tr>
                        <td style="color: #3d3e47; font-weight: 400; font-size: 16px; line-height: 24px;
                            font-family: Roboto, Helvetica, Arial, sans-serif; height: 24px;" align="left" 
                            class="esd-text">)" + m_text + 
                            R"(
                        </td>
                    </tr>
                )";
    }

    NodeType GetNodeType() const override
    {
        return NodeType::PARAGRAPH_NODE;
    }

private:
    std::string m_text;
    std::vector<std::string> m_links{};
    std::string m_linkColor{};
};


// класс "Узел отступа", нужен для расположения отступов между соседними узлами
// выделен в отдельную сущность, чтобы не зависеть от других узлов
// хранится в векторе узлов в скелете рассылки и обрабатывается им
class SpacingNode final : public ICampaignNode
{
public:
    SpacingNode() = default;
    SpacingNode(unsigned int height) : m_spacingHeight(height) {}

    void SetSpacingHeight(unsigned int height)
    {
        m_spacingHeight = height;
    }

    std::string GetHTMLRepresentation() override
    {
        return R"(
                    <tr>
                        <td style="font-size: 0px; line-height: )" + std::to_string(m_spacingHeight) + R"(px; height: )" + std::to_string(m_spacingHeight) + R"(px;" class="esd-text">
                            &nbsp;
                        </td>
                    </tr>
                )";
    }

    NodeType GetNodeType() const override
    {
        return NodeType::SPACING_NODE;
    }

private:
    unsigned int m_spacingHeight{20};
};


// класс "Кнопка", нужен для того чтобы создавать кнопки в рассылке
class ButtonNode final : public ICampaignNode
{
public:
    ButtonNode() = default;
    ButtonNode(
        const std::string& buttonText, 
        const std::string& buttonColor, // цвет сюда поступает в формате #xxxxxx
        const std::string& buttonLink
    )
        : m_buttonText(buttonText)
        , m_buttonColor(buttonColor)
        , m_buttonLink(buttonLink)
    {
        if (m_buttonColor[0] != '#' || m_buttonColor.length() != 7)
        {
            throw std::invalid_argument("Invalid background color at \"ButtonNode\": " + m_buttonColor);
        }
    }

    std::string GetHTMLRepresentation() override
    {
        return R"(
                    <tr>
                        <td align="center">
                            <table border="0" width="300" cellspacing="0" cellpadding="0">
                                <tbody>
                                    <tr>
                                        <td style="border-radius: 10px; background-color: )" + m_buttonColor + R"(; )" + 
                                        R"(color: #ffffff; font-weight: bold; font-size: 16px; line-height: 24px; )" + 
                                        R"(font-family: Roboto, Helvetica, Arial, sans-serif; text-align: center; cursor: pointer; ")" + 
                                        R"(align="center" bgcolor=")" + m_buttonColor + R"(" height="60"><a style="display: table-cell; )" + 
                                        R"(vertical-align: middle; width: 300px; height: 60px; box-sizing: border-box; )" + 
                                        R"(color: #ffffff !important; font-weight: bold; font-size: 16px; line-height: 24px; )" + 
                                        R"(font-family: Roboto, Helvetica, Arial, sans-serif; text-align: center; text-decoration: none; )" + 
                                        R"(cursor: pointer; padding: 5px 20px 5px 20px;" href=")" + m_buttonLink + R"(" )" +
                                        R"(target="_blank"><strong style="color: #ffffff; font-weight: bold; font-size: 16px; )" + 
                                        R"(line-height: 24px; font-family: Roboto, Helvetica, Arial, sans-serif; text-align: center; )" + 
                                        R"(text-decoration: none; cursor: pointer;">)" + m_buttonText + R"(</strong></a></td>
                                    </tr>
                                </tbody>
                            </table>
                        </td>
                    </tr>
                )";
    }

    NodeType GetNodeType() const override
    {
        return NodeType::BUTTON_NODE;
    }

private:
    std::string m_buttonText{};
    std::string m_buttonColor{};
    std::string m_buttonLink{};
};


// класс "Маркированный список", нужен для создания маркированных списков в рассылке
class LabeledListNode final : public ICampaignNode
{
public: 
    LabeledListNode() = default;
    LabeledListNode(
        const std::vector<std::string>& emojies,
        const std::vector<std::string>& texts
    )
        : m_listEmojies(emojies)
        , m_listTexts(texts)
    {
        if (emojies.size() != texts.size())
        {
            throw std::logic_error("Error: non-compatible labeled list");
        }
    }

    std::string GetHTMLRepresentation() override
    {
        if (m_listEmojies.empty() || m_listTexts.empty())
        {
            throw std::invalid_argument("Error: non-compatible labeled list");
        }

        // при наличии лишь одного узла в маркированном списке, пишем туда единственный первый элемент 
        // и не вставляем туда отступ после текста (редкий случай - LL с одним пунктом, но может быть)
        if (m_listEmojies.size() == 1)
        {
            return std::string(R"(
                        <tr>
                            <td align="left">
                                <table border="0" width="100%" cellspacing="0" cellpadding="0">
                                    <tbody>
                                        <tr>
                                            <td style="color: #3d3e47; font-weight: bold; font-size: 16px; line-height: 24px; )") +
                                            std::string(R"(font-family: Roboto, Helvetica, sans-serif;" valign="top" width="30" class="esd-text"> )") + 
                                            std::string(R"(<span style="color: #3d3e47; font-weight: bold; font-size: 16px; line-height: 24px; )") + 
                                            std::string(R"(font-family: Roboto, Helvetica, sans-serif;">)") + m_listEmojies[0] + std::string(R"(</span>
                                            </td>)") +
                                            std::string(R"(<td style="color: #3d3e47; font-weight: 400; font-size: 16px; line-height: 24px;)") + 
                                            std::string(R"(font-family: Roboto, Helvetica, sans-serif;" class="esd-text">)") + m_listTexts[0] + std::string(R"(
                                            </td>
                                        </tr>
                                    </tbody>
                                </table>
                            </td>
                        </tr>
                    )");
        }
                    
        // базовая обработка всех пунктов 
        std::string rawTemp{};
        rawTemp = std::string(R"(<tr>
                                    <td align="left">
                                        <table border="0" width="100%" cellspacing="0" cellpadding="0">
                                            <tbody>)");
        for (size_t k = 0; k < m_listEmojies.size(); ++k)
        {
            rawTemp += std::string(R"(
                                        <tr>
                                            <td style="color: #3d3e47; font-weight: bold; font-size: 16px; line-height: 24px; )") +
                                            std::string(R"(font-family: Roboto, Helvetica, sans-serif;" valign="top" width="30" class="esd-text"> )") + 
                                            std::string(R"(<span style="color: #3d3e47; font-weight: bold; font-size: 16px; line-height: 24px; )") + 
                                            std::string(R"(font-family: Roboto, Helvetica, sans-serif;">)") + m_listEmojies[k] + std::string(R"(</span>
                                            </td>)") +
                                            std::string(R"(<td style="color: #3d3e47; font-weight: 400; font-size: 16px; line-height: 24px;)") + 
                                            std::string(R"(font-family: Roboto, Helvetica, sans-serif;" class="esd-text">)") + m_listTexts[k] + std::string(R"(
                                            </td>
                                        </tr>
                                    )");
            if (k < m_listEmojies.size() - 1)
            {
                rawTemp += std::string(R"(
                                        <tr>
                                            <td style="font-size: 0; line-height: 5px;" colspan="2" height="5" class="esd-text">&nbsp;</td>
                                        </tr>
                                    )");
            }
        }

        rawTemp += std::string(R"(          </tbody>
                                        </table>
                                    </td>
                                </tr>)");

        return rawTemp;
    }

    NodeType GetNodeType() const override
    {
        return NodeType::LABELED_LIST_NODE;
    }

private:
    std::vector<std::string> m_listEmojies{};
    std::vector<std::string> m_listTexts{};
};


// класс "Точечный лист", нужен для создания маркированных точечных списков в рассылке
class DotListNode final : public ICampaignNode
{
public: 
    DotListNode() = default;
    DotListNode(
        const std::vector<std::string>& texts
    )
        : m_listTexts(texts)
    {}

    std::string GetHTMLRepresentation() override
    {
        if (m_listTexts.empty())
        {
            throw std::invalid_argument("Error: empty dot list");
        }

        if (m_listTexts.size() == 1)
        {
            return std::string(R"(
                        <tr>
                            <td align="left">
                                <table border="0" width="100%" cellspacing="0" cellpadding="0">
                                    <tbody>
                                        <tr>
                                            <td style="color: #3d3e47; font-weight: bold; font-size: 16px; line-height: 24px; )") +
                                            std::string(R"(font-family: Roboto, Helvetica, sans-serif;" valign="top" width="30" class="esd-text"> )") + 
                                            std::string(R"(<span style="color: #3d3e47; font-weight: bold; font-size: 24px; line-height: 24px; )") + 
                                            std::string(R"(font-family: Roboto, Helvetica, sans-serif;"> •)") + std::string(R"(</span>
                                            </td>)") +
                                            std::string(R"(<td style="color: #3d3e47; font-weight: 400; font-size: 16px; line-height: 24px;)") + 
                                            std::string(R"(font-family: Roboto, Helvetica, sans-serif;" class="esd-text">)") + m_listTexts[0] + std::string(R"(
                                            </td>
                                        </tr>
                                    </tbody>
                                </table>
                            </td>
                        </tr>
                    )");
        }
                    
        // базовая обработка всех пунктов 
        std::string rawTemp{};
        rawTemp = std::string(R"(<tr>
                                    <td align="left">
                                        <table border="0" width="100%" cellspacing="0" cellpadding="0">
                                            <tbody>)");
        for (size_t k = 0; k < m_listTexts.size(); ++k)
        {
            rawTemp += std::string(R"(
                                        <tr>
                                            <td style="color: #3d3e47; font-weight: bold; font-size: 16px; line-height: 24px; )") +
                                            std::string(R"(font-family: Roboto, Helvetica, sans-serif;" valign="top" width="30" class="esd-text"> )") + 
                                            std::string(R"(<span style="color: #3d3e47; font-weight: bold; font-size: 24px; line-height: 24px; )") + 
                                            std::string(R"(font-family: Roboto, Helvetica, sans-serif;"> •)") + std::string(R"(</span>
                                            </td>)") +
                                            std::string(R"(<td style="color: #3d3e47; font-weight: 400; font-size: 16px; line-height: 24px;)") + 
                                            std::string(R"(font-family: Roboto, Helvetica, sans-serif;" class="esd-text">)") + m_listTexts[k] + std::string(R"(
                                            </td>
                                        </tr>
                                    )");
            if (k < m_listTexts.size() - 1)
            {
                rawTemp += std::string(R"(
                                        <tr>
                                            <td style="font-size: 0; line-height: 5px;" colspan="2" height="5" class="esd-text">&nbsp;</td>
                                        </tr>
                                    )");
            }
        }

        rawTemp += std::string(R"(          </tbody>
                                        </table>
                                    </td>
                                </tr>)");

        return rawTemp;
    }

    NodeType GetNodeType() const override
    {
        return NodeType::DOT_LIST_NODE;
    }
    
private:
    std::vector<std::string> m_listTexts{};
};


// класс "Отзыв", нужен создания "плашек с текстом отзыва и подписью автора"
class ReviewNode final : public ICampaignNode
{
public:
    ReviewNode() = default;
    ReviewNode(
        const std::string& reviewText, 
        const std::string& reviewAuthor,
        const std::string& backgroundColor
    )
        : m_reviewText(reviewText)
        , m_reviewAuthor(reviewAuthor)
        , m_backgroundColor(backgroundColor)
    {
        if (m_backgroundColor[0] != '#' || m_backgroundColor.length() != 7)
        {
            throw std::invalid_argument("Invalid background color at \"Review node\": " + m_backgroundColor);
        }
    }

    std::string GetHTMLRepresentation() override
    {
        return R"(<tr>
                    <td align="left" style="border-top-right-radius: 10px; border-bottom-right-radius: 10px; border-bottom-left-radius: 10px; 
                    border-top-left-radius: 10px; background-color: )" + GetBrighterColorByBase(m_backgroundColor) + R"(; color: #3d3e47; font-weight: bold; font-size: 16px; 
                    line-height: 24px; font-family: Roboto, Helvetica, sans-serif; padding: 22px 24px 20px 24px;">
                        <table width="100%" border="0" cellpadding="0" cellspacing="0">
                            <tbody>
                                <tr>
                                    <td style="color: #3d3e47; font-weight: 400; font-size: 16px; line-height: 24px; 
                                    font-family: Roboto, Helvetica, Arial, sans-serif;" align="left" class="esd-text"><em>“)" + m_reviewText + R"(”</em></td>
                                </tr>
                                <tr>
                                    <td style="font-size: 0px; line-height: 5px; height: 5px;" class="esd-text"><br></td>
                                </tr>
                                <tr>
                                    <td style="color: #3d3e47; font-weight: 400; font-size: 16px; line-height: 24px; 
                                    font-family: Roboto, Helvetica, Arial, sans-serif;" align="right" class="esd-text">)" + m_reviewAuthor + R"(</td>
                                </tr>
                            </tbody>
                        </table>
                    </td>
                </tr>)";
    }

    NodeType GetNodeType() const override
    {
        return NodeType::REVIEW_NODE;
    }

private:
    std::string m_reviewText{};
    std::string m_reviewAuthor{};
    std::string m_backgroundColor{};
};


// класс "Отправитель", нужен для создания фотокарточек в конце писем. Берёт готовую вёрстку из текстового файла
// при отсутствии отправителя выбрасывается исключение: "Failed to open file: 'Anna Poli.txt'"
class SenderNode final : public ICampaignNode
{
public:
    SenderNode() = default;
    SenderNode(const std::string& targetFile) : m_fileName(targetFile + ".txt") {}

    std::string GetHTMLRepresentation() override
    {
        std::string sendersPath = "Senders/" + m_fileName;
        std::ifstream senderNodeFile{sendersPath};
        if (!senderNodeFile.is_open())
        {
            std::string localPath = m_fileName;
            senderNodeFile.open(localPath);
            if (!senderNodeFile.is_open())
            {
                throw std::invalid_argument("Failed to open file: " + m_fileName + ".txt (checked Senders/ and current dir)");
            }
        }

        std::string temporaryBuffer{}; // копируем весь файл в строку при помощи итераторов
        temporaryBuffer.assign(std::istreambuf_iterator<char>(senderNodeFile), std::istreambuf_iterator<char>());
        senderNodeFile.close();

        return temporaryBuffer;
    }

    NodeType GetNodeType() const override
    {
        return NodeType::SENDER_NODE;
    }

private:
    std::string m_fileName{};
};


// класс "Ряд кнопок", используется для создания узлов, где несколько кнопок расположено в ряд
class ButtonsRowNode final : public ICampaignNode
{
public:
    ButtonsRowNode() = default;
    ButtonsRowNode(
        const std::vector<std::string>& buttonTexts,
        const std::vector<std::string>& buttonLinks,
        std::string buttonTextColor
    )
        : m_buttonTexts(buttonTexts)
        , m_buttonLinks(buttonLinks)
        , m_buttonTextColor(buttonTextColor)
    {
        if (m_buttonLinks.size() != m_buttonTexts.size())
        {
            throw std::invalid_argument("Error: non-compatible buttons row");
        }

        if (m_buttonTextColor[0] != '#' || m_buttonTextColor.length() != 7)
        {
            throw std::invalid_argument("Invalid background color at \"ButtonsRowNode\"");
        }

        if (m_buttonTexts.size() < 2)
        {
            throw std::invalid_argument("Buttons row must contain at least two buttons");
        }

        if (m_buttonTexts.size() > 5)
        {
            throw std::invalid_argument("Too many buttons in row. Maximum is 5");
        }
    }

    std::string GetHTMLRepresentation() override
    {
        std::string rawTemp{};
        rawTemp += std::string(R"(<tr>
                                    <td align="center">
                                        <table style="border-collapse: collapse;" border="0" width="100%" cellspacing="0" cellpadding="0">
                                            <tbody>
                                                <tr>)");

        for (size_t k = 0; k < m_buttonTexts.size(); ++k)
        {
            rawTemp += std::string(R"(
                                        <th style="padding: 0; border-radius: 6px;" bgcolor=")" + GetBrighterColorByBase(m_buttonTextColor) + R"(">
                                            <a href=")" + m_buttonLinks[k] + R"(" target="_blank" style="display: block; background-color: )" + 
                                            GetBrighterColorByBase(m_buttonTextColor) + R"(; border-radius: 6px; padding: 14px 7px; 
                                            font-weight: bold; font-size: 16px; line-height: 20px; font-family: Roboto, Helvetica, sans-serif; 
                                            color: )" + m_buttonTextColor + R"(; text-align: center; text-decoration: none;">
                                                <strong style="color: )" + m_buttonTextColor + R"(;">)" + m_buttonTexts[k] + R"(</strong>
                                            </a>
                                        </th>)"
                                    );
            if (k < m_buttonTexts.size() - 1)
            {
                rawTemp += std::string(R"(<th style="width: 10px;" width="10"></th>)");
            }
        }

        rawTemp += std::string(R"(              </tr>
                                            </tbody>
                                        </table>
                                    </td>
                                </tr>)");
        
        return rawTemp;
    }

    NodeType GetNodeType() const override
    {
        return NodeType::BUTTONS_ROW_NODE;
    }

private:
    std::vector<std::string> m_buttonTexts;
    std::vector<std::string> m_buttonLinks;
    std::string m_buttonTextColor{};
};


// класс "Переменная из CRM", представляет собой ассоциативный массив, который хранит в себе
// взаимно однозначное соответствие идентификаторов переменных из CRM и их краткому человеко-понятному имени, н-р:
// CurrentYear %FOOTER_CURRENT_YEAR%
class VariableFromCRMNode final : public ICampaignNode
{
public:
    VariableFromCRMNode() = default;
    VariableFromCRMNode(const std::string& variableRequest) : m_variableRequest(variableRequest) 
    {
        PrepareVariablesDictionary();
    }

    VariableFromCRMNode(
        const std::string& variableRequest,
        const std::string& assignFileName
    ) 
        : m_variableRequest(variableRequest)
        , m_assignFileName(assignFileName)
    {
        PrepareVariablesDictionary();
    }

    std::string GetHTMLRepresentation() override
    {
        std::string innerRequest = m_variableRequest;
        std::transform(innerRequest.begin(), innerRequest.end(), innerRequest.begin(), [](unsigned char ch){ return std::tolower(ch); });
        auto iter = m_variablesBySynonym.find(innerRequest);
        if (iter != m_variablesBySynonym.end())
        {
            return iter->second;
        }

        throw std::runtime_error("Unknown variable synonym: " + m_variableRequest);
    }

    NodeType GetNodeType() const override
    {
        return NodeType::VARIABLE_FROM_CRM_NODE;
    }

private:
    std::string m_variableRequest{}; // [VC:variableRequest]
    std::string m_assignFileName{"assign.txt"};
    std::unordered_map<std::string, std::string> m_variablesBySynonym{};

    void PrepareVariablesDictionary()
    {
        std::ifstream assignFile{m_assignFileName};
        if (!assignFile.is_open())
        {
            throw std::runtime_error("Failed to open assign file");
        }

        std::string line{};
        while (std::getline(assignFile, line))
        {
            if (line.empty())
            {
                continue;
            }

            std::istringstream iss(line);
            std::string synonym{}, variable{};
            if (iss >> synonym >> variable)
            {
                std::transform(synonym.begin(), synonym.end(), synonym.begin(), [](unsigned char ch){ return std::tolower(ch); });
                m_variablesBySynonym[synonym] = variable;
            }
        }
    }
};


#endif