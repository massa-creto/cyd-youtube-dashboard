# Contributing

Thanks for helping improve CYD YouTube Dashboard. Please keep changes focused, reproducible and easy to review.

## Before opening a pull request

1. Describe the hardware revision and the behavior you observed.
2. Make one logical change per pull request where practical.
3. Run `python -m unittest discover -s tests -v` and `pio run -d firmware -e cyd_st7789`.
4. Explain what you tested on a real device and what remains untested.
5. Keep generated build files, credentials, flash dumps and local settings out of commits.

Use English for code identifiers, comments, commit messages and project documentation. Avoid unrelated formatting changes in firmware patches; they make hardware regressions harder to trace.

## Licensing

By contributing, you confirm you have the right to submit the work under the project's applicable noncommercial license. Do not copy code, fonts, graphics or documentation from another project without checking its license and recording the required attribution. A contribution does not change the license of third-party code.

## Bugs and security reports

Include reproduction steps, board revision, PlatformIO output and relevant serial logs. **Remove Wi-Fi names, API keys and passwords** before attaching logs. For security vulnerabilities, follow [the security policy](docs/SECURITY.md) instead of posting exploitation details publicly.
