# Third-Party Notices

本项目（Roco Kingdom 3D）以 [MIT License](LICENSE) 发布。随源码或二进制分发的第三方组件版权归原作者所有，分别适用下列许可证。完整许可证原文见各组件文件头或对应目录中的 `LICENSE` 文件。

## 组件清单

| 组件 | 版本 | 许可证 | 版权持有者 | 位置 |
| --- | --- | --- | --- | --- |
| [raylib](https://github.com/raysan5/raylib) | 6.0（上游 `a4680fc`） | zlib/libpng | Copyright (c) 2013-2026 Ramon Santamaria (@raysan5) | `vendor/raylib/` |
| [raygui](https://github.com/raysan5/raygui) | 5.0-dev | zlib/libpng | Copyright (c) 2014-2026 Ramon Santamaria (@raysan5) | `include/raygui.h` |
| [Noto Sans SC](https://fonts.google.com/noto/specimen/Noto+Sans+SC) | 游戏用字子集 | SIL OFL 1.1 | Copyright 2014-2021 Adobe, with Reserved Font Name 'Source' | `resources/fonts/roco-ui.ttf` |
| [GLFW](https://www.glfw.org/) | raylib 内置 | zlib/libpng | Copyright (c) 2002-2006 Marcus Geelnard; 2006-2019 Camilla Löwy | `vendor/raylib/src/external/glfw/` |
| [cgltf](https://github.com/jkuhlmann/cgltf) | raylib 内置 | MIT | Copyright (c) 2018-2021 Johannes Kuhlmann | `vendor/raylib/src/external/cgltf.h` |
| [miniaudio](https://github.com/mackron/miniaudio) | raylib 内置 | Public domain / MIT-0 | Copyright (c) David Reid (mackron) | `vendor/raylib/src/external/miniaudio.h` |
| [stb](https://github.com/nothings/stb) | raylib 内置 | Public domain / MIT | Copyright (c) Sean Barrett 及贡献者 | `vendor/raylib/src/external/stb_*` |
| [dr_libs](https://github.com/mackron/dr_libs) | raylib 内置 | Public domain / MIT-0 | Copyright (c) David Reid (mackron) | `vendor/raylib/src/external/dr_*.h` |
| [glad](https://github.com/Dav1dde/glad) | raylib 内置 | Generated loader（MIT / Khronos） | 见文件头 | `vendor/raylib/src/external/glad.h` |
| [RGFW](https://github.com/ColleagueRiley/RGFW) | raylib 内置 | libpng/zlib | Copyright (C) 2022-26 Riley Mabb (@ColleagueRiley) | `vendor/raylib/src/external/RGFW/` |
| 其他单头文件库 | raylib 内置 | Public domain / MIT / zlib | 见各文件头 | `vendor/raylib/src/external/` |

"raylib 内置"表示随 raylib 源码一起分发，仅由 raylib 在对应平台按需编译；本项目不单独依赖这些组件。

内置字体 `resources/fonts/roco-ui.ttf` 为 Noto Sans SC 的子集（ASCII/Latin-1 + 游戏用到的 483 个汉字及标点）。完整 OFL 1.1 许可证原文见 [`resources/fonts/OFL.txt`](resources/fonts/OFL.txt)。

`vendor/raylib/src/external/` 中还包含 qoi、qoa、m3d、par_shapes、tinyobj_loader_c、sdefl/sinfl、jar_mod/jar_xm、rprand、rlsw、vox_loader 等单头文件库，均使用公有领域或 MIT/zlib 类宽松许可证，具体条款见各文件头。

## zlib License

适用于 raylib、raygui、GLFW、RGFW：

```text
This software is provided 'as-is', without any express or implied warranty. In no event
will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial
applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote
   the original software. If you use this software in a product, an acknowledgment in the
   product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as
   being the original software.
3. This notice may not be removed or altered from any source distribution.
```

## MIT License

适用于 cgltf 及采用 MIT 许可证的内置库：

```text
Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

## SIL Open Font License 1.1

适用于内置 Noto Sans SC 游戏用字子集（`resources/fonts/roco-ui.ttf`）。许可证原文见 [`resources/fonts/OFL.txt`](resources/fonts/OFL.txt)。

## Public Domain / MIT-0

miniaudio、dr_libs、stb 等组件由作者声明为公有领域，或允许在 MIT-0（无署名要求）下使用；本文件中的署名仅为致谢。
