# 项目学习

1. 我正在学习这个项目。你制作的所有教程文件，都保存到Tutorial文件夹中。
2. Tutorial 文件夹中的 Markdown 教程如果要写源码文件链接，必须使用相对于 Tutorial 目录的正确相对路径，例如 `../base/object_pool.h`、`../base/object_pool.h#L20-L50`，不要写成 `base/object_pool.h`，否则 VS Code 会按当前 md 文件目录解析，导致无法打开文件。