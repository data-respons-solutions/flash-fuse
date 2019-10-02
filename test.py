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

class test_jtag_disable(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.dir = self.tmpdir.name
    def tearDown(self):
        self.tmpdir.cleanup()
        
    def test_burn(self):
        create_otp(self.dir, 'HW_OCOTP_CFG5', '0x00000000')
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'jtag-disable', '--commit']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_CFG5'), '0x00100000')
        
    def test_burn_already_fused(self):
        create_otp(self.dir, 'HW_OCOTP_CFG5', '0x00100000')
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'jtag-disable', '--commit']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_CFG5'), '0x00100000')
                
    def test_verify_not_fused(self):
        create_otp(self.dir, 'HW_OCOTP_CFG5', '0x00000000')
        self.assertEqual(1, flash_fuse(self.dir, ['--fuse', 'jtag-disable']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_CFG5'), '0x00000000')
            
    def test_verify_fused(self):
        create_otp(self.dir, 'HW_OCOTP_CFG5', '0x00100000')
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'jtag-disable']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_CFG5'), '0x00100000')
            
class test_cfg5_dir_bt_ds(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.dir = self.tmpdir.name
    def tearDown(self):
        self.tmpdir.cleanup()
        
    def test_burn(self):
        create_otp(self.dir, 'HW_OCOTP_CFG5', '0x00000000')
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'cfg5_dir_bit_ds', '--commit']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_CFG5'), '0x00000008')
                
    def test_burn_already_fused(self):
        create_otp(self.dir, 'HW_OCOTP_CFG5', '0x00000008')
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'cfg5_dir_bit_ds', '--commit']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_CFG5'), '0x00000008')
            
    def test_verify_not_fused(self):
        create_otp(self.dir, 'HW_OCOTP_CFG5', '0x00000000')
        self.assertEqual(1, flash_fuse(self.dir, ['--fuse', 'cfg5_dir_bit_ds']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_CFG5'), '0x00000000')
            
    def test_verify_fused(self):
        create_otp(self.dir, 'HW_OCOTP_CFG5', '0x00000008')
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'cfg5_dir_bit_ds']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_CFG5'), '0x00000008')

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
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'mac', '00:11:22:33:44:55', '--commit']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x22334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')
              
    def test_burn_already_fused(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0x22334455')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x0011')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000300')
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'mac', '00:11:22:33:44:55', '--commit']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x22334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')
                
    def test_burn_already_fused_wrong_mac(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0xff334455')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x0011')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000300')
        self.assertEqual(1, flash_fuse(self.dir, ['--fuse', 'mac', '00:11:22:33:44:55', '--commit']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0xff334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')

    def test_verify_not_fused(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0x00000000')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x00000000')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000000')
        self.assertEqual(1, flash_fuse(self.dir, ['--fuse', 'mac', '00:11:22:33:44:55']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x00000000')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000000')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000000')
            
    def test_verify_fused(self):
        create_otp(self.dir, 'HW_OCOTP_MAC0', '0x22334455')
        create_otp(self.dir, 'HW_OCOTP_MAC1', '0x0011')
        create_otp(self.dir, 'HW_OCOTP_LOCK', '0x00000300')
        self.assertEqual(0, flash_fuse(self.dir, ['--fuse', 'mac', '00:11:22:33:44:55']))
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC0'), '0x22334455')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_MAC1'), '0x00000011')
        self.assertEqual(get_otp(self.dir, 'HW_OCOTP_LOCK'), '0x00000300')

if __name__ == '__main__':
    unittest.main()