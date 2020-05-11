#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cassert>

constexpr auto APPLICATION_TITLE = "Parallax Cubemap VMT Tool v0.4";
// It's 0.5 because this is the 3rd proper "version" of sorts:
// 0.1 was the initial working tool which could open a file and create the complementary VMT
// 0.2 was multiple files
// 0.3 fixed a problem where the program would exit on an invalid file instead of continuing
// 0.4 is some code cleanup and a bit of const here and there
// 0.5 is further code cleanup and breaking up into more functions
// TODO: changelog instead of this block of text

enum class EMessagePrefix
{
    None,
    Warn,
    Err
};

enum class EDetectedShader
{
    None,
    LightmappedGeneric,
    SDK_LightmappedGeneric
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

std::string SetFileSuffix(const EDetectedShader &eShaderType, const std::string &strExportPath)
{
    if (eShaderType == EDetectedShader::LightmappedGeneric)
    {
        PrintLine("Exporting PCC (SDK_LightmappedGeneric): " + strExportPath + "_pcc.vmt");
        return strExportPath + "_pcc.vmt";
    }
    if (eShaderType == EDetectedShader::SDK_LightmappedGeneric)
    {
        PrintLine("Exporting Regular LightmappedGeneric: " + strExportPath + "_no_pcc.vmt");
        return strExportPath + "_no_pcc.vmt";
    }
    // FIXME: I get a warning about some paths not returning, so I am going to place an assert here at this time
    else
    {
        assert(0 && "An invalid shader was passed into the Suffix function!");
        // I was going to return a nullptr but as this is used inside an ofstream i will play it safe
        // and just pass a string back with 'error' in it to indicate something has gone wrong :V
        return strExportPath + "_error.vmt";
    }
}

void CreateVmtFile(const std::string &strExportPath, const std::stringstream &isRestOfVmt, const EDetectedShader &eShaderType)
{
    // TODO: Remove existing suffix from input file? Maybe just tell people about the suffix in the Usage dialog?

    std::ofstream ofNewVmtFile(SetFileSuffix(eShaderType, strExportPath));

    if (eShaderType == EDetectedShader::LightmappedGeneric)
    {
        ofNewVmtFile << "\"SDK_LightmappedGeneric\"\n";
    }
    else if (eShaderType == EDetectedShader::SDK_LightmappedGeneric)
    {
        ofNewVmtFile << "\"LightmappedGeneric\"\n";
    }

    ofNewVmtFile << isRestOfVmt.rdbuf();
    ofNewVmtFile.close();
}

// I suppose this is one way of making the path
// TODO: Look at <filesystem> for potentially better ways of doing this
std::string MakeExportPathString(std::filesystem::path inputPath)
{
    // Get filename explicitly, no path or extension via stem()
    std::string strFileNameNoExtension(inputPath.stem().string());

    // Get path through removing filename from it - i'm just trying these things out and keeping them if they seem useful
    std::string strPathNoFileName(inputPath.remove_filename().string());

    return strPathNoFileName + strFileNameNoExtension;
}

// TODO: Any algorithms etc for better checking than this? Is the current solution okay?
EDetectedShader DetectFileShader(std::string &strFirstLine)
{
    if (strFirstLine == "sdk_lightmappedgeneric")
    {
        PrintLine("Found SDK_LightmappedGeneric");
        return EDetectedShader::SDK_LightmappedGeneric;
    }
    if (strFirstLine == "lightmappedgeneric")
    {
        PrintLine("Found LightmappedGeneric");
        return EDetectedShader::LightmappedGeneric;
    }

    // TODO: Make user specify?
    PrintLine("Expected SDK_LightmappedGeneric or LightmappedGeneric as first line in file.", EMessagePrefix::Err);
    return EDetectedShader::None;
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

    // For loop to iterate each input file, it's a big one
    // TODO: Break this up further?
    for (int i = 1; i < argc; i++)
    {
        PrintLine("Argument " + std::to_string(i) + ": " + argv[i]);

        // Messing about with filesystem to figure out paths
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

            std::string strFirstLine;
            std::getline(ifVmtFile, strFirstLine);

            std::stringstream isRestOfFile;
            for (std::string str; std::getline(ifVmtFile, str);)
            {
                isRestOfFile << str << "\n";
            }
            ifVmtFile.close();

            // Code snippets that hopefully remove the quotes and whitespace, this is ridiculous for such a simple thing tbh
            // TODO: Look into algorithms/etc that may be better?
            strFirstLine.erase(std::remove(strFirstLine.begin(), strFirstLine.end(), '\"'), strFirstLine.end());
            strFirstLine.erase(std::remove_if(strFirstLine.begin(), strFirstLine.end(), isspace), strFirstLine.end());

#pragma warning(push)
#pragma warning(disable: 4244)
            // Snippet found online, the ::tolower part is quite interesting
            std::transform(strFirstLine.begin(), strFirstLine.end(), strFirstLine.begin(), ::tolower);
#pragma warning(pop)

            EDetectedShader eFoundShader = DetectFileShader(strFirstLine);

            if (eFoundShader != EDetectedShader::None)
            {
                CreateVmtFile(MakeExportPathString(pathFilesystemInputPath), isRestOfFile, eFoundShader);
            }
        }
        else
        {
            // TODO: Can we get a more descriptive error than this generic one?
            PrintLine("Couldn't Open File: " + strInputPath, EMessagePrefix::Err);
        }
    }
}
