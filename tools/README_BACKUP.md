# Windows private read-only flash backup

This tool is **not** an installer or restore tool. It reads the ESP32 flash to a private local file; it does not erase or write the device. Entering download mode can reboot it.

1. Install Python 3.11+ and `python -m pip install esptool==4.8.1` on Windows. Use the matching COM port shown in Device Manager.
2. Close serial monitors and browser serial connections.
3. Create an empty directory outside the Git repository, e.g. `C:\CYD_Private_Backups\backup1` (the script will create it).
4. Run `python tools\backup_flash_windows.py --port COM5 --out C:\CYD_Private_Backups\backup1 --confirm-read` (replace COM5).
5. Keep both the `.bin` and JSON file offline; verify file size and SHA-256. **The backup includes Wi-Fi credentials, API keys, and other secrets.**

The script deliberately refuses to guess flash size or operate without explicit confirmation. **Restoring is not automated** because flash layout, encryption, target identity, and recovery steps need hardware validation first. No restore command should be used on the working CYD during Session 2.
