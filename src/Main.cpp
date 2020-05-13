#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

constexpr auto APPLICATION_TITLE = "Parallax Cubemap VMT Tool v0.6";

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

// Returns full path with filename and extension
// e.g. C:/Users/blah/folder/output_file_pcc.vmt
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
    assert(0 && "An invalid shader was passed into the Suffix function!");
    // Tested assert(), looks like the program is terminated? But just in case, i will leave this line here
    return strExportPath + "_error.vmt";
}

// Creates a new VMT file with the first line changed; returns boolean to try and signify this worked as expected
bool CreateVmtFile(const std::string &strExportPath, const std::stringstream &isRestOfVmt, const EDetectedShader &eShaderType)
{
    // TODO: Remove existing suffix from input file? Maybe just tell people about the suffix in the Usage dialog?

    // This is the one place I can put this without messages being duplicated for now
    // TODO: We need to ask the user in this case if this is what they want
    std::string strOutputPath = SetFileSuffix(eShaderType, strExportPath);
    if (std::filesystem::exists(strOutputPath))
    {
        PrintLine("Overwriting an existing file!", EMessagePrefix::Warn);
    }

    std::ofstream ofNewVmtFile(strOutputPath);

    if (eShaderType == EDetectedShader::LightmappedGeneric)
    {
        ofNewVmtFile << "\"SDK_LightmappedGeneric\"\n";
    }
    else if (eShaderType == EDetectedShader::SDK_LightmappedGeneric)
    {
        ofNewVmtFile << "\"LightmappedGeneric\"\n";
    }
    else
    {
        // An unknown shader was passed in so close it firstly then return false to signify error
        ofNewVmtFile.close();
        assert(0 && "An invalid shader was passed into the CreateVmtFile function!");
        // TODO: Delete this file instead of leaving an invalid/empty one?
        return false;
    }

    ofNewVmtFile << isRestOfVmt.rdbuf();
    ofNewVmtFile.close();
    return true;
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

void ReadLinesFromFile(std::ifstream &ifVmtFile, std::stringstream &isRestOfFile)
{
    for (std::string str; std::getline(ifVmtFile, str);)
    {
        isRestOfFile << str << "\n";
    }
    ifVmtFile.close();
}

// This function will transform the string to allow for easier use by DetectFileShader() and potentially elsewhere
void ValidateShaderName(std::string &strFirstLine)
{
    // Code snippets that hopefully remove the quotes and whitespace, this is ridiculous for such a simple thing tbh
    // TODO: Look into algorithms/etc that may be better?
    strFirstLine.erase(std::remove(strFirstLine.begin(), strFirstLine.end(), '\"'), strFirstLine.end());
    strFirstLine.erase(std::remove_if(strFirstLine.begin(), strFirstLine.end(), isspace), strFirstLine.end());

#pragma warning(push)
#pragma warning(disable : 4244)
    // Snippet found online, the ::tolower part is quite interesting
    std::transform(strFirstLine.begin(), strFirstLine.end(), strFirstLine.begin(), ::tolower);
#pragma warning(pop)
}

int main(int argc, char *argv[])
{
    PrintLine(APPLICATION_TITLE);
    if (argc < 2)
    {
        PrintLine("Sorry, you need to pass a path via the command line or drag-and-drop!\n", EMessagePrefix::Err);
        PrintLine("Usage:\nTo specify a file on command line try:\nparallax_cubemap_vmt_tool \"path_to_file\\file_name.vmt\"");
        // exit() should be ok here since we've not done anything yet
        exit(0);
    }

    // For loop to iterate each input file, it's a big one
    // TODO: Break this up further?
    int iSuccessfulFileWrites = 0;
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

        if (pathFilesystemInputPath.extension().string() != ".vmt")
        {
            PrintLine("The input file is not of file type .VMT!", EMessagePrefix::Err);
            continue;
        }

        std::ifstream ifVmtFile(strInputPath);
        if (ifVmtFile.is_open())
        {
            PrintLine("Successfully Opened: " + strInputPath);

            // The first line of the file is put into a separate var
            std::string strFirstLine;
            std::getline(ifVmtFile, strFirstLine);
            ValidateShaderName(strFirstLine);

            // Then we place the rest of the file in another var
            std::stringstream isRestOfFile;
            ReadLinesFromFile(ifVmtFile, isRestOfFile);

            EDetectedShader eFoundShader = DetectFileShader(strFirstLine);

            // TODO: Seems like there's a bit of if() nesting going on
            // Do all these called functions really need to check as much as they currently are? See CreateVmtFile
            if (eFoundShader != EDetectedShader::None)
            {
                // CreateVmtFile returns a bool to try and ensure the amount of successful file writes shown is correct
                if (CreateVmtFile(MakeExportPathString(pathFilesystemInputPath), isRestOfFile, eFoundShader))
                {
                    iSuccessfulFileWrites++;
                }
            }
        }
        else
        {
            // TODO: Can we get a more descriptive error than this generic one?
            PrintLine("Couldn't Open File: " + strInputPath, EMessagePrefix::Err);
        }
    }
    PrintLine(std::to_string(iSuccessfulFileWrites) + "/" + std::to_string(argc - 1) + " Files Written.");
}
