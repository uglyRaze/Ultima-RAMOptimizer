# RAM Optimizer Tool

A Windows console application that helps monitor and optimize memory usage of running processes.

Download here: https://github.com/uglyRaze/Ultima-RAMOptimizer/releases/tag/RAM

## Features

- Lists all visible running processes in a selectable menu
- Displays real-time memory usage (RAM) for selected processes
- Optimizes memory by emptying working sets of selected processes
- Simple keyboard navigation (arrow keys + Enter)
- Automatically requests admin privileges when needed


## Requirements

- Windows 8 or later
- Administrator privileges (will prompt automatically)
- C++17 compatible compiler

## Optimization example

1. No optimization (Chrome) 
   ![Screenshot 2025-06-30 233028](https://github.com/user-attachments/assets/9ba4855b-ae28-4ccc-b7e3-989f6fc64c4c)
   
3. After optimization (~4 sec)
   ![Screenshot 2025-06-30 233112](https://github.com/user-attachments/assets/6881222a-ff15-4999-b13a-10c3693f10ef)

## How to Use

1. Run the executable
2. Use ↑ and ↓ arrow keys to select a process
3. Press Enter to:
   - View detailed memory usage
   - Automatically optimize RAM for the selected process
4. Press Esc to return to the process selection menu
5. Press Ctrl+C or close the window to exit

## Memory Optimization

When you select a process:
- The tool shows current memory usage in MB
- Automatically calls `EmptyWorkingSet` to free up unused RAM
- Continues monitoring memory usage until you press Esc

## Technical Details

- Uses Windows API functions:
  - `EnumWindows` to list processes
  - `GetProcessMemoryInfo` to monitor RAM
  - `EmptyWorkingSet` for optimization
- Implements a custom memory allocator (HybridAllocator) for efficient memory management
- UTF-8 compatible console output

## Building

1. Compile with a C++17 compatible compiler
2. Link against:
   - psapi.lib
   - shlwapi.lib

## Notes

- Some system processes may not respond to optimization
- The tool may need to be run as Administrator for certain processes
- Memory savings may vary depending on application behavior
