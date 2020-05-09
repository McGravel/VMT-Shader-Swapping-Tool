#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

constexpr auto APPLICATION_TITLE = "Parallax Cubemap VMT Tool v0.3";
// It's 0.3 because this is the 3rd proper "version" of sorts:
// 0.1 was the initial working tool which could open a file and create the complementary VMT
// 0.2 was multiple files
// 0.3 fixed a problem where the program would exit on an invalid file instead of continuing
// No changelog or anything because this isn't THAT important lol but if it goes on then yeah this'll go somewhere else

enum class EMessagePrefix
{
    None,
    Warn,
    Err
};

// TODO: Better way of doing this? Classes? Using one of the Map things to assign a string to enums?
void PrintLine(const std::string &strToPrint, EMessagePrefix eMsgPrefix = EMessagePrefix::None)
{
    if (eMsgPrefix == EMessagePrefix::None)
    {
        std::cout << strToPrint + "\n";
    }
    if (eMsgPrefix == EMessagePrefix::Warn)
    {
        std::cout << "[WARNING] " + strToPrint + "\n";
    }
    if (eMsgPrefix == EMessagePrefix::Err)
    {
        std::cout << "[ERROR] " + strToPrint + "\n";
    }
}

void CreateVmtFile(const std::string &strExportPath, const std::stringstream &isRestOfVmt, const bool &bIsPccFile)
{
    // TODO: Remove existing suffix from input file? Maybe just tell people about the suffix in the Usage dialog?

    std::string strOutputDestination = strExportPath;

    if (bIsPccFile)
    {
        strOutputDestination += "_pcc.vmt";
        PrintLine("Exporting PCC (SDK_LightmappedGeneric): " + strOutputDestination);
    }
    else
    {
        strOutputDestination += "_no_pcc.vmt";
        PrintLine("Exporting Regular LightmappedGeneric: " + strOutputDestination);
    }

    std::ofstream ofNewVmtFile(strOutputDestination);

    if (bIsPccFile)
    {
        ofNewVmtFile << "\"SDK_LightmappedGeneric\"\n";
    }
    else
    {
        ofNewVmtFile << "\"LightmappedGeneric\"\n";
    }

    ofNewVmtFile << isRestOfVmt.rdbuf();
    ofNewVmtFile.close();
}

// TODO: Look at <filesystem> for potentially better ways of doing this
std::string MakeExportPathString(std::filesystem::path inputPath)
{
    // Get filename explicitly, no path or extension via stem()
    std::string strFileNameNoExtension(inputPath.stem().string());

    // Get path through removing filename from it - i'm just trying these things out and keeping them if they seem useful
    std::string strPathNoFileName(inputPath.remove_filename().string());

    // I suppose this is one way of making the path
    return (strPathNoFileName + strFileNameNoExtension);
}

int main(int argc, char *argv[])
{
    PrintLine(APPLICATION_TITLE);
    if (argc < 2)
    {
        PrintLine("Sorry, you need to pass a path via the command line or drag-and-drop!\n", EMessagePrefix::Err);
        PrintLine("Usage:\nTo specify a file on command line try:\nparallax_cubemap_vmt_tool \"path_to_file\\file_name.vmt\"");
        exit(0);
    }

    // In an attempt to make this program support multiple files (not folders atm) I just put the whole thing in a for loop
    // And it works, so once again, it could likely be done much better but regardless it seems to function as intended
    // Maybe this tool could be much faster but given its simplicity it likely is sufficient as-is
    for (int i = 1; i < argc; i++)
    {
        PrintLine("Argument " + std::to_string(i) + ": " + argv[i]);

        // Messing about with filesystem to figure out paths
        // This may be a poor way of doing it but it's my first time with files in C++
        std::filesystem::path pathFilesystemInputPath{ argv[i] };

        // Change path to str to allow it to be passed into existing code
        std::string strInputPath(pathFilesystemInputPath.string());

        if (strInputPath.empty())
        {
            PrintLine("Empty path specified.", EMessagePrefix::Err);
            continue;
        }

        std::ifstream ifVmtFile(strInputPath);
        if (ifVmtFile.is_open())
        {
            PrintLine("Successfully Opened: " + strInputPath);

            bool bReadingFirstLine = true;
            std::string strFirstLine;
            std::stringstream isRestOfFile;
            // Read the file line-by-line, which is one way of getting the first line exclusively to check which Shader is used
            // TODO: Can we getline() for just the first line, then just get the rest in one go?
            for (std::string str; std::getline(ifVmtFile, str);)
            {
                if (bReadingFirstLine)
                {
                    strFirstLine = str;
                    bReadingFirstLine = false;
                }
                else
                {
                    isRestOfFile << str << "\n";
                }
            }
            ifVmtFile.close();

            // Code snippets that hopefully remove the quotes and whitespace, this is ridiculous for such a simple thing tbh
            // TODO: Look into algorithms/etc that may be better?
            strFirstLine.erase(std::remove(strFirstLine.begin(), strFirstLine.end(), '\"'), strFirstLine.end());
            strFirstLine.erase(std::remove_if(strFirstLine.begin(), strFirstLine.end(), isspace), strFirstLine.end());

            // Snippet found online, the ::tolower part is quite interesting
            std::transform(strFirstLine.begin(), strFirstLine.end(), strFirstLine.begin(), ::tolower);

            // TODO: Any algorithms etc for better checking than this? Is the current solution okay?
            bool bExportingPccFile = true;
            if (strFirstLine == "sdk_lightmappedgeneric")
            {
                PrintLine("Found SDK_LightmappedGeneric");
                bExportingPccFile = false;
            }
            else if (strFirstLine == "lightmappedgeneric")
            {
                PrintLine("Found LightmappedGeneric");
            }
            else
            {
                // TODO: Make user specify?
                PrintLine("Expected SDK_LightmappedGeneric or LightmappedGeneric as first line in file.", EMessagePrefix::Err);
                continue;
            }

            CreateVmtFile(MakeExportPathString(pathFilesystemInputPath), isRestOfFile, bExportingPccFile);
        }
        else
        {
            // TODO: Can we get a more descriptive error than this generic one?
            PrintLine("Couldn't Open File: " + strInputPath, EMessagePrefix::Err);
        }
    }
}
