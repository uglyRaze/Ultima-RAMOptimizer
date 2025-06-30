#include "wind.hpp"
#include <iostream>
#include "wind.hpp"


int main()
{
    admin();
    
    DisableConsoleMouseInput();
    while (true) {
        processes.clear();
        EnumWindows(EnumWindowsProc, 0);

        if (processes.empty()) {
            wcout << "Windows not found\n";
            cin.get();
            this_thread::sleep_for(chrono::milliseconds(600));
            continue;
        }
        if (selectedIndex >= (int)processes.size()) selectedIndex = (int)processes.size() - 1;
        if (selectedIndex < 0) selectedIndex = 0;
        DrawMenu();
        int a = 0;
        int c = _getch();
        if (c == 224) {
            int arrow = _getch();
            if (arrow == 72) {
                selectedIndex--;
                if (selectedIndex < 0) selectedIndex = 0;
            }
            else if (arrow == 80) {
                selectedIndex++;
                if (selectedIndex >= (int)processes.size()) selectedIndex = (int)processes.size() - 1;
            }
        }

        else if (c == 13) {
            system("cls");
            wcout << L"Memory usage for '" << processes[selectedIndex] << L"':\n";
            wcout << L"----------------------------------------\n";

            while (true) {
                ram(processes[selectedIndex]);
                
                if (_kbhit()) {
                    c = _getch();
                    if (c == 13 || c == 27) break;
                }

                this_thread::sleep_for(chrono::milliseconds(900));
            }

            system("cls");
        }
    }
    
}


