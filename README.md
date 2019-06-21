1. 编译：
    请使用从我们官网申请的 AppId & AppSign 替换 main.cpp 中的 g_appId & g_appSignature;
    执行 ./build.sh，可根据实际需要修改传给 cmake 的参数定义。具体参见 main.cpp 源码；
    某些发行版操作系统可能并没有安装 cmake 等编译工具，请根据提示安装各依赖库;
    如果编译时提示找不到 libasound.so 等库，请先使用如下命令安装:
        sudo apt install alsa-utils v4l-utils

2. 准备教学视频：
    安装 ffmpeg module;
    使用 mp4convert.py 工具将录制的原始视频进行转换，具体参数见 mp4convert.py 脚本;
    文件名请使用 [%d].mp4, [%d] 代表整数，且需要连续;

3. 执行：
    直接运行 ./start.sh -p -fc 6 启动 Demo;
    具体参数请参照 main.cpp 源码

说明：
    1. 该代码仅用于演示如何使用 Zego SDK。请根据自己的实际需求进行集成。
    2. 要想正常运行本演示程序，需要设备已连接公网。
    3. 该 Demo 只能运行在 Linux 系统上，推荐 Ubuntu Server 18.04 LTS。
    4. 如果使用的不是 mp4convert.py 中默认的分辨率、码率、帧率，请相应修改 main.cpp 中设置分辨率、码率、帧率的代码
