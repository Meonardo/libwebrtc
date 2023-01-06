import os
import sys
import subprocess
import argparse
import shutil

HOME_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), "../.."))

GN_ARGS = [
    'target_os="win"',
    'target_cpu="x64"',
    'is_component_build=false',
    'rtc_use_h264=true',
    'rtc_enable_win_wgc=true',
    'use_lld=false',
    'use_custom_libcxx=false',
    # 'rtc_libvpx_build_vp9=true',
    'ffmpeg_branding="Chrome"',
    'is_clang=true',
    'treat_warnings_as_errors=false',
    'rtc_include_tests=false',
    'rtc_build_examples=false',
]


def gngen(scheme):
    gn_args = list(GN_ARGS)
    if scheme == 'release':
        gn_args.append('is_debug=false')
    else:
        gn_args.append('is_debug=true')

    flattened_args = ' '.join(gn_args)

    print("Home path: ", HOME_PATH)
    print("GN args: ", flattened_args)
    print("output path: ", getoutputpath(scheme))

    ret = subprocess.call(['gn.bat', 'gen', getoutputpath(scheme), '--args=%s' % flattened_args, '--ide=vs'],
                          cwd=HOME_PATH, shell=False)
    if ret == 0:
        return True
    
    print("subprocess call return", ret)
    return False


def getoutputpath(scheme):
    t = os.path.join(HOME_PATH, 'out-%s' % scheme)
    return os.path.join(t, 'Windows-x64')


def ninjabuild(scheme):
    out_path = getoutputpath(scheme)
    if subprocess.call(['ninja', '-C', out_path], cwd=HOME_PATH) != 0:
        return False
    return True


def copy_libwebrtc(scheme, output_path, copy_header_src=False):
    # libwebrtc.dll, libwebrtc.dll.lib, libwebrtc.dll.pdb
    print('start copying out files to %s!' % output_path)

    targets = [
        "libwebrtc.dll", 
        "libwebrtc.dll.lib", 
        "libwebrtc.dll.pdb",
    ]
    lib_path = getoutputpath(scheme)

    if copy_header_src:
        # copy header & src files
        dst_include_path = os.path.join(output_path, 'include')
        src_include_path = os.path.join(HOME_PATH, r'libwebrtc\include')
        if os.path.exists(dst_include_path):
            shutil.rmtree(dst_include_path)
        shutil.copytree(src_include_path, dst_include_path)

        dst_src_path = os.path.join(output_path, 'src')
        src_path = os.path.join(HOME_PATH, r'libwebrtc\src')
        if os.path.exists(dst_src_path):
            shutil.rmtree(dst_src_path)
        shutil.copytree(src_path, dst_src_path)

    # copy dll files
    for lib in targets:
        src_lib_path = os.path.join(lib_path, lib)
        if scheme == 'debug':
            dst_lib_path = os.path.join(output_path, r'lib\Debug')
        else:
            dst_lib_path = os.path.join(output_path, r'lib\Release')
    
        if not os.path.exists(dst_lib_path):
            os.mkdir(dst_lib_path)
        shutil.copy(src_lib_path, dst_lib_path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--scheme', default='debug', choices=('debug', 'release'),
                        help='Schemes for building. Supported value: debug, release')
    parser.add_argument('--gn_gen', default=True, action='store_true',
                        help='Explicitly ninja file generation.')
    parser.add_argument('--output_path', help='Path to copy libwebrtc.')
    opts = parser.parse_args()

    if opts.gn_gen:
        if not gngen(opts.scheme):
            print("gn gen failed!")
            return 1
    if not ninjabuild(opts.scheme):
            print("ninjabuild failed!")
            return 1

    if opts.output_path:
        copy_libwebrtc(opts.scheme, opts.output_path)

    print('Done')
    return 0


if __name__ == '__main__':
    sys.exit(main())