#!/usr/bin/python3

import unittest
import tempfile
import os
import subprocess

def create_otp(dir, name, value):
    with open(os.path.join(dir, name), 'w') as f:
        f.write(f'{value}\n')
    
def get_otp(dir, name):
    with open(os.path.join(dir, name), 'r') as f:
        return '0x' + f.read().strip('\n')[2:].zfill(8)
        
def flash_fuse(dir, arglist):
    args = ['./flash-fuse-imx.py', '--fuse-path' , dir]
    args.extend(arglist)
    return subprocess.run(args).returncode

class test_jtag(unittest.TestCase):
    def test_burn(self):
        with tempfile.TemporaryDirectory() as dir:
                create_otp(dir, 'HW_OCOTP_CFG5', '0x00000012')
                self.assertEqual(0, flash_fuse(dir, ['--disable-jtag', '--commit']))
                self.assertEqual(get_otp(dir, 'HW_OCOTP_CFG5'), '0x00100000')
                
    def test_burn_already_fused(self):
        with tempfile.TemporaryDirectory() as dir:
            create_otp(dir, 'HW_OCOTP_CFG5', '0x00100012')
            self.assertEqual(0, flash_fuse(dir, ['--disable-jtag', '--commit']))
            self.assertEqual(get_otp(dir, 'HW_OCOTP_CFG5'), '0x00100012')
            
    def test_verify_not_fused(self):
        with tempfile.TemporaryDirectory() as dir:
            create_otp(dir, 'HW_OCOTP_CFG5', '0x00000012')
            self.assertEqual(1, flash_fuse(dir, ['--disable-jtag']))
            self.assertEqual(get_otp(dir, 'HW_OCOTP_CFG5'), '0x00000012')
            
    def test_verify_fused(self):
        with tempfile.TemporaryDirectory() as dir:
            create_otp(dir, 'HW_OCOTP_CFG5', '0x00100012')
            self.assertEqual(0, flash_fuse(dir, ['--disable-jtag']))
            self.assertEqual(get_otp(dir, 'HW_OCOTP_CFG5'), '0x00100012')
            
class test_mandatory(unittest.TestCase):
    def test_burn(self):
        with tempfile.TemporaryDirectory() as dir:
                create_otp(dir, 'HW_OCOTP_CFG5', '0x00000000')
                self.assertEqual(0, flash_fuse(dir, ['--mandatory', '--commit']))
                self.assertEqual(get_otp(dir, 'HW_OCOTP_CFG5'), '0x00000008')
                
    def test_burn_already_fused(self):
        with tempfile.TemporaryDirectory() as dir:
            create_otp(dir, 'HW_OCOTP_CFG5', '0x00000008')
            self.assertEqual(0, flash_fuse(dir, ['--mandatory', '--commit']))
            self.assertEqual(get_otp(dir, 'HW_OCOTP_CFG5'), '0x00000008')
            
    def test_verify_not_fused(self):
        with tempfile.TemporaryDirectory() as dir:
            create_otp(dir, 'HW_OCOTP_CFG5', '0x00000000')
            self.assertEqual(1, flash_fuse(dir, ['--mandatory']))
            self.assertEqual(get_otp(dir, 'HW_OCOTP_CFG5'), '0x00000000')
            
    def test_verify_fused(self):
        with tempfile.TemporaryDirectory() as dir:
            create_otp(dir, 'HW_OCOTP_CFG5', '0x00000008')
            self.assertEqual(0, flash_fuse(dir, ['--mandatory']))
            self.assertEqual(get_otp(dir, 'HW_OCOTP_CFG5'), '0x00000008')

class test_mac(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.dir = self.tmpdir.name
    def tearDown(self):
        self.tmpdir.cleanup()

    def test_burn(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0x00000000')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x00000000')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000000')
        self.assertEqual(0, flash_fuse(self.dir, ['--mac', '00:11:22:33:44:55', '--commit']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x22334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')
              
    def test_burn_already_fused(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0x22334455')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x0011')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000300')
        self.assertEqual(0, flash_fuse(self.dir, ['--mac', '00:11:22:33:44:55', '--commit']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x22334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')
                
    def test_burn_already_fused_wrong_mac(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0xff334455')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x0011')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000300')
        self.assertEqual(1, flash_fuse(self.dir, ['--mac', '00:11:22:33:44:55', '--commit']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0xff334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')

    def test_verify_not_fused(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0x00000000')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x00000000')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000000')
        self.assertEqual(1, flash_fuse(self.dir, ['--mac', '00:11:22:33:44:55']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x00000000')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000000')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000000')
            
    def test_verify_fused(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0x22334455')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x0011')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000300')
        self.assertEqual(0, flash_fuse(self.dir, ['--mac', '00:11:22:33:44:55']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x22334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')

if __name__ == '__main__':
    unittest.main()