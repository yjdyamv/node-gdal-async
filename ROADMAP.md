## Completed

- [x] ~~Switch from nan to N-API~~ — complete ABI-stable migration, zero NAN deps
- [x] ~~Switch to cmake-js~~ — already there
- [x] ~~Dependency management~~ — Conan → vcpkg, no bundled source in repo

## Now (v3.12.1)

- [ ] CI all-green on Linux/macOS/Windows
- [ ] Publish prebuilt binaries via GitHub Releases
- [ ] Publish `@yjdyamv/gdal-async` to npm

## Next

- [ ] Support aborting GDAL operations via AbortController (Node >= 15)
- [ ] `worker_threads` support (mostly automatic with N-API)
- [ ] Static linking for single .node artifact (Windows & macOS)
