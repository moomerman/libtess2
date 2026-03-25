# libtess2

Pre-built [libtess2](https://github.com/memononen/libtess2) (Mikko Mononen's polygon tessellation library) with [Odin](https://odin-lang.org) bindings.

## Platforms

| Platform       | Library                        |
|---------------|-------------------------------|
| macOS arm64   | `lib/libtess2_darwin_arm64.a` |
| macOS amd64   | `lib/libtess2_darwin_amd64.a` |
| Linux amd64   | `lib/libtess2_linux_amd64.a`  |
| Windows amd64 | `lib/libtess2_windows_amd64.lib` |
| WebAssembly   | `lib/libtess2_wasm.o`        |

## Usage (Odin)

Add as a git submodule or clone into your project, then import the package:

```odin
import tess "libtess2/odin"

t := tess.NewTess(nil)
defer tess.DeleteTess(t)

verts := [?]f32{ 0, 0, 100, 0, 50, 100 }
tess.AddContour(t, 2, &verts, size_of(f32) * 2, 3)

normal := [?]f32{ 0, 0, 1 }
tess.Tesselate(t, 0, 0, 3, 2, &normal)
```

## Usage (C)

Link against the appropriate library for your platform. See `c/tess2_test.c` for a complete example.

```sh
cc -o test tess2_test.c ../lib/libtess2_darwin_arm64.a -lm
```

## Versioning

Libraries are built from the latest upstream [libtess2](https://github.com/memononen/libtess2) source via the `build-libs` workflow. The upstream source is vendored in `src/` and the git SHA tracks the exact version.

To rebuild, run the **Build libs** workflow from GitHub Actions.

## Structure

```
src/           Vendored upstream C source (auto-updated by build-libs workflow)
lib/           Pre-built static libraries for all platforms
odin/          Odin language bindings and tests
c/             C test harness
```

## License

libtess2 is licensed under the [SGI Free Software License B](src/LICENSE.txt).
