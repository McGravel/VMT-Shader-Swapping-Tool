/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <fmt/core.h>
#include <fmt/color.h>

constexpr auto APPLICATION_TITLE = "VMT Shader Swapping Tool v0.7";

enum class EMessagePrefix
{
    None,
    Warn,
    Err,
    Success,
    Info
};

enum class EDetectedShader
{
    None,
    LightmappedGeneric,
    SDK_LightmappedGeneric,
    VertexLitGeneric
};

enum class EPccOrVlgResponse
{
    None,
    Pcc,
    Vlg,
    PccDontAskAgain,
    VlgDontAskAgain
};

void PrintLine(const std::string &strToPrint, EMessagePrefix eMsgPrefix = EMessagePrefix::None)
{
    if (eMsgPrefix == EMessagePrefix::None)
    {
        //std::cout << strToPrint + "\n";
        fmt::print("{}\n", strToPrint);
    }
    if (eMsgPrefix == EMessagePrefix::Warn)
    {
        fmt::print(fmt::fg(fmt::color::yellow) | fmt::emphasis::bold, "[WARNING]: {}\n", strToPrint);
    }
    if (eMsgPrefix == EMessagePrefix::Err)
    {
        fmt::print(fmt::fg(fmt::color::orange_red) | fmt::emphasis::bold, "[ERROR]: {}\n", strToPrint);
    }
    if (eMsgPrefix == EMessagePrefix::Success)
    {
        fmt::print(fmt::fg(fmt::color::sea_green) | fmt::emphasis::bold, "[SUCCESS]: {}\n", strToPrint);
    }
    if (eMsgPrefix == EMessagePrefix::Info)
    {
        fmt::print(fmt::fg(fmt::color::dodger_blue) | fmt::emphasis::bold, "[INFO]: {}\n", strToPrint);
    }
}

EPccOrVlgResponse PromptLmgMode()
{
    while (true)
    {
        int iResponse = 0;
        PrintLine("LightmappedGeneric Detected. Please pick which shader to use:", EMessagePrefix::Info);
        PrintLine("1: Parallax-Corrected Cubemap (SDK_LightmappedGeneric)\n"
                  "2: VertexLitGeneric\n"
                  "3: Parallax-Corrected Cubemap (SDK_LightmappedGeneric) (Don't ask again)\n"
                  "4: VertexLitGeneric (Don't ask again)");

        std::cin >> iResponse;
        if (iResponse > 0 && iResponse < 5)
        {
            switch (iResponse)
            {
            case 1:
                return EPccOrVlgResponse::Pcc;
            case 2:
                return EPccOrVlgResponse::Vlg;
            case 3:
                PrintLine("Outputting all subsequent LightmappedGeneric files as Parallax-Corrected Cubemap (SDK_LightmappedGeneric).", EMessagePrefix::Info);
                return EPccOrVlgResponse::PccDontAskAgain;
            case 4:
                PrintLine("Outputting all subsequent LightmappedGeneric files as VertexLitGeneric.", EMessagePrefix::Info);
                return EPccOrVlgResponse::VlgDontAskAgain;
            default:
                assert(0 && "User Input Broke!");
                break;
            }
        }
        std::cin.clear();
        std::cin.ignore(10000, '\n');
    }
}

EPccOrVlgResponse CheckLmgMode(const EPccOrVlgResponse &inputMode)
{
    if (inputMode != EPccOrVlgResponse::None)
    {
        return inputMode;
    }

    return PromptLmgMode();
}

// Return base version of EPccOrVlgResponse from DontAskAgain version if it is set to either one
EPccOrVlgResponse GetPccOrVlg(const EPccOrVlgResponse &inputType)
{ 
    if (inputType == EPccOrVlgResponse::PccDontAskAgain || inputType == EPccOrVlgResponse::VlgDontAskAgain)
    {
        if (inputType == EPccOrVlgResponse::PccDontAskAgain)
        {
            PrintLine("Outputting as Parallax-Corrected Cubemap (SDK_LightmappedGeneric), as we're not asking again.");
            return EPccOrVlgResponse::Pcc;
        }
        if (inputType == EPccOrVlgResponse::VlgDontAskAgain)
        {
            PrintLine("Outputting as VertexLitGeneric, as we're not asking again.");
            return EPccOrVlgResponse::Vlg;
        }
    }
    else
    {
        return inputType;
    }
}

EDetectedShader DetectFileShader(const std::string &strFirstLine)
{
    if (strFirstLine == "sdk_lightmappedgeneric")
    {
        return EDetectedShader::SDK_LightmappedGeneric;
    }
    if (strFirstLine == "lightmappedgeneric")
    {
        return EDetectedShader::LightmappedGeneric;
    }
    if (strFirstLine == "vertexlitgeneric")
    {
        return EDetectedShader::VertexLitGeneric;
    }

    PrintLine("Expected SDK_LightmappedGeneric, LightmappedGeneric, or VertexLitGeneric as the first line in file.",
              EMessagePrefix::Err);
    return EDetectedShader::None;
}

std::string MakeExportPathString(std::filesystem::path inputPath)
{
    // Get filename explicitly, no path or extension via stem()
    std::string strFileNameNoExtension(inputPath.stem().string());

    // Get path through removing filename from it
    std::string strPathNoFileName(inputPath.remove_filename().string());

    return strPathNoFileName + strFileNameNoExtension;
}

// Returns full path with filename and extension
// e.g. C:/Users/blah/folder/output_file_pcc.vmt
std::string SetFileSuffix(const EDetectedShader &eShaderType, const std::string &strExportPath, const bool &bSuffix)
{
    if (eShaderType == EDetectedShader::LightmappedGeneric)
    {
        if (!bSuffix)
        {
            PrintLine("Exporting Parallax-Corrected Cubemap (SDK_LightmappedGeneric): " + strExportPath + "_pcc.vmt", EMessagePrefix::Info);
            return strExportPath + "_pcc.vmt";
        }
        else
        {
            PrintLine("Overwriting as Parallax-Corrected Cubemaps (SDK_LightmappedGeneric): " + strExportPath + ".vmt", EMessagePrefix::Info);
            return strExportPath + ".vmt";
        }
    }

    if (eShaderType == EDetectedShader::SDK_LightmappedGeneric || eShaderType == EDetectedShader::VertexLitGeneric)
    {
        if (!bSuffix)

        {
            PrintLine("Exporting LightmappedGeneric: " + strExportPath + "_lmg.vmt", EMessagePrefix::Info);
            return strExportPath + "_lmg.vmt";
        }
        else
        {
            PrintLine("Overwriting as LightmappedGeneric: " + strExportPath + ".vmt", EMessagePrefix::Info);
            return strExportPath + ".vmt";
        }
    }

    assert(0 && "An invalid shader was passed into the Suffix function!");
    return strExportPath + "_error.vmt";
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
    strFirstLine.erase(std::remove(strFirstLine.begin(), strFirstLine.end(), '\"'), strFirstLine.end());
    strFirstLine.erase(std::remove_if(strFirstLine.begin(), strFirstLine.end(), isspace), strFirstLine.end());

#pragma warning(push)
#pragma warning(disable : 4244)
    // Snippet found online, the ::tolower part is quite interesting
    std::transform(strFirstLine.begin(), strFirstLine.end(), strFirstLine.begin(), ::tolower);
#pragma warning(pop)
}

bool PromptYesNo()
{ 
    while (true)
    {
        std::string strResponse;
        PrintLine("Y/N?");
        std::cin >> strResponse;

        if (strResponse == "Y" || strResponse == "y")
        {
            return true;
        }
        if (strResponse == "N" || strResponse == "n")
        {
            return false;
        }
    }
}

// Creates a new VMT file with the first line changed; returns boolean to try and signify this worked as expected
bool CreateVmtFile(const std::string &strExportPath, const std::stringstream &isRestOfVmt, const EDetectedShader &eShaderType)
{
    PrintLine("Directly overwrite instead of appending a suffix and creating a new file?");
    bool bHasSuffix = PromptYesNo();
    std::string strOutputPath = SetFileSuffix(eShaderType, strExportPath, bHasSuffix);

    if (std::filesystem::exists(strOutputPath) && !bHasSuffix)
    {
        PrintLine("Overwriting an existing file!", EMessagePrefix::Warn);
        PrintLine("Confirm overwriting the file?");
        bool bConfirmOverwrite = PromptYesNo();
        if (!bConfirmOverwrite)
        {
            PrintLine("Overwrite Cancelled.", EMessagePrefix::Info);
            return false;
        }
    }

    std::ofstream ofNewVmtFile(strOutputPath);

    if (eShaderType == EDetectedShader::LightmappedGeneric)
    {
        // Static mode of operation that persists between calls
        // eMode is used to see if PCC or VLG is in "don't ask again" mode
        static EPccOrVlgResponse eMode = CheckLmgMode(eMode);
        // Then get the regular version if it's not already not a "don't ask again" variant
        EPccOrVlgResponse eResponse = GetPccOrVlg(eMode);

        if (eResponse == EPccOrVlgResponse::Pcc)
        {
            ofNewVmtFile << "\"SDK_LightmappedGeneric\"\n";
        }
        else if (eResponse == EPccOrVlgResponse::Vlg)
        {
            ofNewVmtFile << "\"VertexLitGeneric\"\n";
        }
        else
        {
            // I don't think the assert outside this block will catch this
            assert(0 && "Invalid Option Returned from User Input");
        }
    }
    else if (eShaderType == EDetectedShader::SDK_LightmappedGeneric || eShaderType == EDetectedShader::VertexLitGeneric)
    {
        ofNewVmtFile << "\"LightmappedGeneric\"\n";
    }
    else
    {
        // An unknown shader was passed in so close it firstly then return false to signify error
        ofNewVmtFile.close();
        assert(0 && "An invalid shader was passed into the CreateVmtFile function!");
        // Delete this file instead of leaving an invalid/empty one?
        return false;
    }

    ofNewVmtFile << isRestOfVmt.rdbuf();
    ofNewVmtFile.close();
    PrintLine("File successfully written: " + strOutputPath, EMessagePrefix::Success);
    return true;
}

int main(int argc, char *argv[])
{
    PrintLine(APPLICATION_TITLE);
    if (argc < 2)
    {
        PrintLine("Sorry, you need to pass a path via the command line or drag-and-drop!\n", EMessagePrefix::Err);
        PrintLine("Usage:\nTo specify a file on command line try:\nvmt_shader_swap_tool \"path_to_file\\file_name.vmt\"");
        // exit() should be ok here since we've not done anything yet
        exit(0);
    }

    // For loop to iterate each input file, it's a big one
    int iSuccessfulFileWrites = 0;
    for (int i = 1; i < argc; i++)
    {
        PrintLine("\nArgument " + std::to_string(i) + ": " + argv[i]);

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
            PrintLine("Successfully Opened: " + strInputPath, EMessagePrefix::Success);

            // The first line of the file is put into a separate var
            std::string strFirstLine;
            std::getline(ifVmtFile, strFirstLine);
            ValidateShaderName(strFirstLine);

            // Then we place the rest of the file in another var
            std::stringstream isRestOfFile;
            ReadLinesFromFile(ifVmtFile, isRestOfFile);

            EDetectedShader eFoundShader = DetectFileShader(strFirstLine);

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
            PrintLine("Couldn't Open File: " + strInputPath, EMessagePrefix::Err);
        }
    }

    PrintLine("\n" + std::to_string(iSuccessfulFileWrites) + "/" + std::to_string(argc - 1) + " Files Written.");
}
