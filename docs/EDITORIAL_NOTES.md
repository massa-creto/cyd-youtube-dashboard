# Editorial and maintenance notes

This revision standardizes terminology and separates verified behavior from planned features. Documentation is written for a developer or end user, not as a project diary.

The application logic in `firmware/src/main.cpp` is intentionally unchanged from RC4. A cosmetic rewrite of hardware code without a complete compile and regression cycle would introduce unnecessary risk. The source still needs a separate modularization and code-review pass before a production release.

## Review scope

- Reworked the README, contributor guide, branding policy, licensing explanation, release checklist and GitHub publishing instructions.
- Rebuilt the GitHub Pages landing page with accessible HTML sections, responsive layout and an existing architecture diagram.
- Removed generated Python bytecode from the distributable archive.
- Retained existing PlatformIO configuration, application logic, tests and CI workflows.

## Outstanding work

- Verify complete license texts and resolved third-party dependencies.
- Run a clean PlatformIO build in CI and a full hardware regression.
- Review first-run security and secrets storage.
- Validate factory and upgrade flashing before enabling Web Serial.
