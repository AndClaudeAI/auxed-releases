# auxed

**Cinema in the palm of your hands, and now on your desk.** auxed plays films and music on your phone and computer with ExoPlayer and VLC under one roof, casts to your TV, and moves films between phones at full Wi-Fi speed.

## Download for Android

Open **[the latest release](https://github.com/AndClaudeAI/auxed-releases/releases/latest)** on your Android phone (Android 7 or newer), tap **auxed.apk**, then open the download. If Android asks, allow your browser to install apps.

Once installed, auxed tells you when a new version is out (Home shows *Build N is ready*), and it can also update straight from a friend's newer auxed in **Transfer**, no internet needed.

## Download for your computer

From **[the latest release](https://github.com/AndClaudeAI/auxed-releases/releases/latest)**:

| Computer | File | Install |
| --- | --- | --- |
| Windows 10 / 11 | **auxed-windows.exe** | Run it. If Windows SmartScreen says it protected your PC, choose **More info › Run anyway**. |
| macOS 11 or newer (Intel and Apple silicon) | **auxed-mac.dmg** | Open it and drag auxed into Applications. The first time, right-click auxed and choose **Open** (or **System Settings › Privacy & Security › Open Anyway**). |
| Linux | **auxed-linux.AppImage** or **auxed-linux.deb** | AppImage: `chmod +x auxed-linux.AppImage`, then run it. Debian / Ubuntu: `sudo apt install ./auxed-linux.deb`. |

The warnings appear because the desktop installers aren't signed with a paid publisher certificate yet; they are built from the same source as the Android app by the release workflow.

On a computer auxed finds films and music in your Videos, Music, Downloads and Desktop folders, opens the films and songs you open with it, sends and receives films with phones and other computers over Wi-Fi (**Transfer**), and offers its own updates the same way the phone app does.

## Licences and source code

auxed itself is proprietary. It includes open-source software, listed in [NOTICE.md](NOTICE.md) and in the app under **Settings › About › Open-source licences**. The full licence texts are in [licenses/](licenses/).

FFmpeg and libVLC are used under the GNU LGPL 2.1 or later. Their source code, and the exact scripts used to build auxed's copies, are in [third-party/](third-party/). See [SOURCE-OFFER.md](SOURCE-OFFER.md).

## Help

Questions, problems or source-code requests: open an [issue](https://github.com/AndClaudeAI/auxed-releases/issues).
