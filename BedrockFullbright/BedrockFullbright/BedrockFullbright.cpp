#include <Windows.h>
#include <cstdio>
#include <vector>
#include <TlHelp32.h>

DWORD pID;
std::vector<unsigned int> offsets = {
    0x0,
    0x28,
    0xDE8,
    0x18,
    0x58,
    0x1C8,
    0x18
};

uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName)
{
    uintptr_t base = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);

    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32W entry;
        entry.dwSize = sizeof(entry);

        if (Module32FirstW(hSnapshot, &entry))
        {
            do
            {
                if (wcscmp(entry.szModule, modName) == 0)
                {
                    base = (uintptr_t)entry.modBaseAddr;
                    break;
                }
            } while (Module32NextW(hSnapshot, &entry));
        }
        CloseHandle(hSnapshot);
    }
    return base;
}

uintptr_t fixptr(HANDLE process, uintptr_t base, std::vector<unsigned int> offsets)
{
    uintptr_t ptr = base;
    uintptr_t temp = 0;

    for (unsigned int i = 0; i < offsets.size(); i++)
    {
        if (!ReadProcessMemory(process, (LPCVOID)ptr, &temp, sizeof(temp), nullptr))
        {
            printf("failed at offset finding");
            return 0;
        }
        ptr = temp + offsets[i];
    }
    return ptr;
}

int main()
{
    HWND hwnd = FindWindowA(0, "Minecraft");
    if (!hwnd) {
        printf("could not find minecraft window\n");
        Sleep(3000);
        return 1;
    }

    GetWindowThreadProcessId(hwnd, &pID);
    printf("fullbright 26.13\n");

    HANDLE minecraft = OpenProcess(PROCESS_ALL_ACCESS, false, pID);
    if (!minecraft) {
        return 1;
    }

    uintptr_t base = GetModuleBaseAddress(pID, L"Minecraft.Windows.exe");
    if (!base) {
        printf("failed to get base address\n");
        CloseHandle(minecraft);
        return 1;
    }

    printf("read base address 0x%p\n", (void*)base);

    uintptr_t GammaOffset = base + 0x0A5257E0;
    printf("gamma offset 0x%p\n", (void*)GammaOffset);

    float newgamma = 1000.f;

    uintptr_t Gamma = fixptr(minecraft, GammaOffset, offsets);
    if (!Gamma) {
        printf("failed to resolve pointer\n");
        CloseHandle(minecraft);
        return 1;
    }

    printf("pointer-->address 0x%p\n", (void*)Gamma);

    if (WriteProcessMemory(minecraft, (LPVOID)Gamma, &newgamma, sizeof(newgamma), 0))
    {
        printf("write gamma value\n");
    }
    else
    {
        printf("couldnt write to gamma value\n");
    }
    Sleep(3000);
    CloseHandle(minecraft);
    return 0;
}