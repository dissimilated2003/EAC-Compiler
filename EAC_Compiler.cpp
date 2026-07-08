#include "EAC_CampaignNodes.h"
#include "EAC_RegionsInfo.h"
#include "EAC_CampaignBuilder.h"
#include "EAC_TokenstreamParser.h"


#include <fstream>
#include <filesystem>
#include <stdexcept>


struct Args
{
    Args() = default;
    Args(
        const std::string& reg,         // параметр региона
        const std::string& docxFile,    // путь до .docx-файла, в котором задано формальное представление EAC
        const std::string& theme        // цветовая тема (нужна для кнопок, ссылок и т.д.)
    )
        : region(reg)
        , targetDocxFile(docxFile)
        , themeColor(theme)
    {}

    std::string region;
    std::string targetDocxFile;
    std::string themeColor;
};


bool HasDocxExtension(const std::string& filename)
{
    namespace fs = std::filesystem;
    fs::path filePath(filename);
    std::string fileExtension = filePath.extension().string();
    std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);
    return fileExtension == ".docx";
}


[[ nodiscard ]] Args ParseArgs(int argc, char* argv[])
{
    if (argc != 4)
    {
        throw std::invalid_argument("Invalid number of arguments");
    }

    Args temporaryArgs{};
    temporaryArgs.region = argv[1];

    temporaryArgs.targetDocxFile = argv[2];
    if (!HasDocxExtension(temporaryArgs.targetDocxFile))
    {
        throw std::invalid_argument("Invalid input file format (.docx expected)");
    }
    
    std::ifstream targetFile{temporaryArgs.targetDocxFile};
    if (!targetFile.is_open())
    {
        throw std::invalid_argument("No such file or directory");
    }
    targetFile.close();

    temporaryArgs.themeColor = "#" + std::string(argv[3]);
    if (temporaryArgs.themeColor.length() != 7)
    {
        throw std::invalid_argument("Invalid color. Correct example: ff00ee, with no '#'");
    }

    return temporaryArgs;
}


void PrintArgs(const Args& args)
{
    std::cout << args.region << "\n";
    std::cout << args.targetDocxFile << "\n";
    std::cout << args.themeColor << "\n";
}


void BuildTestScenario(std::unique_ptr<ICampaignBuilder>& builder, const Args& args)
{
    std::unique_ptr<ICampaignNode> vc1 = std::make_unique<VariableFromCRMNode>("Name");

    std::vector<std::unique_ptr<ICampaignNode>> nodes{};
        
    nodes.push_back(std::make_unique<ParagraphNode>(std::string("Hi " + vc1->GetHTMLRepresentation())));
    nodes.push_back(std::make_unique<SpacingNode>(20));
    nodes.push_back(std::make_unique<ParagraphNode>(std::string(
        R"(If you couldn't make it to our #iSpringNetEvent or you just want to watch it again — <a style="color: )" + 
        args.themeColor +  R"(; font-weight: 700;" href="https://youtu.be/zsfIPZKhb7k" target="_blank">click here to access the recording</a>.)" 
    )));
    nodes.push_back(std::make_unique<SpacingNode>(20));
    nodes.push_back(std::make_unique<ParagraphNode>(std::string(
        R"(We also wanted to say thank you once more for stopping by the iSpring booth at ATD26 in Los Angeles. It really was a pleasure to meet you.)"
    )));
    nodes.push_back(std::make_unique<SpacingNode>(20));
    nodes.push_back(std::make_unique<ParagraphNode>(std::string(
        R"(And just a quick reminder: you can always book a free consultation with an iSpring expert to talk about your goals and how iSpring can help.)"
    )));
    nodes.push_back(std::make_unique<SpacingNode>(35));
    nodes.push_back(std::make_unique<ButtonNode>("Book a free call", args.themeColor, R"(https://www.ispring.com/demo)"));
    nodes.push_back(std::make_unique<SpacingNode>(35));
    nodes.push_back(std::make_unique<ParagraphNode>(std::string(
        R"(We're not saying goodbye. We hope to see you at future events — online or in person.)"
    )));
    nodes.push_back(std::make_unique<SpacingNode>(60));
    nodes.push_back(std::make_unique<ParagraphNode>(std::string(
        R"(Best,<br>The iSpring Team)"
    )));

    for (auto& node : nodes)
    {
        builder->AddNode(std::move(node));
    }
}


// как передать параметр региона рассылки? argv[1] = region
// вызов:               .\EmailACCompiler en-emea-na
// просмотр регионов:   .\EmailACCompiler <любой неверный регион>
int main(int argc, char* argv[])
{
    Args args = ParseArgs(argc, argv);
    
    LaunchLexicalStreamParser("EAC_LexicalstreamParser.py", args.targetDocxFile, args.themeColor);
    auto builder = std::make_unique<BaseCampaignBuilder>(args.region);
    TokenstreamParser parser{*builder, args.themeColor};
    parser.ParseTokenstream("tokenstream.txt");
    
    std::ofstream result{"result.html"};
    result << builder->BuildCampaignHTML();
    std::cout << "Campaign HTML builded into result.html\n";

    return 0;
}