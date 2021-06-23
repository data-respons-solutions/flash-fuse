#!/usr/bin/python3

import unittest
import tempfile
import os
import subprocess

def create_nvmem(file):
    with open(file, 'wb') as f:
        f.write(b'0' * 1024)
        
def set_value(file, offset, value):
    with open(file, 'wb+') as f:
        f.seek(offset)
        f.write(value)
        
def get_value(file, offset):
    with open(file, 'rb') as f:
        f.seek(offset)
        return f.read(4)
            
def flash_fuse(path, arglist):
    args = ['./flash-fuse-imx8mm', '--path' , path]
    args.extend(arglist)
    return subprocess.run(args).returncode

class test_mac(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.nvmem = os.path.join(self.tmpdir.name, 'nvmem')
        create_nvmem(self.nvmem)
    def tearDown(self):
        self.tmpdir.cleanup()

    def test_burn(self):
        set_value(self.nvmem, 0x90, b'\x00\x00\x00\x00')
        set_value(self.nvmem, 0x94, b'\x00\x00\x00\x00')
        self.assertEqual(0, flash_fuse(self.nvmem, ['--fuse', 'MAC', '00:11:22:33:44:55', '--commit']))
        self.assertEqual(get_value(self.nvmem, 0x90), b'\x11\x00\x33\x22')
        self.assertEqual(get_value(self.nvmem, 0x94), b'\x55\x44\x00\x00')
        
    '''        
    def test_burn_already_fused(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0x22334455')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x0011')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000300')
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'MAC', '00:11:22:33:44:55', '--commit']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x22334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')
                
    def test_burn_already_fused_wrong_mac(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0xff334455')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x0011')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000300')
        self.assertEqual(1, flash_fuse(self.dir, ['--fuse', 'MAC', '00:11:22:33:44:55', '--commit']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0xff334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')

    def test_verify_not_fused(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0x00000000')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x00000000')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000000')
        self.assertEqual(1, flash_fuse(self.dir, ['--fuse', 'MAC', '00:11:22:33:44:55']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x00000000')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000000')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000000')
            
    def test_verify_fused(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0x22334455')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x0011')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000300')
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'MAC', '00:11:22:33:44:55']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x22334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')
    '''

if __name__ == '__main__':
    unittest.main()