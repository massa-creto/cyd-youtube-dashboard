# Repository publication checklist

- [x] Author set to MassaCreto and channel link documented as attribution only
- [x] No channel-specific branding in active firmware sources (automated scan)
- [x] No personal Google API key or named home Wi-Fi SSID found in project source (pattern scan; not a guarantee)
- [x] Exclude build outputs, bytecode, secrets, local IDE settings
- [x] Documentation and architecture illustration present
- [x] User-reported PlatformIO compile/upload and partial live function on RC3.1
- [ ] Verify CI PlatformIO build on GitHub
- [ ] Audit exact transitive license notices and package versions
- [ ] Replace LICENSE pointer with legally reviewed, complete license text if needed; confirm rights and applicability
- [ ] Resolve shared default credentials and unencrypted HTTP administration before broad deployment
- [ ] Verify Google TLS chain and clock sync on physical hardware
- [ ] Complete fresh-device hardware QA and upgrade QA
- [ ] Verify factory binary layout, activate browser installer only after that
- [ ] Run dependency and security scans, confirm supply-chain pinning

**Release policy:** repository source may be uploaded as a clearly marked development candidate. Do not tag a production release or enable unattended flashing until all blocking items are closed.
