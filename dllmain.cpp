#include <Windows.h>
#include <vector>
#include <string>

void* EngineModule = GetModuleHandle(L"engine.dll");
const char* SendClanTagPattern = "53 56 57 8B DA 8B F9 FF 15";

std::vector<uint8_t> PatternToBytes(const char* Pattern)
{
    std::vector<uint8_t> Bytes;

    char* FirstByte = (char*)Pattern;
    char* LastByte = (char*)Pattern + strlen(Pattern);

    for (char* CurrentByte = FirstByte; CurrentByte < LastByte; CurrentByte++)
    {
        if (*CurrentByte == ' ' || *CurrentByte == '?')
        {
            CurrentByte++;
            
            if (*CurrentByte == '?')
            {
                CurrentByte++;
            }

            Bytes.push_back(-1);
        }
        else
        {
            uint8_t Byte = (uint8_t)strtoul(CurrentByte, &CurrentByte, 16);
            Bytes.push_back(Byte);
        }
    }

    return Bytes;
}

uint8_t* PatternScan(void* Module, const char* Pattern)
{
    PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)Module;
    PIMAGE_NT_HEADERS NTHeaders = (PIMAGE_NT_HEADERS)((uint8_t*)Module + DosHeader->e_lfanew);

    std::vector<uint8_t> PatternBytesVector = PatternToBytes(Pattern);
    size_t PatternBytesSize = PatternBytesVector.size();
    uint8_t* PatternBytes = PatternBytesVector.data();

    DWORD ModuleBytesSize = NTHeaders->OptionalHeader.SizeOfImage;
    uint8_t* ModuleBytes = (uint8_t*)Module;

    for (DWORD ModuleByteIndex = 0; ModuleByteIndex < ModuleBytesSize - PatternBytesSize; ModuleByteIndex++)
    {
        bool Found = true;

        for (DWORD PatternByteIndex = 0; PatternByteIndex < PatternBytesSize; PatternByteIndex++)
        {
            if (PatternBytes[PatternByteIndex] != -1 &&
                ModuleBytes[ModuleByteIndex + PatternByteIndex] != PatternBytes[PatternByteIndex])
            {
                Found = false;
                break;
            }
        }

        if (Found)
        {
            return ModuleBytes + ModuleByteIndex;
        }
    }

    return nullptr;
}

void SetClanTag(const char* Tag)
{
    auto SendClanTag = reinterpret_cast<void(__fastcall*)(const char* pTag, const char* pName)>(
        PatternScan(EngineModule, SendClanTagPattern));

    SendClanTag(Tag, Tag);
}

DWORD WINAPI OnDllAttach(PVOID Module)
{
    AllocConsole();

    const char* Message = "Write tag that you need: ";
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), Message, strlen(Message), nullptr, NULL);
  
    char Symbol = '\0';
    DWORD SymbolsRead = 0;
    std::string ClanTag = "";

    while (Symbol != '\n')
    {
        ReadConsoleA(GetStdHandle(STD_INPUT_HANDLE), &Symbol, 1, &SymbolsRead, nullptr);
        ClanTag.push_back(Symbol);
    }

    ClanTag.resize(ClanTag.size() - 1);

    SetClanTag(ClanTag.c_str());
    
    FreeConsole();
    FreeLibraryAndExitThread((HMODULE)Module, 1);
}

BOOL APIENTRY DllMain(HMODULE Module, DWORD ReasonForCall, LPVOID Reserved)
{
    switch (ReasonForCall)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(NULL, 0, OnDllAttach, Module, 0, NULL);

        break;
    }
    return TRUE;
}

