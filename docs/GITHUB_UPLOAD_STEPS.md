# Upload this project to GitHub

Repository: https://github.com/massa-creto/cyd-youtube-dashboard

1. Unzip the package locally. Open the extracted folder and confirm that `README.md`, `firmware/`, `docs/`, `installer-web/`, `tests/` and `.github/` are present at the root.
2. Use GitHub Desktop or Git to commit the **contents**, not the enclosing folder. Hidden `.github/` must be included.
3. Push to `main` and open **Actions**. The source-test and PlatformIO-build jobs should pass; inspect failures before proceeding.
4. Go to **Settings → Pages → Build and deployment → GitHub Actions**.
5. Run **Build, test and publish safe installer landing page** under Actions, then visit https://massa-creto.github.io/cyd-youtube-dashboard/.
6. Confirm that the site still says **browser flashing is not yet available**. This is intentional until factory flashing is validated.
7. Do not publish personal `.pio/` output, flash dumps, `.env` files, API keys or Wi-Fi settings.

The repository is ready for source review and website publication as a development candidate. This does not constitute a tested public factory installer.
