#!/usr/bin/python3

import unittest
import tempfile
import os
import subprocess

def create_nvmem(file):
    with open(file, 'wb') as f:
        f.write(b'0' * 1024)
        
def set_value(file, offset, value):
    with open(file, 'rb+') as f:
        f.seek(offset)
        f.write(value)
        
def get_value(file, offset):
    with open(file, 'rb') as f:
        f.seek(offset)
        return f.read(4)
    
def check_all_zero(file, exceptions):
    with open(file, 'rb') as f:
        i = 0
        bytes = b''
        while True:
            bytes = f.read(4)
            if len(bytes) == 0:
                break
            if len(bytes) != 4:
                raise RuntimeError('nvmem wrong size')
            if not i in exceptions:
                if bytes != b'0000':
                    raise RuntimeError(f'nvmem data written in wrong block: [{i}]: {bytes}')
            i += 4
                
def flash_fuse(path, arglist):
    args = ['./flash-fuse-imx8mm', '--path' , path]
    args.extend(arglist)
    return subprocess.run(args, capture_output=True, text=True)

class test_mac(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.nvmem = os.path.join(self.tmpdir.name, 'nvmem')
        create_nvmem(self.nvmem)
    def tearDown(self):
        check_all_zero(self.nvmem, [0x90, 0x94])
        self.tmpdir.cleanup()
    def test_burn(self):
        set_value(self.nvmem, 0x90, b'\x00\x00\x00\x00')
        set_value(self.nvmem, 0x94, b'\x00\x00\x00\x00')
        r = flash_fuse(self.nvmem, ['--fuse', 'MAC', '00:11:22:33:44:55', '--commit'])
        self.assertEqual(0, r.returncode )
        self.assertEqual(get_value(self.nvmem, 0x90), b'\x11\x00\x33\x22')
        self.assertEqual(get_value(self.nvmem, 0x94), b'\x55\x44\x00\x00')
    def test_burn_already_fused_same_mac(self):
        set_value(self.nvmem, 0x90, b'\x11\x00\x33\x22')
        set_value(self.nvmem, 0x94, b'\x55\x44\x00\x00')
        r = flash_fuse(self.nvmem, ['--fuse', 'MAC', '00:11:22:33:44:55', '--commit'])
        self.assertEqual(0, r.returncode )
        self.assertEqual(get_value(self.nvmem, 0x90), b'\x11\x00\x33\x22')
        self.assertEqual(get_value(self.nvmem, 0x94), b'\x55\x44\x00\x00')
    def test_burn_already_fused_wrong_mac(self):
        set_value(self.nvmem, 0x90, b'\x01\x00\x00\x00')
        set_value(self.nvmem, 0x94, b'\x00\x00\x00\x00')
        r = flash_fuse(self.nvmem, ['--fuse', 'MAC', '00:11:22:33:44:55', '--commit'])
        self.assertEqual(1, r.returncode )
        self.assertEqual(get_value(self.nvmem, 0x90), b'\x01\x00\x00\x00')
        self.assertEqual(get_value(self.nvmem, 0x94), b'\x00\x00\x00\x00')
        set_value(self.nvmem, 0x90, b'\x00\x00\x00\x00')
        set_value(self.nvmem, 0x94, b'\x02\x00\x00\x00')
        r = flash_fuse(self.nvmem, ['--fuse', 'MAC', '00:11:22:33:44:55', '--commit'])
        self.assertEqual(1, r.returncode )
        self.assertEqual(get_value(self.nvmem, 0x90), b'\x00\x00\x00\x00')
        self.assertEqual(get_value(self.nvmem, 0x94), b'\x02\x00\x00\x00')
    def test_get(self):
        set_value(self.nvmem, 0x90, b'\x11\x00\x33\x22')
        set_value(self.nvmem, 0x94, b'\x55\x44\x00\x00')
        r = flash_fuse(self.nvmem, ['--fuse', 'MAC', '--get'])
        self.assertEqual(0, r.returncode)
        self.assertEqual('00:11:22:33:44:55\n', r.stdout)
    def test_verify(self):
        set_value(self.nvmem, 0x90, b'\x11\x00\x33\x22')
        set_value(self.nvmem, 0x94, b'\x55\x44\x00\x00')
        r = flash_fuse(self.nvmem, ['--fuse', 'MAC', '00:11:22:33:44:55', '--verify'])
        self.assertEqual(0, r.returncode )
        self.assertEqual(get_value(self.nvmem, 0x90), b'\x11\x00\x33\x22')
        self.assertEqual(get_value(self.nvmem, 0x94), b'\x55\x44\x00\x00')
    def test_verify_wrong(self):
        set_value(self.nvmem, 0x90, b'\x11\x00\x33\x22')
        set_value(self.nvmem, 0x94, b'\x55\x44\x00\x00')
        r = flash_fuse(self.nvmem, ['--fuse', 'MAC', '99:11:22:33:44:55', '--verify'])
        self.assertEqual(1, r.returncode )
        self.assertEqual(get_value(self.nvmem, 0x90), b'\x11\x00\x33\x22')
        self.assertEqual(get_value(self.nvmem, 0x94), b'\x55\x44\x00\x00')

if __name__ == '__main__':
    unittest.main()