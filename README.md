# D3D9-Menu-Cheat-Pointblank-Zepetto

A Direct3D9-based cheat menu for Point Blank (Zepetto IDN Server) with features like ESP, crosshair customization, and anti-kick functionality.

## Features
- **ESP Name Tag**: Displays player name tags with customizable line colors.
- **Anti-Kick**: Prevents being kicked from the game.
- **Customizable Crosshair**: Options for dot, cross, or circle crosshairs with adjustable size and color.
- **Menu Customization**: Configurable menu and border colors, including RGB animation.
- **Box Style**: Toggle between bordered or filled background styles.
- **Dynamic Scaling**: Menu scales based on screen resolution.
- **Time Display**: Shows current time in the menu.

## Prerequisites
- **Operating System**: Windows
- **Libraries**:
  - DirectX 9 SDK [Link](https://www.microsoft.com/en-us/download/details.aspx?id=6812)
  - MinHook library [`libMinHook.x86.lib`](https://github.com/TsudaKageyu/minhook)
- **Game**: Point Blank (Zepetto version IDN Server)
- **Compiler**: Compatible with C++ compilers supporting Windows API and Direct3D9 (e.g., MSVC)

## Installation
1. Clone the repository:
   ```bash
   git clone https://github.com/kyy-95631488/D3D9-Menu-Cheat-Pointblank-Zepetto.git
   ```
2. Install the required DirectX 9 SDK and MinHook library.
3. Build the project using a C++ compiler (e.g., Visual Studio).
4. Inject the compiled DLL into the Point Blank process using a DLL injector.

## Usage
- **Toggle Menu**: Press `INSERT` to show/hide the menu.
- **Navigation**: Use `UP` and `DOWN` arrow keys to navigate menu items.
- **Adjust Settings**: Use `LEFT` and `RIGHT` arrow keys to toggle or adjust selected options.
- **Features**:
  - Enable/disable ESP Name Tag and Anti-Kick.
  - Customize crosshair type (Off, Dot, Cross, Circle), size, and color.
  - Change menu and border colors, including an RGB animation option.
  - Toggle box style between border-only and filled background.

## Controls
- **INSERT**: Toggle menu visibility
- **UP/DOWN**: Navigate menu items
- **LEFT/RIGHT**: Adjust selected item (toggle or increment/decrement)

## Code Structure
- **Main File**: Implements a `D3DMenu` class for rendering and handling the cheat menu.
- **Hooks**: Uses MinHook to hook the Direct3D9 `EndScene` function for rendering.
- **Features**:
  - `ToggleESPNameTag`: Patches memory to enable/disable ESP name tags.
  - `ToggleAntiKick`: Patches memory to prevent kicks.
  - `DrawCrosshair`: Renders customizable crosshairs.
  - `DrawESPBody`: Draws ESP boxes and name tags.
- **Dependencies**:
  - Direct3D9 (`d3d9.lib`, `d3dx9.lib`)
  - MinHook (`libMinHook.x86.lib`)

## Building
1. Ensure DirectX 9 SDK and MinHook are installed.
2. Open the project in your IDE (e.g., Visual Studio).
3. Link against `d3d9.lib`, `d3dx9.lib`, and `libMinHook.x86.lib`.
4. Build the solution in Release mode for x86 architecture.
5. The output DLL can be found in the build directory.

## Injection
1. Launch Point Blank.
2. Use a DLL injector (e.g., Xenos, Extreme Injector) or Cheat Engine to inject the compiled DLL into `PointBlank.exe`.
3. A success message will appear if the injection is successful.

## Notes
- **Use at Your Own Risk**: This is a cheat tool and may violate the game's terms of service, potentially leading to bans.
- **Compatibility**: Tested with Point Blank (Zepetto). May require updates for different game versions.
- **Logging**: Outputs logs to the console and debugger for debugging purposes.

## Contributing
1. Fork the repository.
2. Create a new branch (`git checkout -b feature-branch`).
3. Make your changes and commit (`git commit -m "Add feature"`).
4. Push to the branch (`git push origin feature-branch`).
5. Create a Pull Request.

## Disclaimer
This project is for educational purposes only. The author is not responsible for any consequences resulting from the use of this software. Use responsibly and respect the game's terms of service.

## License
This project is licensed under the MIT License. See the `LICENSE` file for details.