#!/usr/bin/env python3
#coding: utf-8


"""
该脚本直接调用 ffmpeg 进行 mp4 文件转换，所以在执行此脚本前，请确保已经安装了 ffmpeg 并正确设置了环境变量。如果您熟悉 ffmpeg，也可以直接调用 ffmpeg 自行进行转换或者基于此脚本实现您自己的转换程序以便集成至您的应用中。

ffmpeg 下载地址：https://ffmpeg.org/download.html

安装及环境变量设置请自行百度。

ffmpeg 调用示例：
ffmpeg -i ./1_5_2.mp4 -s 640*480 -r 15 -g 30 -b:v 500k -maxrate 500k -bufsize 500k -c:v libx264 -profile:v baseline -preset fast -tune zerolatency -sc_threshold 0 -x264opts "bframes=0" -pass 1 -an -f mp4 -y /dev/null

ffmpeg -i ./1_5_2.mp4 -s 640*480 -r 15 -g 30 -b:v 500k -maxrate 500k -bufsize 500k -c:v libx264 -profile:v baseline -preset fast -tune zerolatency -sc_threshold 0 -x264opts "bframes=0" -c:a aac -b:a 128k -ar 44100 -ac 2 -pass 2 -f mp4 ./1_5_2_outtr.mp4

脚本使用方法：
在调用该脚本进行视频转换时，会优先使用命令行中指定的参数；如果有使用 --config 指定参数配置文件，会从配置文件中加载转换参数用来补全命令行中没有指定的部分参数；如果即没有指定配置文件，也没有在命令行中指定，则使用原始文件的 Meta 信息（目前读取原始 Meta 信息的方法还不完善）；
在读取原始 Meta 信息失败时，则会使用本脚本中预定义的转换参数进行转换．
"""


import os,sys
import re
import argparse
import shutil
import subprocess

DEFAULT_VIDEO_SIZE_WIDTH = 960
DEFAULT_VIDEO_SIZE_HEIGHT = 540
DEFAULT_VIDEO_FRAME_RATE = 20
DEFAULT_VIDEO_BITRATE = 1000
DEFAULT_AUDIO_SAMPLE_RATE = 44100
DEFAULT_AUDIO_BITRATE = 128
DEFAULT_AUDIO_CHANNEL = 1


def __is_windows():
    return sys.platform == 'win32'


def __fix_cmd(raw_cmd):
    """
    fix the command for windows platform
    :param raw_cmd: 原始待执行 Shell 命令
    """
    if not __is_windows():
        raw_cmd = '{ ' + raw_cmd + '; }'

    return raw_cmd + " 2>&1"


class AVInfo(object):
    video_pattern = re.compile("Stream #\d:\d\(\S{3}\): Video:[^,]+,[^,]+, (\d+x\d+)[^,]*, (\d+ kb/s), (\d+[.]?\d* fps),[\s\S]+", re.MULTILINE | re.DOTALL)
    audio_pattern = re.compile("Stream #\d:\d\(\S{3}\): Audio:[^,]+, (\d+ Hz), ([^,]+),[^,]+, (\d+ kb/s)[\s\S]+", re.MULTILINE | re.DOTALL)
    pattern_error = re.compile("At least one output file must be specified")

    def __init__(self, mp4_file):
        self.src = mp4_file
        self.size = (0,0)   # video size, (width, height), eg. (640, 480)
        self.vfr = 0        # video frame rate, eg. 20
        self.bitrate = 0    # video bitrate, eg. 1000Kbps
        self.ar = 0         # audio rate, eg. 44100Hz
        self.abitrate = 0   # audio bitrate, eg. 128Kbps
        self.channel = 0    # audio channel, 1 or 2

    def __str__(self):
        return r"""{src: '%s', v_width: %d, v_height: %d, vfr: %d fps, bitrate: %d kb/s, ar: %d Hz, abitrate: %d kb/s, channel: %d""" % (self.src, self.size[0], self.size[1], self.vfr, self.bitrate, self.ar, self.abitrate, self.channel)

    def pickup_from_raw_file(self, verbose=False):
        if not os.path.exists(self.src) or not os.path.isfile(self.src):
            print("File '%s' not exists or not a media file" % self.src)
            self.size = DEFAULT_VIDEO_SIZE_WIDTH, DEFAULT_VIDEO_SIZE_HEIGHT
            self.vfr = DEFAULT_VIDEO_FRAME_RATE
            self.bitrate = DEFAULT_VIDEO_BITRATE
            self.ar = DEFAULT_AUDIO_SAMPLE_RATE
            self.abitrate = DEFAULT_AUDIO_BITRATE
            self.channels = DEFAULT_AUDIO_CHANNEL
        else:
            state, dump_str_info = exec_cmd("ffmpeg -i {}".format(self.src), verbose)
            try:
                video_info = AVInfo.video_pattern.search(dump_str_info).groups()  # widthxheight, rate, fps
                if verbose:
                    print("Raw video info, size: {}, bitrate: {}, frame rate: {} ".format(*video_info))

                self.size = [int(v) for v in video_info[0].split('x')]
                self.bitrate = float(video_info[1].split()[0])
                self.vfr = float(video_info[2].split()[0])
            except Exception as e:
                print("!!! Warning, can't dump video info(msg: %s), use default!!!" % e)
                self.size = DEFAULT_VIDEO_SIZE_WIDTH, DEFAULT_VIDEO_SIZE_HEIGHT
                self.vfr = DEFAULT_VIDEO_FRAME_RATE
                self.bitrate = DEFAULT_VIDEO_BITRATE

            try:
                audio_info = AVInfo.audio_pattern.search(dump_str_info).groups()  # ar, channels, abitrate
                if verbose:
                    print("Raw audio info, sample rate: {}, channels: {}, bitrate: {} ".format(*audio_info))

                self.ar = int(audio_info[0].split()[0])
                if audio_info[1] == "mono":
                    self.channel = 1
                elif audio_info[1] == "stereo":
                    self.channel = 2
                else:
                    print('!!! Warning, unknown channel key: <{}>, use default'.format(audio_info[1]))
                    self.channel = 1

                self.abitrate = float(audio_info[2].split()[0])
                
            except Exception as e:
                print("!!! Warning, can't dump audio info(msg: %s), use default!!!" % e)
                self.ar = DEFAULT_AUDIO_SAMPLE_RATE
                self.abitrate = DEFAULT_AUDIO_BITRATE
                self.channels = DEFAULT_AUDIO_CHANNEL

        return self


class ConvertArgs(object):
    def __init__(self):
        self.video_size_with = 0
        self.video_size_height = 0
        self.video_frame_rate = 0
        self.video_bitrate = 0
        self.audio_rate = 0
        self.audio_bitrate = 0
        self.audio_channel = 0
        self.verbose = False

    def __setattr__(self, k, v):
        if v < 0:
            raise ValueError("{}'s value must be a positive integer".format(k))

        super(ConvertArgs, self).__setattr__(k, v)

    def init(self, raw_args):
        self._init_from_config_file(raw_args.config)
        self._init_from_cmd_line(raw_args)
        self.verbose = raw_args.verbose

    def _init_from_config_file(self, config_file):
        """
        config file format and support meta, eg:
            video_size_with=960
            video_size_height=540
            video_frame_rate=20
            video_bitrate=1000
            audio_rate=44100
            audio_bitrate=128
            audio_channel=2
        """
        if config_file is None or len(config_file) == 0 or not os.path.exists(config_file) or not os.path.isfile(config_file):
            return

        with open(config_file, 'r') as fconfig:
            for line in fconfig.readlines():
                _line = line.strip()
                if len(_line) == 0 or _line.startswith('#'):
                    continue

                k, v = line.strip().split('=')
                self.__setattr__(k.strip(), int(v))

    def _init_from_cmd_line(self, raw_args):
        try:
            if raw_args.size is not None and len(raw_args.size) > 0:
                w, h = re.split('x|X', raw_args.size)
                self.video_size_with = int(w)
                self.video_size_height = int(h)
        except ValueError as e:
            raise Exception("--size value 's format must be wxh")

        self.video_frame_rate = raw_args.vfr if raw_args.vfr is not None else self.video_frame_rate
        self.video_bitrate = raw_args.bitrate if raw_args.bitrate is not None else self.video_bitrate

        self.audio_rate = raw_args.ar if raw_args.ar is not None else self.audio_rate
        self.audio_bitrate = raw_args.abitrate if raw_args.abitrate is not None else self.audio_bitrate
        self.audio_channel = raw_args.channel if raw_args.channel is not None else self.audio_channel


    def reconfigure_with_av_info_if_need(self, avInfo):
        if self.video_size_with == 0 and self.video_size_height == 0:
            self.video_size_with = avInfo.size[0]
            self.video_size_height = avInfo.size[1]
        if self.video_frame_rate == 0: self.video_frame_rate = avInfo.vfr
        if self.video_bitrate == 0: self.video_bitrate = avInfo.bitrate

        if self.audio_rate == 0: self.audio_rate = avInfo.ar
        if self.audio_bitrate == 0: self.audio_bitrate = avInfo.abitrate
        if self.audio_channel == 0: self.audio_channel = avInfo.channel


def parse_arguments():
    parser = argparse.ArgumentParser(description="Set up conver parameters, load order: command line --> config file --> the raw's meta data --> default value")
    parser.add_argument("--src", action="store", type=str, default=".", help="specify the mp4 source path.")
    parser.add_argument("--dst", action="store", type=str, default="out", help="specify the output path.")
    parser.add_argument("--size", action="store", type=str, help="set frame size (WxH or abbreviation), default 960x540. if set, will overrite the config file value")
    parser.add_argument("--vfr", action="store", type=int, help="set frame rate (Hz value, fraction or abbreviation), default 20. if set, will overrite the config file value")
    parser.add_argument("--bitrate", action="store", type=int, help="set video bitrate, default 1000Kb. if set, will overrite the config file value")
    parser.add_argument("--ar", action="store", type=int, help="set audio sample rate, default 44100Hz. if set, will overrite the config file value")
    parser.add_argument("--abitrate", action="store", type=int, help="set audio bitrate, default 128K. if set, will overrite the config file value")
    parser.add_argument("--channel", action="store", type=int, help="set audio channel number, default 1. if set, will overrite the config file value")
    parser.add_argument("--config", action="store", type=str, help="parse parameters from this file")
    parser.add_argument("--verbose", action="store_true", default=False, help="whether print command and ffmpeg's output")
    args = parser.parse_args()

    return args


def exec_cmd(raw_cmd, is_print=False):
    """
    execute command.
    :param raw_cmd: command line;
    :param is_print: whether print console message, default False;
    :return (state, text): execute failed when state not equals zero.
    """

    if is_print:
        print() # print \r\n
        try:
            print("[*] Execute: [{}]".format(raw_cmd))
        except:
            print('[**] Execute: PRINT RAW CMD ERROR')

    _cmd = __fix_cmd(raw_cmd)
    process = subprocess.Popen(_cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    return_code = process.wait()
    state = 0
    if return_code != 0:
        state = return_code
        text = stdout.decode('utf-8', errors='replace')
        text_error = stderr.decode('utf-8', errors='replace')
        if len(text_error) > 0:
            text += '\r\n\r\n' + text_error
    else:
        state = 0
        text = stdout.decode('utf-8', errors='replace')

    if is_print:
        print(text)

    return state, text


def check_ffmpeg_exists():
    state, output = (0, "")
    if __is_windows():
        state, output = exec_cmd("where ffmpeg")
    else:
        state, output = exec_cmd("type ffmpeg")

    if state != 0:
        raise Exception("ffmpeg is not install just now, please install ffmpeg from https://ffmpeg.org/download.html")

    return True


def convert(src, dst, args):
    def _do_convert(src, dst, args):
        sys.stdout.write(">>> Converting {} to {} ".format(src, dst))
        sys.stdout.flush()

        avInfo = AVInfo(src)
        avInfo.pickup_from_raw_file(args.verbose)

        # re configure the convert params if need
        args.reconfigure_with_av_info_if_need(avInfo)

        none_target = 'NUL' if __is_windows() else '/dev/null'
        cmd_step1_template = '''ffmpeg -i {in_file} -s {v_width}*{v_height} -r {v_frame_rate} -g {v_gop} -b:v {v_bitrate}k -maxrate {v_bitrate}k -bufsize {v_bitrate}k -c:v libx264 -profile:v baseline -preset fast -tune zerolatency -sc_threshold 0 -x264opts "bframes=0" -pass 1 -an -f mp4 -y {none_target}'''
        cmd_step1 = cmd_step1_template.format(**{"in_file": src, "v_width": args.video_size_with, "v_height": args.video_size_height, "v_frame_rate": args.video_frame_rate, "v_gop": args.video_frame_rate * 2, "v_bitrate": args.video_bitrate, "none_target":none_target})
        state1, output = exec_cmd(cmd_step1, args.verbose)

        cmd_step2_template = '''ffmpeg -i {in_file} -s {v_width}*{v_height} -r {v_frame_rate} -g {v_gop} -b:v {v_bitrate}k -maxrate {v_bitrate}k -bufsize {v_bitrate}k -c:v libx264 -profile:v baseline -preset fast -tune zerolatency -sc_threshold 0 -x264opts "bframes=0" -c:a aac -b:a {a_bitrate}k -ar {a_rate} -ac {a_channel} -pass 2 -f mp4 {out_file}'''
        cmd_step2 = cmd_step2_template.format(**{"in_file": src, "v_width": args.video_size_with, "v_height": args.video_size_height, "v_frame_rate": args.video_frame_rate, "v_gop": args.video_frame_rate * 2, "v_bitrate": args.video_bitrate, "a_bitrate": args.audio_bitrate, "a_rate": args.audio_rate, "a_channel": args.audio_channel, "out_file": dst})
        state2, output = exec_cmd(cmd_step2, args.verbose)

        print('success' if state1 == 0 and state2 == 0 else "failed!")

    if not os.path.exists(src):
        raise Exception("src {} not exists".format(src))

    if os.path.exists(dst) and os.path.samefile(src, dst):
        raise Exception("target can't the same as src")

    if os.path.isfile(src):
        if os.path.exists(dst) and os.path.isfile(dst):
            os.remove(dst)
        elif os.path.isdir(dst):
            dst = os.path.join(dst, os.path.basename(src))
        elif dst.endswith('/') or dst.endswith('\\'):
            os.makedirs(dst)
            dst = os.path.join(dst, os.path.basename(src))

        _do_convert(src, dst, args)
    elif os.path.isdir(src):
        if not os.path.exists(dst):
            os.makedirs(dst)

        path_prefix = src
        if not path_prefix.endswith('/'):
            path_prefix += "/"

        offset = len(path_prefix)
        for root, folders, fnames in os.walk(src):
            for fname in fnames:
                src_file = os.path.join(root, fname)

                dst_file = os.path.join(dst, src_file[offset : ])
                dir_name = os.path.dirname(dst_file)
                if not os.path.exists(dir_name):
                    os.makedirs(dir_name)

                _do_convert(src_file, dst_file, args)
    else:
        raise ("src {} is not a file or folder")


def run():
    print("check ffmpeg has install")
    check_ffmpeg_exists()

    print("begin convert...")
    args = parse_arguments()
    convert_args = ConvertArgs()
    convert_args.init(args)
    convert(args.src, args.dst, convert_args)
    print("convert finish.")


if __name__ == "__main__":
    run()
