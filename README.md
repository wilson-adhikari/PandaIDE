# PandaIDE

PandaIDE is a free, open-source and lightweight IDE for **assembly, C/C++ and Python**. It is built with C++ and Qt 6 and runs on Windows, macOS and Linux.

## Download (free)

Get the latest version from **[Releases](https://github.com/wilson-adhikari/PandaIDE/releases/latest)**.

| System | File | Start |
|---|---|---|
| Windows 10/11, 64-bit | `PandaIDE-<version>-windows-x64.zip` | Unzip, then run `PandaIDE.exe` |
| macOS 12 or later (Apple silicon and Intel) | `PandaIDE-<version>-macos-universal.dmg` | Open the disk image and drag PandaIDE to Applications |
| Linux, x86-64 | `PandaIDE-<version>-linux-x86_64.AppImage` | `chmod +x PandaIDE-*.AppImage`, then run it |

The downloads are not code-signed yet, so the first start needs one extra step:

- **Windows:** if SmartScreen appears, click **More info → Run anyway**.
- **macOS:** right-click PandaIDE in Applications and choose **Open → Open** (only the first time). Alternatively, run `xattr -dr com.apple.quarantine /Applications/PandaIDE.app`.
- **Linux:** some distributions need `libfuse2` to run AppImages.

## What it does

- **Open a folder.** It shows the file tree. You can open, edit and save files (**Ctrl+S**, Save As), and tabs mark unsaved changes. PandaIDE asks before quitting with unsaved work, and reopens the last folder at start.
- **Run the current file** (**Ctrl+R**). Files run in place, so imports of neighbouring files work. It supports:
  - Python;
  - C and C++, compiled with g++ or clang++;
  - assembly: NASM (`.asm`) or GNU assembler (`.s`) sources are linked with gcc/clang, with `main` as the entry point.
- **Problems.** Compiler errors and warnings appear in a list; click one to jump to the line.
- **Terminal.** PowerShell, Command Prompt or Git Bash on Windows; your login shell on macOS and Linux.
- **Search** across the whole folder.
- **Git.** It shows changed files and the current branch, and can commit all changes.
- **Suggestions** for C/C++, Python and assembly from keywords and the open file, plus a symbol outline.
- **Sign in with GitHub** (Account menu or status bar).

PandaIDE does not bundle compilers. Running code needs the tools for that language:

- Python 3 for Python;
- gcc, clang or MinGW-w64 for C, C++ and assembly (on Windows, for example from [MSYS2](https://www.msys2.org));
- [NASM](https://nasm.us) for `.asm` files.

**Not built yet:** a debugger, the built-in UACE assembler and emulator, and plugins.

## GitHub sign-in

PandaIDE uses GitHub's device flow:

1. Click **Sign in with GitHub**.
2. GitHub opens in your browser; enter the code PandaIDE shows you.

PandaIDE only asks for `read:user`, which is your public profile. The sign-in is kept in PandaIDE's settings on your computer. **Sign out** removes it there; you can also revoke it at <https://github.com/settings/applications>.

**For maintainers (once).** Sign-in needs a GitHub OAuth App:

1. Create it at <https://github.com/settings/applications/new>. Use the repository URL as homepage and callback URL; the device flow does not use the callback.
2. Tick **Enable Device Flow**.
3. Copy the **Client ID**. It is public, not a secret.
4. Add it as a repository variable named `PANDAIDE_GITHUB_CLIENT_ID` (**Settings → Secrets and variables → Actions → Variables**). Release builds include it from then on.

For local builds, pass `-DPANDAIDE_GITHUB_CLIENT_ID=<id>` to CMake, or set the environment variable of the same name.

## Build from source

You need:

- Qt 6.5 or newer, with Core, Gui, Widgets, Qml, Quick, Network and Test;
- CMake 3.20 or newer;
- Ninja;
- a C++17 compiler.

```bash
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/<kit>
cmake --build build
ctest --test-dir build --output-on-failure
```

The app is `build/apps/panda-ide/PandaIDE` (`.exe` on Windows, `.app` on macOS).

## Releasing

Push a version tag:

```bash
git tag v0.2.0
git push origin v0.2.0
```

[GitHub Actions](.github/workflows/build.yml) then builds and tests on all three systems, packages the zip, dmg and AppImage, and publishes them as a GitHub Release. Pushes and pull requests get the same build and tests without a release.

## Project layout

| Path | Contents |
|---|---|
| `apps/panda-ide/` | The IDE: C++ services (workspace, terminal and Run, GitHub account, completion) and the QML interface; tests are in `tests/` |
| `uace/` | Assembly syntax detection (NASM, MASM, GAS, ARM) |
| `languages/`, `toolchains/` | Language and compiler descriptions |
| `syntaxes/` | Assembly syntax definitions |
| `examples/` | Sample projects |
| `packaging/` | Linux desktop entry and icon |

## License

MIT. See [LICENSE](LICENSE).
